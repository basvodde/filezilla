// FileZillaEngine.cpp: Implementierung der Klasse CFileZillaEngine.
//
//////////////////////////////////////////////////////////////////////

#include "FileZilla.h"
#include "engine_private.h"
#include "logging_private.h"
#include "ControlSocket.h"
#include "ftpcontrolsocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const wxEventType fzEVT_ENGINE_NOTIFICATION = wxNewEventType();

wxFzEngineEvent::wxFzEngineEvent(int id, enum EngineNotificationType eventType, int data /*=0*/) : wxEvent(id, fzEVT_ENGINE_NOTIFICATION)
{
	m_eventType = eventType;
	wxFzEngineEvent::data = data;
}

wxEvent *wxFzEngineEvent::Clone() const
{
	return new wxFzEngineEvent(*this);
}

BEGIN_EVENT_TABLE(CFileZillaEngine, wxEvtHandler)
	EVT_FZ_ENGINE_NOTIFICATION(wxID_ANY, CFileZillaEngine::OnEngineEvent)
END_EVENT_TABLE();

CFileZillaEngine::CFileZillaEngine()
{
	m_pEventHandler = 0;
	m_pControlSocket = 0;
	m_pCurrentCommand = 0;
	m_bIsInCommand = false;
	m_nControlSocketError = 0;
}

CFileZillaEngine::~CFileZillaEngine()
{
	delete m_pControlSocket;
	delete m_pCurrentCommand;

	// Delete notification list
	for (std::list<CNotification *>::iterator iter = m_NotificationList.begin(); iter != m_NotificationList.end(); iter++)
		delete *iter;
}

int CFileZillaEngine::Init(wxEvtHandler *pEventHandler, COptionsBase *pOptions)
{
	m_pEventHandler = pEventHandler;
	m_pOptions = pOptions;

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

	if (m_pControlSocket)
		delete m_pControlSocket;

	m_pCurrentCommand = command.Clone();
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
	if (m_pCurrentCommand)
	{
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

int CFileZillaEngine::Disconnect(const CDisconnectCommand &command)
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

int CFileZillaEngine::Cancel(const CCancelCommand &command)
{
	if (!IsBusy())
		return FZ_REPLY_WOULDBLOCK;

	SendEvent(engineCancel);

	return FZ_REPLY_WOULDBLOCK;
}

int CFileZillaEngine::List(const CListCommand &command)
{
	if (!IsConnected())
		return FZ_REPLY_NOTCONNECTED;

	if (IsBusy())
		return FZ_REPLY_BUSY;

	m_pCurrentCommand = command.Clone();
	return m_pControlSocket->List(command.GetPath(), command.GetSubDir());
}

void CFileZillaEngine::OnEngineEvent(wxFzEngineEvent &event)
{
	switch (event.m_eventType)
	{
	case engineCancel:
		if (!IsBusy())
			break;

		m_pControlSocket->Cancel();
		break;
	case engineHostresolve:
		{
			std::list<CAsyncHostResolver *> remaining;
			for (std::list<CAsyncHostResolver *>::iterator iter = m_HostResolverThreads.begin(); iter != m_HostResolverThreads.end(); iter++)
			{
				if (!(*iter)->Done())
					remaining.push_back(*iter);
				else
				{
					if (!(*iter)->Obsolete())
					{
						if (GetCurrentCommandId() == cmd_connect)
							m_pControlSocket->ContinueConnect();
					}
					(*iter)->Wait();
					(*iter)->Delete();
					delete *iter;					
				}
			}
			m_HostResolverThreads.clear();
			m_HostResolverThreads = remaining;
		}
		break;
	case engineTransferEnd:
		if (m_pControlSocket)
			m_pControlSocket->TransferEnd(event.data);
	default:
		break;
	}
}

bool CFileZillaEngine::SendEvent(enum EngineNotificationType eventType, int data /*=0*/)
{
	wxFzEngineEvent evt(wxID_ANY, eventType, data);
	wxPostEvent(this, evt);
	return true;
}

COptionsBase *CFileZillaEngine::GetOptions() const
{
	return m_pOptions;
}

int CFileZillaEngine::FileTransfer(const CFileTransferCommand &command)
{
	if (!IsConnected())
		return FZ_REPLY_NOTCONNECTED;

	if (IsBusy())
		return FZ_REPLY_BUSY;

	m_pCurrentCommand = command.Clone();
	return m_pControlSocket->FileTransfer(command.GetLocalFile(), command.GetRemotePath(), command.GetRemoteFile(), command.Download());
}