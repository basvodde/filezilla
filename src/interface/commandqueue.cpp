#include "FileZilla.h"
#include "commandqueue.h"
#include "Mainfrm.h"
#include "state.h"
#include "recursive_operation.h"
#include "loginmanager.h"
#include "queue.h"

DEFINE_EVENT_TYPE(fzEVT_GRANTEXCLUSIVEENGINEACCESS)

CCommandQueue::CCommandQueue(CFileZillaEngine *pEngine, CMainFrame* pMainFrame)
{
	m_pEngine = pEngine;
	m_pMainFrame = pMainFrame;
	m_exclusiveEngineRequest = false;
	m_exclusiveEngineLock = false;
	m_requestId = 0;
}

CCommandQueue::~CCommandQueue()
{
	for (std::list<CCommand *>::iterator iter = m_CommandList.begin(); iter != m_CommandList.end(); iter++)
		delete *iter;
}

bool CCommandQueue::Idle() const
{
	return m_CommandList.empty() && !m_exclusiveEngineLock;
}

void CCommandQueue::ProcessCommand(CCommand *pCommand)
{
	m_CommandList.push_back(pCommand);
	if (m_CommandList.size() == 1)
		ProcessNextCommand();
}

void CCommandQueue::ProcessNextCommand()
{
	if (m_exclusiveEngineLock)
		return;

	if (m_pEngine->IsBusy())
		return;

	while (!m_CommandList.empty())
	{
		CCommand *pCommand = m_CommandList.front();

		int res = m_pEngine->Command(*pCommand);
		
		if (pCommand->GetId() == cmd_connect)
		{
			if (res == FZ_REPLY_WOULDBLOCK || res == FZ_REPLY_OK)
			{
				const CConnectCommand* pConnectCommand = (const CConnectCommand *)pCommand;
				const CServer& server = pConnectCommand->GetServer();
				m_pMainFrame->GetState()->SetServer(&server);
			}
		}
		else if (pCommand->GetId() == cmd_disconnect)
		{
			m_pMainFrame->GetState()->SetServer(0);
		}

		if (res == FZ_REPLY_WOULDBLOCK)
			break;
		else if (res == FZ_REPLY_OK)
		{
			delete pCommand;
			m_CommandList.pop_front();
			continue;
		}
		else if (res == FZ_REPLY_ALREADYCONNECTED)
		{
			m_pMainFrame->GetState()->SetServer(0);
			res = m_pEngine->Command(CDisconnectCommand());
			if (res == FZ_REPLY_WOULDBLOCK)
			{
				m_CommandList.push_front(new CDisconnectCommand);
				break;
			}
			else if (res != FZ_REPLY_OK)
			{
				wxBell();
				delete pCommand;
				m_CommandList.pop_front();
				continue;
			}
		}
		else
		{
			if ((res & FZ_REPLY_NOTCONNECTED) == FZ_REPLY_NOTCONNECTED)
					m_pMainFrame->GetState()->SetServer(0);

			wxBell();
			
			// Let the remote list view know if a LIST command failed,
			// so that it may issue the next command in recursive operations.
			if (pCommand->GetId() == cmd_list)
				m_pMainFrame->GetState()->GetRecursiveOperationHandler()->ListingFailed();

			m_CommandList.pop_front();
			delete pCommand;
		}
	}

	if (m_CommandList.empty() && m_exclusiveEngineRequest)
	{
		GrantExclusiveEngineRequest();
	}
}

bool CCommandQueue::Cancel()
{
	if (m_exclusiveEngineLock)
		return false;

	if (m_CommandList.empty())
		return true;
	
	std::list<CCommand *>::iterator iter = m_CommandList.begin();
	CCommand *pCommand = *(iter++);

	for (; iter != m_CommandList.end(); iter++)
		delete *iter;

	m_CommandList.clear();
	m_CommandList.push_back(pCommand);

	if (!m_pEngine)
	{
		delete pCommand;
		m_CommandList.clear();
		return true;
	}

	int res = m_pEngine->Command(CCancelCommand());
	if (res == FZ_REPLY_WOULDBLOCK)
		return false;
	else
		return true;
}

void CCommandQueue::Finish(COperationNotification *pNotification)
{
	if (pNotification->nReplyCode & FZ_REPLY_DISCONNECTED)
	{
		if (pNotification->commandId == cmd_none && !m_CommandList.empty())
		{
			// Pending event, has no relevance during command execution
			delete pNotification;
			return;
		}
		if (pNotification->nReplyCode & FZ_REPLY_PASSWORDFAILED)
			CLoginManager::Get().CachedPasswordFailed(*m_pMainFrame->GetState()->GetServer());
		m_pMainFrame->GetState()->SetServer(0);
	}

	if (m_exclusiveEngineLock)
	{
		m_pMainFrame->GetQueue()->ProcessNotification(pNotification);
		return;
	}

	if (m_CommandList.empty())
	{
		delete pNotification;
		return;
	}

	CCommand* pCommand = m_CommandList.front();

	// Let the remote list view know if a LIST command failed,
	// so that it may issue the next command in recursive operations.
	if (pCommand->GetId() == cmd_list && pNotification->nReplyCode != FZ_REPLY_OK)
		m_pMainFrame->GetState()->GetRecursiveOperationHandler()->ListingFailed();
	
	delete m_CommandList.front();
	m_CommandList.pop_front();
	
	delete pNotification;

	ProcessNextCommand();
}

void CCommandQueue::RequestExclusiveEngine(bool requestExclusive)
{
	wxASSERT(!m_exclusiveEngineLock || !requestExclusive);

	if (!m_exclusiveEngineRequest && requestExclusive)
	{
		m_requestId++;
		if (m_requestId < 0)
			m_requestId = 0;
		if (m_CommandList.empty())
		{
			GrantExclusiveEngineRequest();
			return;
		}
	}
	if (!requestExclusive)
		m_exclusiveEngineLock = false;
	m_exclusiveEngineRequest = requestExclusive;
}

void CCommandQueue::GrantExclusiveEngineRequest()
{
	wxASSERT(!m_exclusiveEngineLock);
	m_exclusiveEngineLock = true;
	m_exclusiveEngineRequest = false;

	wxCommandEvent evt(fzEVT_GRANTEXCLUSIVEENGINEACCESS);
	evt.SetId(m_requestId);
	m_pMainFrame->GetQueue()->AddPendingEvent(evt);
}

CFileZillaEngine* CCommandQueue::GetEngineExclusive(int requestId)
{
	if (!m_exclusiveEngineLock)
		return 0;

	if (requestId != m_requestId)
		return 0;

	return m_pEngine;
}


void CCommandQueue::ReleaseEngine()
{
	m_exclusiveEngineLock = false;

	ProcessNextCommand();
}
