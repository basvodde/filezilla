#include "FileZilla.h"
#include "logging_private.h"
#include "ControlSocket.h"

BEGIN_EVENT_TABLE(CControlSocket, wxEvtHandler)
	EVT_SOCKET(wxID_ANY, CControlSocket::OnSocketEvent)
END_EVENT_TABLE();

CControlSocket::CControlSocket(CFileZillaEngine *pEngine)
	: CLogging(pEngine), wxSocketClient(wxSOCKET_NOWAIT)
{
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
}

int CControlSocket::Connect(const CServer &server)
{
	LogMessage(Status, _("Connecting to %s:%d..."), server.GetHost().c_str(), server.GetPort());
	wxIPV4address addr;
	addr.Hostname(server.GetHost());
	addr.Service(server.GetPort());

	bool res = wxSocketClient::Connect(addr, false);

	if (!res && LastError() != wxSOCKET_WOULDBLOCK)
		return FZ_REPLY_ERROR;	

	return FZ_REPLY_WOULDBLOCK;
}

void CControlSocket::Disconnect()
{
	Close();
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