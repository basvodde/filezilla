#include "FileZilla.h"
#include "logging_private.h"
#include "ControlSocket.h"

BEGIN_EVENT_TABLE(CControlSocket, wxEvtHandler)
	EVT_SOCKET(wxID_ANY, CControlSocket::OnSocketEvent)
END_EVENT_TABLE();

COpData::COpData()
{
}

COpData::~COpData()
{
}


CControlSocket::CControlSocket(CFileZillaEngine *pEngine)
	: CLogging(pEngine), wxSocketClient(wxSOCKET_NOWAIT)
{
	m_pEngine = pEngine;
	m_pCurOpData = 0;
	m_nOpState = 0;
	m_pSendBuffer = 0;
	m_nSendBufferLen = 0;
	m_pCurrentServer = 0;

	SetEvtHandlerEnabled(true);
	SetEventHandler(*this);
	SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	Notify(true);
}

CControlSocket::~CControlSocket()
{
	DoClose();
}

void CControlSocket::OnConnect(wxSocketEvent &event)
{
}

void CControlSocket::OnReceive(wxSocketEvent &event)
{
}

void CControlSocket::OnSend(wxSocketEvent &event)
{
	if (m_pSendBuffer)
	{
		if (!m_nSendBufferLen)
		{
			delete [] m_pSendBuffer;
			m_pSendBuffer = 0;
			return;
		}

		Write(m_pSendBuffer, m_nSendBufferLen);
		if (Error())
		{
			if (LastError() != wxSOCKET_WOULDBLOCK)
				DoClose();
			return;
		}

		int numsent = LastCount();
		if (numsent = m_nSendBufferLen)
		{
			m_nSendBufferLen = 0;
			delete [] m_pSendBuffer;
			m_pSendBuffer = 0;
		}
		else
		{
			memmove(m_pSendBuffer, m_pSendBuffer + numsent, m_nSendBufferLen - numsent);
			m_nSendBufferLen -= numsent;
		}
	}
}

void CControlSocket::OnClose(wxSocketEvent &event)
{
	if (GetCurrentCommandId() != cmd_connect)
		LogMessage(::Error, _("Disconnected from server"));
	DoClose();
}

int CControlSocket::Connect(const CServer &server)
{
	LogMessage(Status, _("Connecting to %s:%d..."), server.GetHost().c_str(), server.GetPort());
	wxIPV4address addr;
	addr.Hostname(server.GetHost());
	addr.Service(server.GetPort());

	m_pCurrentServer = new CServer(server);

	bool res = wxSocketClient::Connect(addr, false);

	if (!res && LastError() != wxSOCKET_WOULDBLOCK)
		return ResetOperation(FZ_REPLY_ERROR);

	return FZ_REPLY_WOULDBLOCK;
}

int CControlSocket::Disconnect()
{
	LogMessage(Status, _("Disconnected from server"));

	DoClose();
	return FZ_REPLY_OK;
}

void CControlSocket::OnSocketEvent(wxSocketEvent &event)
{
	switch (event.GetSocketEvent())
	{
	case wxSOCKET_CONNECTION:
		OnConnect(event);
		break;
	case wxSOCKET_INPUT:
		OnReceive(event);
		break;
	case wxSOCKET_OUTPUT:
		OnSend(event);
		break;
	case wxSOCKET_LOST:
		OnClose(event);
		break;
	}
}

enum Command CControlSocket::GetCurrentCommandId() const
{
	return m_pEngine->GetCurrentCommandId();
}

int CControlSocket::ResetOperation(int nErrorCode)
{
	if (m_pCurOpData)
	{
		delete m_pCurOpData;
		m_pCurOpData = 0;
	}
	m_nOpState = 0;

	if (nErrorCode != FZ_REPLY_OK)
	{
		if (GetCurrentCommandId() == cmd_connect)
			LogMessage(::Error, _("Could not connect to server"));
	}

	return m_pEngine->ResetOperation(nErrorCode);
}

int CControlSocket::DoClose(int nErrorCode /*=FZ_REPLY_DISCONNECTED*/)
{
	nErrorCode = ResetOperation(FZ_REPLY_ERROR | FZ_REPLY_DISCONNECTED | nErrorCode);
	Close();

	delete [] m_pSendBuffer;
	m_pSendBuffer = 0;
	m_nSendBufferLen = 0;

	delete m_pCurrentServer;
	m_pCurrentServer = 0;

	return nErrorCode;
}

bool CControlSocket::Send(const char *buffer, int len)
{
	if (m_pSendBuffer)
	{
		char *tmp = m_pSendBuffer;
		m_pSendBuffer = new char[m_nSendBufferLen + len];
		memcpy(m_pSendBuffer, tmp, m_nSendBufferLen);
		memcpy(m_pSendBuffer + m_nSendBufferLen, buffer, len);
		m_nSendBufferLen += len;
		delete [] tmp;
	}
	else
	{
		Write(buffer, len);
		int numsent = 0;
		if (Error())
		{
			if (LastError() != wxSOCKET_WOULDBLOCK)
			{
				DoClose();
				return false;
			}
		}
		else
			numsent = LastCount();

		if (numsent < len)
		{
			char *tmp = m_pSendBuffer;
			m_pSendBuffer = new char[m_nSendBufferLen + len - numsent];
			memcpy(m_pSendBuffer, tmp, m_nSendBufferLen);
			memcpy(m_pSendBuffer + m_nSendBufferLen, buffer, len - numsent);
			m_nSendBufferLen += len - numsent;
			delete [] tmp;
		}
	}

	return true;
}