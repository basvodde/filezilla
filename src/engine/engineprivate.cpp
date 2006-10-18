#include "FileZilla.h"
#include "asynchostresolver.h"
#include "ControlSocket.h"
#include "ftpcontrolsocket.h"
#include "sftpcontrolsocket.h"
#include "directorycache.h"
#include "logging_private.h"
#include "httpcontrolsocket.h"

class wxFzEngineEvent : public wxEvent
{
public:
	wxFzEngineEvent(int id, enum EngineNotificationType eventType, int data = 0);
	virtual wxEvent *Clone() const;

	enum EngineNotificationType m_eventType;
	int data;
};

extern const wxEventType fzEVT_ENGINE_NOTIFICATION;
typedef void (wxEvtHandler::*fzEngineEventFunction)(wxFzEngineEvent&);

#define EVT_FZ_ENGINE_NOTIFICATION(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        fzEVT_ENGINE_NOTIFICATION, id, -1, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( fzEngineEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

std::list<CFileZillaEnginePrivate*> CFileZillaEnginePrivate::m_engineList;
int CFileZillaEnginePrivate::m_activeStatusSend = 0;
int CFileZillaEnginePrivate::m_activeStatusRecv = 0;

DEFINE_EVENT_TYPE(fzEVT_ENGINE_NOTIFICATION);

wxFzEngineEvent::wxFzEngineEvent(int id, enum EngineNotificationType eventType, int data /*=0*/) : wxEvent(id, fzEVT_ENGINE_NOTIFICATION)
{
	m_eventType = eventType;
	wxFzEngineEvent::data = data;
}

wxEvent *wxFzEngineEvent::Clone() const
{
	return new wxFzEngineEvent(*this);
}


BEGIN_EVENT_TABLE(CFileZillaEnginePrivate, wxEvtHandler)
	EVT_FZ_ENGINE_NOTIFICATION(wxID_ANY, CFileZillaEnginePrivate::OnEngineEvent)
	EVT_FZ_ASYNCHOSTRESOLVE(wxID_ANY, CFileZillaEnginePrivate::OnAsyncHostResolver)
END_EVENT_TABLE();

CFileZillaEnginePrivate::CFileZillaEnginePrivate()
{
	m_maySendNotificationEvent = true;
	m_pEventHandler = 0;
	m_pControlSocket = 0;
	m_pCurrentCommand = 0;
	m_bIsInCommand = false;
	m_nControlSocketError = 0;
	m_asyncRequestCounter = 0;
	m_engineList.push_back(this);

	m_pLogging = new CLogging(this);
}

CFileZillaEnginePrivate::~CFileZillaEnginePrivate()
{
	delete m_pControlSocket;
	delete m_pCurrentCommand;

	// Delete notification list
	for (std::list<CNotification *>::iterator iter = m_NotificationList.begin(); iter != m_NotificationList.end(); iter++)
		delete *iter;

	for (std::list<CAsyncHostResolver *>::iterator iter = m_HostResolverThreads.begin(); iter != m_HostResolverThreads.end(); iter++)
	{
		CAsyncHostResolver* pResolver = *iter;
		pResolver->SetObsolete();
		pResolver->Wait();
		delete pResolver;					
	}

	// Remove ourself from the engine list
	for (std::list<CFileZillaEnginePrivate*>::iterator iter = m_engineList.begin(); iter != m_engineList.end(); iter++)
		if (*iter == this)
		{
			m_engineList.erase(iter);
			break;
		}

	delete m_pLogging;
}

bool CFileZillaEnginePrivate::SendEvent(enum EngineNotificationType eventType, int data /*=0*/)
{
	wxFzEngineEvent evt(wxID_ANY, eventType, data);
	wxPostEvent(this, evt);
	return true;
}

void CFileZillaEnginePrivate::OnEngineEvent(wxFzEngineEvent &event)
{
	switch (event.m_eventType)
	{
	case engineCancel:
		if (!IsBusy())
			break;

		m_pControlSocket->Cancel();
		break;
	case engineTransferEnd:
		if (m_pControlSocket)
			m_pControlSocket->TransferEnd(event.data);
	default:
		break;
	}
}

void CFileZillaEnginePrivate::OnAsyncHostResolver(fzAsyncHostResolveEvent& event)
{
	if (m_HostResolverThreads.empty())
		return;
	CAsyncHostResolver *pResolver = m_HostResolverThreads.front();
	m_HostResolverThreads.pop_front();

	std::list<CAsyncHostResolver *> remaining;
	for (std::list<CAsyncHostResolver *>::iterator iter = m_HostResolverThreads.begin(); iter != m_HostResolverThreads.end(); iter++)
	{
		CAsyncHostResolver* pResolver = *iter;
		pResolver->SetObsolete();
		if (!pResolver->Done())
			remaining.push_back(pResolver);
		else
		{
			pResolver->Wait();
			delete pResolver;					
		}
	}
	m_HostResolverThreads.clear();
	m_HostResolverThreads = remaining;

	if (!pResolver->Done())
		m_HostResolverThreads.push_front(pResolver);
	else
	{
		if (!pResolver->Obsolete())
			m_pControlSocket->ContinueConnect(pResolver->Successful() ? &pResolver->m_Address : 0);
		pResolver->Wait();
		delete pResolver;
	}
}

bool CFileZillaEnginePrivate::IsBusy() const
{
	return m_pCurrentCommand != 0;
}

bool CFileZillaEnginePrivate::IsConnected() const
{
	if (!m_pControlSocket)
		return false;

	return m_pControlSocket->Connected();
}

const CCommand *CFileZillaEnginePrivate::GetCurrentCommand() const
{
	return m_pCurrentCommand;
}

enum Command CFileZillaEnginePrivate::GetCurrentCommandId() const
{
	if (!m_pCurrentCommand)
		return cmd_none;

	else
		return GetCurrentCommand()->GetId();
}

void CFileZillaEnginePrivate::AddNotification(CNotification *pNotification)
{
	m_lock.Enter();
	m_NotificationList.push_back(pNotification);

	if (m_maySendNotificationEvent && m_pEventHandler)
	{
		m_maySendNotificationEvent = false;
		m_lock.Leave();
		wxFzEvent evt(wxID_ANY);
		evt.SetEventObject(this);
		wxPostEvent(m_pEventHandler, evt);
	}
	else
		m_lock.Leave();
}

int CFileZillaEnginePrivate::ResetOperation(int nErrorCode)
{
	if (m_pCurrentCommand)
	{
		if ((nErrorCode & FZ_REPLY_NOTSUPPORTED) == FZ_REPLY_NOTSUPPORTED)
		{
			wxASSERT(m_bIsInCommand);
			m_pLogging->LogMessage(Error, _("Command not supported by this protocol"));
		}

		if (!m_bIsInCommand)
		{
			COperationNotification *notification = new COperationNotification();
			notification->nReplyCode = nErrorCode;
			notification->commandId = m_pCurrentCommand->GetId();
			AddNotification(notification);
		}
		else
			m_nControlSocketError |= nErrorCode;
	}

	delete m_pCurrentCommand;
	m_pCurrentCommand = 0;

	if (!m_HostResolverThreads.empty())
		m_HostResolverThreads.front()->SetObsolete();

	return nErrorCode;
}

void CFileZillaEnginePrivate::SetActive(bool recv)
{
	if (m_pControlSocket)
		m_pControlSocket->SetAlive();

	m_lock.Enter();
	if (recv)
	{
		if (!m_activeStatusRecv)
		{
			m_activeStatusRecv = 2;
			m_lock.Leave();
			AddNotification(new CActiveNotification(true));
			return;
		}
	}
	else
	{
		if (!m_activeStatusSend)
		{
			m_activeStatusSend = 2;
			m_lock.Leave();
			AddNotification(new CActiveNotification(false));
			return;
		}
	}
	m_lock.Leave();
}

unsigned int CFileZillaEnginePrivate::GetNextAsyncRequestNumber()
{
	wxCriticalSectionLocker lock(m_lock);
	return ++m_asyncRequestCounter;
}

void CFileZillaEnginePrivate::AddNewAsyncHostResolver(CAsyncHostResolver* pResolver)
{
	wxASSERT(pResolver);

	if (!m_HostResolverThreads.empty())
		m_HostResolverThreads.front()->SetObsolete();
	
	m_HostResolverThreads.push_front(pResolver);
}

// Command handlers
int CFileZillaEnginePrivate::Connect(const CConnectCommand &command)
{
	if (IsConnected())
		return FZ_REPLY_ALREADYCONNECTED;

	if (m_pControlSocket)
	{
		delete m_pControlSocket;
		m_pControlSocket = 0;
	}

	m_pCurrentCommand = command.Clone();
	switch (command.GetServer().GetProtocol())
	{
	case FTP:
		m_pControlSocket = new CFtpControlSocket(this);
		break;
	case SFTP:
		m_pControlSocket = new CSftpControlSocket(this);
		break;
	case HTTP:
		m_pControlSocket = new CHttpControlSocket(this);
		break;
	default:
		return FZ_REPLY_SYNTAXERROR;
	}

	int res = m_pControlSocket->Connect(command.GetServer());

	return res;
}

int CFileZillaEnginePrivate::Disconnect(const CDisconnectCommand &command)
{
	if (!IsConnected())
		return FZ_REPLY_OK;

	m_pCurrentCommand = command.Clone();
	int res = m_pControlSocket->Disconnect();
	if (res == FZ_REPLY_OK)
	{
		delete m_pControlSocket;
		m_pControlSocket = 0;
	}

	return res;
}

int CFileZillaEnginePrivate::Cancel(const CCancelCommand &command)
{
	if (!IsBusy())
		return FZ_REPLY_OK;

	SendEvent(engineCancel);

	return FZ_REPLY_WOULDBLOCK;
}

int CFileZillaEnginePrivate::List(const CListCommand &command)
{
	if (!IsConnected())
		return FZ_REPLY_NOTCONNECTED;

	bool refresh = command.Refresh();
	if (!command.Refresh() && !command.GetPath().IsEmpty())
	{
		const CServer* pServer = m_pControlSocket->GetCurrentServer();
		if (pServer)
		{
			CDirectoryListing *pListing = new CDirectoryListing;
			CDirectoryCache cache;
			bool found = cache.Lookup(*pListing, *pServer, command.GetPath(), command.GetSubDir(), true);
			if (found)
			{
				
				if (pListing->m_hasUnsureEntries)
					refresh = true;
				else
				{
					m_lastListDir = pListing->path;
					m_lastListTime = wxDateTime::Now();
					CDirectoryListingNotification *pNotification = new CDirectoryListingNotification(pListing->path);
					AddNotification(pNotification);
					delete pListing;
					return FZ_REPLY_OK;
				}
			}
			delete pListing;
		}
	}
	if (IsBusy())
		return FZ_REPLY_BUSY;

	m_pCurrentCommand = command.Clone();
	return m_pControlSocket->List(command.GetPath(), command.GetSubDir(), refresh);
}

int CFileZillaEnginePrivate::FileTransfer(const CFileTransferCommand &command)
{
	if (!IsConnected())
		return FZ_REPLY_NOTCONNECTED;

	if (IsBusy())
		return FZ_REPLY_BUSY;

	m_pCurrentCommand = command.Clone();
	return m_pControlSocket->FileTransfer(command.GetLocalFile(), command.GetRemotePath(), command.GetRemoteFile(), command.Download(), command.GetTransferSettings());
}

int CFileZillaEnginePrivate::RawCommand(const CRawCommand& command)
{
	if (!IsConnected())
		return FZ_REPLY_NOTCONNECTED;

	if (IsBusy())
		return FZ_REPLY_BUSY;

	if (command.GetCommand() == _T(""))
		return FZ_REPLY_SYNTAXERROR;

	m_pCurrentCommand = command.Clone();
	return m_pControlSocket->RawCommand(command.GetCommand());
}

int CFileZillaEnginePrivate::Delete(const CDeleteCommand& command)
{
	if (!IsConnected())
		return FZ_REPLY_NOTCONNECTED;

	if (IsBusy())
		return FZ_REPLY_BUSY;

	if (command.GetPath().IsEmpty() ||
		command.GetFile() == _T(""))
		return FZ_REPLY_SYNTAXERROR;

	m_pCurrentCommand = command.Clone();
	return m_pControlSocket->Delete(command.GetPath(), command.GetFile());
}

int CFileZillaEnginePrivate::RemoveDir(const CRemoveDirCommand& command)
{
	if (!IsConnected())
		return FZ_REPLY_NOTCONNECTED;

	if (IsBusy())
		return FZ_REPLY_BUSY;

	if (command.GetPath().IsEmpty() ||
		command.GetSubDir() == _T(""))
		return FZ_REPLY_SYNTAXERROR;

	m_pCurrentCommand = command.Clone();
	return m_pControlSocket->RemoveDir(command.GetPath(), command.GetSubDir());
}

int CFileZillaEnginePrivate::Mkdir(const CMkdirCommand& command)
{
	if (!IsConnected())
		return FZ_REPLY_NOTCONNECTED;

	if (IsBusy())
		return FZ_REPLY_BUSY;

	if (command.GetPath().IsEmpty() || !command.GetPath().HasParent())
		return FZ_REPLY_SYNTAXERROR;

	m_pCurrentCommand = command.Clone();
	return m_pControlSocket->Mkdir(command.GetPath());
}

int CFileZillaEnginePrivate::Rename(const CRenameCommand& command)
{
	if (!IsConnected())
		return FZ_REPLY_NOTCONNECTED;

	if (IsBusy())
		return FZ_REPLY_BUSY;

	if (command.GetFromPath().IsEmpty() || command.GetToPath().IsEmpty() ||
		command.GetFromFile() == _T("") || command.GetToFile() == _T(""))
		return FZ_REPLY_SYNTAXERROR;

	m_pCurrentCommand = command.Clone();
	return m_pControlSocket->Rename(command);
}

int CFileZillaEnginePrivate::Chmod(const CChmodCommand& command)
{
	if (!IsConnected())
		return FZ_REPLY_NOTCONNECTED;

	if (IsBusy())
		return FZ_REPLY_BUSY;

	if (command.GetPath().IsEmpty() || command.GetFile().IsEmpty() ||
		command.GetPermission() == _T(""))
		return FZ_REPLY_SYNTAXERROR;

	m_pCurrentCommand = command.Clone();
	return m_pControlSocket->Chmod(command);
}

void CFileZillaEnginePrivate::SendDisconnectNotification()
{
	m_lastListDir.Clear();
	CDirectoryListingNotification *pNotification = new CDirectoryListingNotification(CServerPath());
	AddNotification(pNotification);
}

void CFileZillaEnginePrivate::SendDirectoryListingNotification(const CServerPath& path, bool onList, bool modified, bool failed)
{
	wxASSERT(m_pControlSocket);
	wxASSERT(onList || modified);
	
	const CServer* const pOwnServer = m_pControlSocket->GetCurrentServer();
	wxASSERT(pOwnServer);

	m_lastListDir = path;

	if (failed)
	{
		CDirectoryListingNotification *pNotification = new CDirectoryListingNotification(path, false, true);
		AddNotification(pNotification);
		m_lastListTime = CTimeEx::Now();

		// On failed messages, we don't notify other engines
		return;
	}

	const CDirectoryCache cache;
	
	CTimeEx changeTime;
	cache.GetChangeTime(changeTime, *pOwnServer, path);
	
	CDirectoryListingNotification *pNotification = new CDirectoryListingNotification(path, !onList);
	AddNotification(pNotification);
	m_lastListTime = changeTime;

	if (!modified)
		return;

	// Iterate over the other engine, send notification if last listing
	// directory is the same
	for (std::list<CFileZillaEnginePrivate*>::iterator iter = m_engineList.begin(); iter != m_engineList.end(); iter++)
	{
		CFileZillaEnginePrivate* const pEngine = *iter;
		if (!pEngine->m_pControlSocket || pEngine->m_pControlSocket == m_pControlSocket)
			continue;

		const CServer* const pServer = pEngine->m_pControlSocket->GetCurrentServer();
		if (!pServer || *pServer != *pOwnServer)
			continue;

		if (pEngine->m_lastListDir != path)
			continue;

		if (changeTime <= pEngine->m_lastListTime)
			continue;
		
		pEngine->m_lastListTime = changeTime;
		CDirectoryListingNotification *pNotification = new CDirectoryListingNotification(path, true);
		pEngine->AddNotification(pNotification);
	}
}
