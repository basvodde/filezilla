#include "FileZilla.h"
#include "proxy.h"
#include <errno.h>
#include "ControlSocket.h"

enum handshake_state
{
	http_wait
};

BEGIN_EVENT_TABLE(CProxySocket, wxEvtHandler)
EVT_FZ_SOCKET(wxID_ANY, CProxySocket::OnSocketEvent)
END_EVENT_TABLE()

CProxySocket::CProxySocket(wxEvtHandler* pEvtHandler, CSocket* pSocket, CControlSocket* pOwner)
	: CBackend(pEvtHandler), m_pOwner(pOwner)
{
	m_pSocket = pSocket;
	m_pSocket->SetEventHandler(this, GetId());

	m_proxyState = noconn;

	m_pSendBuffer = 0;
	m_pRecvBuffer = 0;

	m_proxyType = unknown;
}

CProxySocket::~CProxySocket()
{
	if (m_pSocket)
		m_pSocket->SetEventHandler(0, -1);
	delete [] m_pSendBuffer;
	delete [] m_pRecvBuffer;
}

static wxString base64encode(const wxString& str)
{
	// Code shamelessly taken from wxWidgets and adopted to encode UTF-8 strings.
	// wxWidget's http class encodes string from arbitrary encoding into base64,
	// could as well encode /dev/random
	static const char *base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	wxString buf;
	
	wxWX2MBbuf utf8 = str.mb_str(wxConvUTF8);
	const char* from = utf8;

	size_t len = strlen(from);
	while (len >= 3) { // encode full blocks first
		buf << wxString::Format(wxT("%c%c"), base64[(from[0] >> 2) & 0x3f], base64[((from[0] << 4) & 0x30) | ((from[1] >> 4) & 0xf)]);
		buf << wxString::Format(wxT("%c%c"), base64[((from[1] << 2) & 0x3c) | ((from[2] >> 6) & 0x3)], base64[from[2] & 0x3f]);
		from += 3;
		len -= 3;
	}
	if (len > 0) { // pad the remaining characters
		buf << wxString::Format(wxT("%c"), base64[(from[0] >> 2) & 0x3f]);
		if (len == 1) {
			buf << wxString::Format(wxT("%c="), base64[(from[0] << 4) & 0x30]);
		} else {
			buf << wxString::Format(wxT("%c%c"), base64[((from[0] << 4) & 0x30) | ((from[1] >> 4) & 0xf)], base64[(from[1] << 2) & 0x3c]);
		}
		buf << wxString::Format(wxT("="));
	}

	return buf;
}

int CProxySocket::Handshake(enum CProxySocket::ProxyType type, const wxString& host, unsigned int port, const wxString& user, const wxString& pass)
{
	if (type == UNKNOWN || host == _T("") || port < 1 || port > 65535)
		return EINVAL;

	if (m_proxyState != noconn)
		return EALREADY;

	wxWX2MBbuf host_raw = host.mb_str(wxConvUTF8);

	if (type != HTTP)
		return EPROTONOSUPPORT;

	m_user = user;
	m_pass = pass;
	m_proxyType = type;

	m_proxyState = handshake;
	m_handshakeState = http_wait;

	wxWX2MBbuf challenge;
	int challenge_len;
	if (user != _T(""))
	{
		challenge = base64encode(user + _T(":") + pass).mb_str(wxConvUTF8);
		challenge_len = strlen(challenge);
	}
	else
	{
		challenge = 0;
		challenge_len = 0;
	}

	// Bit oversized, but be on the safe side
	m_pSendBuffer = new char[70 + strlen(host_raw) * 2 + 2*5 + challenge_len];

	if (challenge)
	{
		m_sendBufferLen = sprintf(m_pSendBuffer, "CONNECT %s:%d HTTP/1.1\r\nHost: %s:%d\r\n\r\n",
								(const char*)host_raw, port,
								(const char*)host_raw, port);
	}
	else
	{
		m_sendBufferLen = sprintf(m_pSendBuffer, "CONNECT %s:%d HTTP/1.1\r\nHost: %s:%d\r\nProxy-Authorization: Basic %s\r\n\r\n",
								(const char*)host_raw, port,
								(const char*)host_raw, port,
								(const char*)challenge);
	}

	m_pRecvBuffer = new char[4096];
	m_recvBufferLen = 4096;
	m_recvBufferPos = 0;

	return EINPROGRESS;
}

void CProxySocket::OnSocketEvent(CSocketEvent& event)
{
	if (event.GetId() != GetId())
		return;

	switch (event.GetType())
	{
	case CSocketEvent::hostaddress:
		{
			const wxString& address = event.GetData();
			m_pOwner->LogMessage(Status, _("Connecting to %s..."), address.c_str()); 
		}
	case CSocketEvent::connection_next:
		if (event.GetError())
			m_pOwner->LogMessage(Status, _("Connection attempt failed with \"%s\", trying next address."), CSocket::GetErrorDescription(event.GetError()).c_str()); 
		break;
	case CSocketEvent::connection:
		if (event.GetError())
		{
			if (m_proxyState == handshake)
				m_proxyState = noconn;
			m_pEvtHandler->AddPendingEvent(event);
		}
		else
		{
			m_pOwner->LogMessage(Status, _("Connection with proxy established, performing handshake..."));
		}
		break;
	case CSocketEvent::read:
		OnReceive();
		break;
	case CSocketEvent::write:
		OnSend();
		break;
	case CSocketEvent::close:
		if (m_proxyState == handshake)
			m_proxyState = noconn;
		m_pEvtHandler->AddPendingEvent(event);
		break;
	default:
		m_pOwner->LogMessage(Debug_Warning, _T("Unhandled socket event %d"), event.GetType());
		break;
	}
}

void CProxySocket::Detach()
{
	if (!m_pSocket)
		return;

	m_pSocket->SetEventHandler(0, -1);
	m_pSocket = 0;
}

void CProxySocket::OnReceive()
{
	if (m_proxyState != handshake)
		return;

	switch (m_handshakeState)
	{
	case http_wait:
		while (true)
		{
			int error;
			int do_read = m_recvBufferLen - m_recvBufferPos - 1;
			char* end = 0;
			for (int i = 0; i < 2; i++)
			{
				int read;
				if (!i)
					read = m_pSocket->Peek(m_pRecvBuffer + m_recvBufferPos, do_read, error);
				else
					read = m_pSocket->Read(m_pRecvBuffer + m_recvBufferPos, do_read, error);
				if (read == -1)
				{
					if (error != EAGAIN)
					{
						m_proxyState = noconn;
						CSocketEvent evt(GetId(), CSocketEvent::close, error);
						m_pEvtHandler->AddPendingEvent(evt);
					}
					return;
				}
				if (!read)
				{
					m_proxyState = noconn;
					CSocketEvent evt(GetId(), CSocketEvent::close, ECONNABORTED);
					m_pEvtHandler->AddPendingEvent(evt);
					return;
				}
				if (m_pSendBuffer)
				{
					m_proxyState = noconn;
					m_pOwner->LogMessage(Debug_Warning, _T("Incoming data before requst fully sent"));
					CSocketEvent evt(GetId(), CSocketEvent::close, ECONNABORTED);
					m_pEvtHandler->AddPendingEvent(evt);
					return;
				}

				if (!i)
				{
					// Response ends with strstr
					m_pRecvBuffer[m_recvBufferPos + read] = 0;
					end = strstr(m_pRecvBuffer, "\r\n\r\n");
					if (!end)
					{
						if (m_recvBufferPos + read + 1 == m_recvBufferLen)
						{
							m_proxyState = noconn;
							m_pOwner->LogMessage(Debug_Warning, _T("Incoming header too large"));
							CSocketEvent evt(GetId(), CSocketEvent::close, ENOMEM);
							m_pEvtHandler->AddPendingEvent(evt);
							return;
						}
						do_read = read;
					}
					else
						do_read = end - m_pRecvBuffer + 4 - m_recvBufferPos;
				}
				else
				{
					if (read != do_read)
					{
						m_proxyState = noconn;
						m_pOwner->LogMessage(Debug_Warning, _T("Could not read what got peeked"));
						CSocketEvent evt(GetId(), CSocketEvent::close, ECONNABORTED);
						m_pEvtHandler->AddPendingEvent(evt);
						return;
					}
					m_recvBufferPos += read;
				}
			}

			if (!end)
				continue;
			
			end = strchr(m_pRecvBuffer, '\r');
			wxASSERT(end);
			*end = 0;
			wxString reply(m_pRecvBuffer, wxConvUTF8);
			m_pOwner->LogMessage(Response, _("Proxy reply: %s"), reply.c_str());

			if (reply.Left(10) != _T("HTTP/1.1 2") && reply.Left(10) != _T("HTTP/1.0 2"))
			{
				m_proxyState = noconn;
				CSocketEvent evt(GetId(), CSocketEvent::close, ECONNRESET);
				m_pEvtHandler->AddPendingEvent(evt);
				return;
			}

			m_proxyState = conn;
			CSocketEvent evt(GetId(), CSocketEvent::connection, 0);
			m_pEvtHandler->AddPendingEvent(evt);
			return;
		}
	default:
		m_proxyState = noconn;
		m_pOwner->LogMessage(Debug_Warning, _T("Unhandled handshake state %d"), m_handshakeState);
		CSocketEvent evt(GetId(), CSocketEvent::close, ECONNABORTED);
		m_pEvtHandler->AddPendingEvent(evt);
		return;
	}
}

void CProxySocket::OnSend()
{
	if (m_proxyState != handshake || !m_pSendBuffer)
		return;

	while (true)
	{
		int error;
		int written = m_pSocket->Write(m_pSendBuffer, m_sendBufferLen, error);
		if (written == -1)
		{
			if (error != EAGAIN)
			{
				m_proxyState = noconn;
				CSocketEvent evt(GetId(), CSocketEvent::close, error);
				m_pEvtHandler->AddPendingEvent(evt);
			}

			return;
		}

		if (written == m_sendBufferLen)
		{
			delete [] m_pSendBuffer;
			m_pSendBuffer = 0;
			return;
		}
		memmove(m_pSendBuffer, m_pSendBuffer + written, m_sendBufferLen - written);
		m_sendBufferLen -= written;
	}
}
