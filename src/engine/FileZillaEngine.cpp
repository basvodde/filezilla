// FileZillaEngine.cpp: Implementierung der Klasse CFileZillaEngine.
//
//////////////////////////////////////////////////////////////////////

#include "FileZilla.h"
#include "FileZillaEngine.h"
#include "logging_private.h"
#include "ControlSocket.h"
#include "ftpcontrolsocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CFileZillaEngine::CFileZillaEngine()
{
	m_pEventHandler = 0;
	m_pControlSocket = 0;
	m_pCurrentCommand = 0;
}

CFileZillaEngine::~CFileZillaEngine()
{
	delete m_pControlSocket;
	delete m_pCurrentCommand;

	// Delete notification list
	for (std::list<CNotification *>::iterator iter = m_NotificationList.begin(); iter != m_NotificationList.end(); iter++)
		delete *iter;
}

int CFileZillaEngine::Init(wxEvtHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;

	return FZ_REPLY_OK;
}

int CFileZillaEngine::Command(const CCommand &command)
{
	if (IsBusy())
		return FZ_REPLY_BUSY;

	switch (command.GetId())
	{
	case cmd_connect:
		return Connect(reinterpret_cast<const CConnectCommand &>(command));
	}

	return FZ_REPLY_INTERNALERROR;
}

bool CFileZillaEngine::IsBusy() const
{
	if (m_pCurrentCommand)
		return true;

	return false;
}

bool CFileZillaEngine::IsConnected() const
{
	if (!m_pControlSocket)
		return false;

	return m_pControlSocket->IsConnected();
}

int CFileZillaEngine::Connect(const CConnectCommand &command)
{
	if (IsConnected())
	{
		m_pControlSocket->Disconnect();
		delete m_pControlSocket;
	}

	switch (command.GetServer().GetProtocol())
	{
	case FTP:
		m_pControlSocket = new CFtpControlSocket(this);
		break;
	}

	int res = m_pControlSocket->Connect(command.GetServer());

	return FZ_REPLY_INTERNALERROR;
}

CNotification *CFileZillaEngine::GetNextNotification()
{
	if (m_NotificationList.empty())
		return 0;

	CNotification *pNotify = m_NotificationList.front();

	m_NotificationList.pop_front();

	return pNotify;
}

void CFileZillaEngine::AddNotification(CNotification *pNotification)
{
	bool bSend = m_NotificationList.empty();

	m_NotificationList.push_back(pNotification);

	if (bSend)
	{
		wxFzEvent evt(wxID_ANY);
		wxPostEvent(m_pEventHandler, evt);
	}
}