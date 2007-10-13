// FileZillaEngine.cpp: Implementierung der Klasse CFileZillaEngine.
//
//////////////////////////////////////////////////////////////////////

#include "FileZilla.h"
#include "ControlSocket.h"
#include "directorycache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CFileZillaEngine::CFileZillaEngine()
{
}

CFileZillaEngine::~CFileZillaEngine()
{
}

int CFileZillaEngine::Init(wxEvtHandler *pEventHandler, COptionsBase *pOptions)
{
	m_pEventHandler = pEventHandler;
	m_pOptions = pOptions;
	m_pRateLimiter = CRateLimiter::Create(m_pOptions);

	return FZ_REPLY_OK;
}

int CFileZillaEngine::Command(const CCommand &command)
{
	if (command.GetId() != cmd_cancel && IsBusy())
		return FZ_REPLY_BUSY;

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
	case cmd_cancel:
		res = Cancel(reinterpret_cast<const CCancelCommand &>(command));
		break;
	case cmd_list:
		res = List(reinterpret_cast<const CListCommand &>(command));
		break;
	case cmd_transfer:
		res = FileTransfer(reinterpret_cast<const CFileTransferCommand &>(command));
		break;
	case cmd_raw:
		res = RawCommand(reinterpret_cast<const CRawCommand&>(command));
		break;
	case cmd_delete:
		res = Delete(reinterpret_cast<const CDeleteCommand&>(command));
		break;
	case cmd_removedir:
		res = RemoveDir(reinterpret_cast<const CRemoveDirCommand&>(command));
		break;
	case cmd_mkdir:
		res = Mkdir(reinterpret_cast<const CMkdirCommand&>(command));
		break;
	case cmd_rename:
		res = Rename(reinterpret_cast<const CRenameCommand&>(command));
		break;
	case cmd_chmod:
		res = Chmod(reinterpret_cast<const CChmodCommand&>(command));
		break;
	default:
		return FZ_REPLY_SYNTAXERROR;
	}

	if (res != FZ_REPLY_WOULDBLOCK)
		ResetOperation(res);

	m_bIsInCommand = false;
	
	if (command.GetId() != cmd_disconnect)
		res |= m_nControlSocketError;
	else if (res & FZ_REPLY_DISCONNECTED)
		res = FZ_REPLY_OK;
	m_nControlSocketError = 0;
	
	return res;
}

bool CFileZillaEngine::IsBusy() const
{
	return CFileZillaEnginePrivate::IsBusy();
}

bool CFileZillaEngine::IsConnected() const
{
	return CFileZillaEnginePrivate::IsConnected();
}

CNotification* CFileZillaEngine::GetNextNotification()
{
	wxCriticalSectionLocker lock(m_lock);

	if (!m_maySendNotificationEvent)
	{
		m_maySendNotificationEvent = true;
		if (m_NotificationList.empty())
			return 0;

		m_NotificationList.push_back(0);
	}
	else if (m_NotificationList.empty())
		return 0;
	CNotification* pNotification = m_NotificationList.front();
	m_NotificationList.pop_front();
		
	return pNotification;
}

const CCommand *CFileZillaEngine::GetCurrentCommand() const
{
	return CFileZillaEnginePrivate::GetCurrentCommand();
}

enum Command CFileZillaEngine::GetCurrentCommandId() const
{
	return CFileZillaEnginePrivate::GetCurrentCommandId();
}

bool CFileZillaEngine::SetAsyncRequestReply(CAsyncRequestNotification *pNotification)
{
	if (!pNotification)
		return false;
	if (!IsBusy())
		return false;

	m_lock.Enter();
	if (pNotification->requestNumber != m_asyncRequestCounter)
	{
		m_lock.Leave();
		return false;
	}
	m_lock.Leave();

	if (!m_pControlSocket)
		return false;

	m_pControlSocket->SetAlive();
	m_pControlSocket->SetAsyncRequestReply(pNotification);

	return true;
}

bool CFileZillaEngine::IsPendingAsyncRequestReply(const CAsyncRequestNotification *pNotification)
{
	if (!pNotification)
		return false;
	if (!IsBusy())
		return false;

	wxCriticalSectionLocker lock(m_lock);
	return pNotification->requestNumber == m_asyncRequestCounter;
}

bool CFileZillaEngine::IsActive(bool recv)
{
	if (recv)
	{
		if (m_activeStatusRecv == 2)
		{
			m_activeStatusRecv = 1;
			return true;
		}
		else
		{
			m_activeStatusRecv = 0;
			return false;
		}
	}
	else
	{
		if (m_activeStatusSend == 2)
		{
			m_activeStatusSend = 1;
			return true;
		}
		else
		{
			m_activeStatusSend = 0;
			return false;
		}
	}
	return false;
}

bool CFileZillaEngine::GetTransferStatus(CTransferStatus &status, bool &changed)
{
	if (!m_pControlSocket)
	{
		changed = false;
		return false;
	}

	return m_pControlSocket->GetTransferStatus(status, changed);
}

int CFileZillaEngine::CacheLookup(const CServerPath& path, CDirectoryListing& listing)
{
	if (!IsConnected())
		return FZ_REPLY_ERROR;

	wxASSERT(m_pControlSocket->GetCurrentServer());

	CDirectoryCache cache;
	if (!cache.Lookup(listing, *m_pControlSocket->GetCurrentServer(), path, true))
		return FZ_REPLY_ERROR;

	return FZ_REPLY_OK;
}
