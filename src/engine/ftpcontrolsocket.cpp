#include "FileZilla.h"
#include "ftpcontrolsocket.h"

CFtpControlSocket::CFtpControlSocket(CFileZillaEngine *pEngine) : CControlSocket(pEngine)
{
}

CFtpControlSocket::~CFtpControlSocket()
{
	delete m_pCurOpData;
}

void CFtpControlSocket::OnReceive(wxSocketEvent &event)
{
}

void CFtpControlSocket::OnConnect(wxSocketEvent &event)
{
	LogMessage(Status, _("Connection established, waiting for welcome message..."));
}