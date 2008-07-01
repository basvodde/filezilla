#include "FileZilla.h"
#include "externalipresolver.h"
#include "asynchostresolver.h"
#include "wx/regex.h"
#include "socket.h"
#include "backend.h"
#include <errno.h>

const wxEventType fzEVT_EXTERNALIPRESOLVE = wxNewEventType();

fzExternalIPResolveEvent::fzExternalIPResolveEvent(int id)
	: wxEvent(id, fzEVT_EXTERNALIPRESOLVE)
{
}

wxEvent* fzExternalIPResolveEvent::Clone() const
{
	return new fzExternalIPResolveEvent(GetId());
}

wxString CExternalIPResolver::m_ip;
bool CExternalIPResolver::m_checked = false;

BEGIN_EVENT_TABLE(CExternalIPResolver, wxEvtHandler)
	EVT_FZ_SOCKET(wxID_ANY, CExternalIPResolver::OnSocketEvent)
END_EVENT_TABLE();

CExternalIPResolver::CExternalIPResolver(wxEvtHandler* handler, int id /*=wxID_ANY*/)
	: m_handler(handler), m_id(id)
{
	m_pSocket = 0;
	m_done = false;
	m_pSendBuffer = 0;
	m_sendBufferPos = 0;
	m_pRecvBuffer = 0;
	m_recvBufferPos = 0;

	ResetHttpData(true);
}

CExternalIPResolver::~CExternalIPResolver()
{
	delete [] m_pSendBuffer;
	m_pSendBuffer = 0;
	delete [] m_pRecvBuffer;
	m_pRecvBuffer = 0;

	delete m_pSocket;
	m_pSocket = 0;
}

void CExternalIPResolver::GetExternalIP(const wxString& address /*=_T("")*/, bool force /*=false*/)
{
	if (m_checked)
	{
		if (force)
			m_checked = false;
		else
		{
			m_done = true;
			return;
		}
	}

	m_address = address;

	wxString host;
	int pos;
	if ((pos = address.Find(_T("://"))) != -1)
		host = address.Mid(pos + 3);
	else
		host = address;

	if ((pos = host.Find(_T("/"))) != -1)
		host = host.Left(pos);

	wxString hostWithPort = host;

	if ((pos = host.Find(':', true)) != -1)
	{
		wxString port = host.Mid(pos + 1);
		if (!port.ToULong(&m_port) || m_port < 1 || m_port > 65535)
			m_port = 80;
		host = host.Left(pos);
	}
	else
		m_port = 80;

	if (host == _T(""))
	{
		m_done = true;
		return;
	}

	m_pSocket = new CSocket(this, CBackend::GetNextId());

	int res = m_pSocket->Connect(host, m_port);
	if (res && res != EINPROGRESS)
	{
		Close(false);
		return;
	}

	wxString buffer = wxString::Format(_T("GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: close\r\n\r\n"), address.c_str(), hostWithPort.c_str(), wxString(PACKAGE_STRING, wxConvLocal).c_str());
	m_pSendBuffer = new char[strlen(buffer.mb_str()) + 1];
	strcpy(m_pSendBuffer, buffer.mb_str());
}

void CExternalIPResolver::OnSocketEvent(CSocketEvent& event)
{
	if (!m_pSocket)
		return;

	switch (event.GetType())
	{
	case CSocketEvent::read:
		OnReceive();
		break;
	case CSocketEvent::connection:
		OnConnect(event.GetError());
		break;
	case CSocketEvent::close:
		OnClose();
		break;
	case CSocketEvent::write:
		OnSend();
		break;
	default:
		break;
	}

}

void CExternalIPResolver::OnConnect(int error)
{
	if (error)
		Close(false);
}

void CExternalIPResolver::OnClose()
{
	if (m_data != _T(""))
		OnData(0, 0);
	else
		Close(false);
}

void CExternalIPResolver::OnReceive()
{
	if (!m_pRecvBuffer)
	{
		m_pRecvBuffer = new char[m_recvBufferLen];
		m_recvBufferPos = 0;
	}

	if (m_pSendBuffer)
		return;

	while (m_pSocket)
	{
		unsigned int len = m_recvBufferLen - m_recvBufferPos;
		int error;
		int read = m_pSocket->Read(m_pRecvBuffer + m_recvBufferPos, len, error);
		if (read == -1)
		{
			if (error != EAGAIN)
				Close(false);
			return;
		}

		if (!read)
		{
			Close(false);
			return;
		}

		if (m_finished)
		{
			// Just ignore all further data
			m_recvBufferPos = 0;
			return;
		}

		m_recvBufferPos += read;

		if (!m_gotHeader)
			OnHeader();
		else
		{
			if (m_transferEncoding == chunked)
				OnChunkedData();
			else
				OnData(m_pRecvBuffer, m_recvBufferPos);
		}
	}
}

void CExternalIPResolver::OnSend()
{
	while (m_pSendBuffer)
	{
		unsigned int len = strlen(m_pSendBuffer + m_sendBufferPos);
		int error;
		int written = m_pSocket->Write(m_pSendBuffer + m_sendBufferPos, len, error);
		if (written == -1)
		{
			if (error != EAGAIN)
				Close(false);
			return;
		}

		if (!written)
		{
			Close(false);
			return;
		}

		if (written == (int)len)
		{
			delete [] m_pSendBuffer;
			m_pSendBuffer = 0;

			OnReceive();
		}
		else
			m_sendBufferPos += written;
	}
}

void CExternalIPResolver::Close(bool successful)
{
	delete [] m_pSendBuffer;
	m_pSendBuffer = 0;

	delete [] m_pRecvBuffer;
	m_pRecvBuffer = 0;

	delete m_pSocket;
	m_pSocket = 0;

	if (m_done)
		return;

	m_done = true;

	if (!successful)
		m_ip = _T("");
	m_checked = true;

	if (m_handler)
	{
		fzExternalIPResolveEvent event;
		wxPostEvent(m_handler, event);
		m_handler = 0;
	}
}

void CExternalIPResolver::OnHeader()
{
	// Parse the HTTP header.
	// We do just the neccessary parsing and silently ignore most header fields
	// Redirects are supported though if the server sends the Location field.

	while (true)
	{
		// Find line ending
		unsigned int i = 0;
		for (i = 0; (i + 1) < m_recvBufferPos; i++)
		{
			if (m_pRecvBuffer[i] == '\r')
			{
				if (m_pRecvBuffer[i + 1] != '\n')
				{
					Close(false);
					return;
				}
				break;
			}
		}
		if ((i + 1) >= m_recvBufferPos)
		{
			if (m_recvBufferPos == m_recvBufferLen)
			{
				// We don't support header lines larger than 4096
				Close(false);
				return;
			}
			return;
		}

		m_pRecvBuffer[i] = 0;

		if (!m_responseCode)
		{
			m_responseString = wxString(m_pRecvBuffer, wxConvLocal);
			if (m_recvBufferPos < 16 || memcmp(m_pRecvBuffer, "HTTP/1.", 7))
			{
				// Invalid HTTP Status-Line
				Close(false);
				return;
			}

			if (m_pRecvBuffer[9] < '1' || m_pRecvBuffer[9] > '5' ||
				m_pRecvBuffer[10] < '0' || m_pRecvBuffer[10] > '9' ||
				m_pRecvBuffer[11] < '0' || m_pRecvBuffer[11] > '9')
			{
				// Invalid response code
				Close(false);
				return;
			}

			m_responseCode = (m_pRecvBuffer[9] - '0') * 100 + (m_pRecvBuffer[10] - '0') * 10 + m_pRecvBuffer[11] - '0';

			if (m_responseCode >= 400)
			{
				// Failed request
				Close(false);
				return;
			}

			if (m_responseCode == 305)
			{
				// Unsupported redirect
				Close(false);
				return;
			}
		}
		else
		{
			if (!i)
			{
				// End of header, data from now on

				// Redirect if neccessary
				if (m_responseCode >= 300)
				{
					delete m_pSocket;
					m_pSocket = 0;

					delete [] m_pRecvBuffer;
					m_pRecvBuffer = 0;

					wxString location = m_location;

					ResetHttpData(false);

					GetExternalIP(location);
					return;
				}

				m_gotHeader = true;

				memmove(m_pRecvBuffer, m_pRecvBuffer + 2, m_recvBufferPos - 2);
				m_recvBufferPos -= 2;
				if (m_recvBufferPos)
				{
					if (m_transferEncoding == chunked)
						OnChunkedData();
					else
						OnData(m_pRecvBuffer, m_recvBufferPos);
				}
				return;
			}
			if (m_recvBufferPos > 12 && !memcmp(m_pRecvBuffer, "Location: ", 10))
			{
				m_location = wxString(m_pRecvBuffer + 10, wxConvLocal);
			}
			else if (m_recvBufferPos > 21 && !memcmp(m_pRecvBuffer, "Transfer-Encoding: ", 19))
			{
				if (!strcmp(m_pRecvBuffer + 19, "chunked"))
					m_transferEncoding = chunked;
				else if (!strcmp(m_pRecvBuffer + 19, "identity"))
					m_transferEncoding = identity;
				else
					m_transferEncoding = unknown;
			}
		}

		memmove(m_pRecvBuffer, m_pRecvBuffer + i + 2, m_recvBufferPos - i - 2);
		m_recvBufferPos -= i + 2;

		if (!m_recvBufferPos)
			break;
	}
}

void CExternalIPResolver::OnData(char* buffer, unsigned int len)
{
	if (buffer)
	{
		unsigned int i;
		for (i = 0; i < len; i++)
		{
			if (buffer[i] == '\r' || buffer[i] == '\n')
				break;
		}

		if (i)
			m_data += wxString(buffer, wxConvLocal, i);

		if (i == len)
			return;
	}

	// Validate ip address
	wxString digit = _T("0*[0-9]{1,3}");
	const wxChar* dot = _T("\\.");
	wxString exp = _T("(^|[^\\.[:digit:]])(") + digit + dot + digit + dot + digit + dot + digit + _T(")([^\\.[:digit:]]|$)");
	wxRegEx regex;
	regex.Compile(exp);

	if (!regex.Matches(m_data))
	{
		Close(false);
		return;
	}

	m_ip = regex.GetMatch(m_data, 2);
	Close(true);
}

void CExternalIPResolver::ResetHttpData(bool resetRedirectCount)
{
	m_gotHeader = false;
	m_location = _T("");
	m_responseCode = 0;
	m_responseString = _T("");
	if (resetRedirectCount)
		m_redirectCount = 0;

	m_transferEncoding = unknown;

	m_chunkData.getTrailer = false;
	m_chunkData.size = 0;
	m_chunkData.terminateChunk = false;

	m_finished = false;
}

void CExternalIPResolver::OnChunkedData()
{
	char* p = m_pRecvBuffer;
	unsigned int len = m_recvBufferPos;

	while (true)
	{
		if (m_chunkData.size != 0)
		{
			unsigned int dataLen = len;
			if (m_chunkData.size < len)
				dataLen = m_chunkData.size.GetLo();
			OnData(p, dataLen);
			if (!m_pRecvBuffer)
				return;

            m_chunkData.size -= dataLen;
			p += dataLen;
			len -= dataLen;

			if (m_chunkData.size == 0)
				m_chunkData.terminateChunk = true;

			if (!len)
				break;
		}

		// Find line ending
		unsigned int i = 0;
		for (i = 0; (i + 1) < len; i++)
		{
			if (p[i] == '\r')
			{
				if (p[i + 1] != '\n')
				{
					Close(false);
					return;
				}
				break;
			}
		}
		if ((i + 1) >= len)
		{
			if (len == m_recvBufferLen)
			{
				// We don't support lines larger than 4096
				Close(false);
				return;
			}
			break;
		}

		p[i] = 0;

		if (m_chunkData.terminateChunk)
		{
			if (i)
			{
				// Chunk has to end with CRLF
				Close(false);
				return;
			}
			m_chunkData.terminateChunk = false;
		}
		else if (m_chunkData.getTrailer)
		{
			if (!i)
			{
				m_finished = true;
				m_recvBufferPos = 0;
				return;
			}

			// Ignore the trailer
		}
		else
		{
			// Read chunk size
			char* q = p;
			while (*q)
			{
				if (*q >= '0' && *q <= '9')
				{
					m_chunkData.size *= 16;
					m_chunkData.size += *q - '0';
				}
				else if (*q >= 'A' && *q <= 'F')
				{
					m_chunkData.size *= 10;
					m_chunkData.size += *q - 'A' + 10;
				}
				else if (*q >= 'a' && *q <= 'f')
				{
					m_chunkData.size *= 10;
					m_chunkData.size += *q - 'a' + 10;
				}
				else if (*q == ';' || *q == ' ')
					break;
				else
				{
					// Invalid size
					Close(false);
					return;
				}
				q++;
			}
			if (m_chunkData.size == 0)
				m_chunkData.getTrailer = true;
		}

		p += i + 2;
		len -= i + 2;

		if (!len)
			break;
	}

	if (p != m_pRecvBuffer)
	{
		memmove(m_pRecvBuffer, p, len);
		m_recvBufferPos = len;
	}
}
