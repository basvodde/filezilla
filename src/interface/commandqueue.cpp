#include "FileZilla.h"
#include "commandqueue.h"
#include "Mainfrm.h"
#include "state.h"
#include "RemoteListView.h"
#include "loginmanager.h"

CCommandQueue::CCommandQueue(CFileZillaEngine *pEngine, CMainFrame* pMainFrame)
{
	m_pEngine = pEngine;
	m_pMainFrame = pMainFrame;
}

CCommandQueue::~CCommandQueue()
{
	for (std::list<CCommand *>::iterator iter = m_CommandList.begin(); iter != m_CommandList.end(); iter++)
		delete *iter;
}

bool CCommandQueue::Idle() const
{
	return m_CommandList.empty();
}

void CCommandQueue::ProcessCommand(CCommand *pCommand)
{
	m_CommandList.push_back(pCommand);
	if (m_CommandList.size() == 1)
		ProcessNextCommand();
}

void CCommandQueue::ProcessNextCommand()
{
	if (m_pEngine->IsBusy())
		return;

	while (!m_CommandList.empty())
	{
		CCommand *pCommand = m_CommandList.front();

		int res = m_pEngine->Command(*pCommand);
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
			wxBell();
			
			// Let the remote list view know if a LIST command failed,
			// so that it may issue the next command in recursive operations.
			if (pCommand->GetId() == cmd_list)
				m_pMainFrame->GetRemoteListView()->ListingFailed();

			m_CommandList.pop_front();
			delete pCommand;
		}
	}
}

bool CCommandQueue::Cancel()
{
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
		if (pNotification->nReplyCode & FZ_REPLY_PASSWORDFAILED)
			CLoginManager::Get().CachedPasswordFailed(*m_pMainFrame->GetState()->GetServer());
		m_pMainFrame->GetState()->SetServer(0);
	}

	if (m_CommandList.empty())
		return;

	CCommand* pCommand = m_CommandList.front();

	// Let the remote list view know if a LIST command failed,
	// so that it may issue the next command in recursive operations.
	if (pCommand->GetId() == cmd_list && pNotification->nReplyCode != FZ_REPLY_OK)
		m_pMainFrame->GetRemoteListView()->ListingFailed();
	
	delete m_CommandList.front();
	m_CommandList.pop_front();
	
	ProcessNextCommand();
}
