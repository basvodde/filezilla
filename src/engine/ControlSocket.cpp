#include "FileZilla.h"
#include "logging_private.h"
#include "ControlSocket.h"

BEGIN_EVENT_TABLE(CControlSocket, wxEvtHandler)
	EVT_SOCKET(wxID_ANY, CControlSocket::OnSocketEvent)
END_EVENT_TABLE();

CControlSocket::CControlSocket(CFileZillaEngine *pEngine)
	: CLogging(pEngine), wxSocketClient(wxSOCKET_NOWAIT)
{
	m_pEngine = pEngine;
	m_pCurOpData = 0;
	m_nOpState = 0;

	SetEvtHandlerEnabled(true);
	SetEventHandler(*this);
	SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	Notify(true);
}

CControlSocket::~CControlSocket()
{
	delete m_pCurOpData;
}

void CControlSocket::OnConnect(wxSocketEvent &event)
{
}

void CControlSocket::OnReceive(wxSocketEvent &event)
{
}

void CControlSocket::OnSend(wxSocketEvent &event)
{
}

void CControlSocket::OnClose(wxSocketEvent &event)
{
	if (GetCurrentCommandId() == cmd_connect)
		LogMessage(::Error, _("Could not connect to server"));
	else
		LogMessage(::Error, _("Disconnected from server"));
	DoClose();
}

int CControlSocket::Connect(const CServer &server)
{
	LogMessage(Status, _("Connecting to %s:%d..."), server.GetHost().c_str(), server.GetPort());
	wxIPV4address addr;
	addr.Hostname(server.GetHost());
	addr.Service(server.GetPort());

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

	return m_pEngine->ResetOperation(nErrorCode);
}

int CControlSocket::DoClose(int nErrorCode /*=FZ_REPLY_DISCONNECTED*/)
{
	nErrorCode = ResetOperation(FZ_REPLY_ERROR | FZ_REPLY_DISCONNECTED | nErrorCode);
	Close();
	return nErrorCode;
}