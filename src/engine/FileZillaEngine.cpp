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
	m_bIsInCommand = false;
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
	if (command.GetId() != cmd_cancel && IsBusy())
		return FZ_REPLY_BUSY;

	m_pCurrentCommand = command.Clone();

	m_bIsInCommand = true;

	int res = FZ_REPLY_INTERNALERROR;
	switch (command.GetId())
	{
	case cmd_connect:
		res = Connect(reinterpret_cast<const CConnectCommand &>(command));
		break;
	case cmd_disconnect:
		res = Disconnect(reinterpret_cast<const CDisconnectCommand &>(command));
		break;
	}

	if (res != FZ_REPLY_WOULDBLOCK)
		ResetOperation(res);

	m_bIsInCommand = false;
	return res;
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
		return FZ_REPLY_ALREADYCONNECTED;

	switch (command.GetServer().GetProtocol())
	{
	case FTP:
		m_pControlSocket = new CFtpControlSocket(this);
		break;
	}

	int res = m_pControlSocket->Connect(command.GetServer());

	return res;
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

const CCommand *CFileZillaEngine::GetCurrentCommand() const
{
	return m_pCurrentCommand;
}

enum Command CFileZillaEngine::GetCurrentCommandId() const
{
	if (!m_pCurrentCommand)
		return cmd_none;

	else
		return GetCurrentCommand()->GetId();
}

int CFileZillaEngine::ResetOperation(int nErrorCode)
{
	if (!m_bIsInCommand && m_pCurrentCommand)
	{
		COperationNotification *notification = new COperationNotification();
		notification->nReplyCode = nErrorCode;
		notification->commandId = m_pCurrentCommand->GetId();
		AddNotification(notification);
	}

	delete m_pCurrentCommand;
	m_pCurrentCommand = 0;

	return nErrorCode;
}

int CFileZillaEngine::Disconnect(const CDisconnectCommand &command)
{
	if (!IsConnected())
		return FZ_REPLY_OK;

	int res = m_pControlSocket->Disconnect();
	if (res == FZ_REPLY_OK)
	{
		delete m_pControlSocket;
		m_pControlSocket = 0;
	}

	return res;
}