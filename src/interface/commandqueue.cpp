#include "FileZilla.h"
#include "commandqueue.h"

CCommandQueue::CCommandQueue(CFileZillaEngine *pEngine)
{
	m_pEngine = pEngine;
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
			
			delete pCommand;
			m_CommandList.pop_front();
		}
	}
}

void CCommandQueue::Cancel()
{
	if (m_CommandList.empty())
		return;
	
	std::list<CCommand *>::iterator iter = m_CommandList.begin();
	CCommand *pCommand = *(iter++);

	for (; iter != m_CommandList.end(); iter++)
	{
		delete *iter;
	}
	m_CommandList.clear();
	m_CommandList.push_back(pCommand);
	if (m_pEngine)
		m_pEngine->Command(CCancelCommand());
}

void CCommandQueue::Finish(CNotification *pNotification)
{
	if (!m_CommandList.empty())
	{
		delete m_CommandList.front();
		m_CommandList.pop_front();
	}
	ProcessNextCommand();
}