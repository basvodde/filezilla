#include "FileZilla.h"
#include "ControlSocket.h"


BEGIN_EVENT_TABLE(CControlSocket, wxEvtHandler)
	EVT_SOCKET(wxSOCKET_CONNECTION, CControlSocket::OnConnect)
	EVT_SOCKET(wxSOCKET_INPUT, CControlSocket::OnReceive)
	EVT_SOCKET(wxSOCKET_OUTPUT, CControlSocket::OnSend)
	EVT_SOCKET(wxSOCKET_LOST, CControlSocket::OnClose)
END_EVENT_TABLE();

CControlSocket::CControlSocket()
{
}

CControlSocket::~CControlSocket()
{
}

