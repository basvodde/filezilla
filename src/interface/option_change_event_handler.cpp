#include "filezilla.h"
#include "option_change_event_handler.h"

std::set<COptionChangeEventHandler*> COptionChangeEventHandler::m_handlers[OPTIONS_NUM];
std::vector<int> COptionChangeEventHandler::m_queuedNotifications;

COptionChangeEventHandler::COptionChangeEventHandler()
{
}

COptionChangeEventHandler::~COptionChangeEventHandler()
{
	for (std::set<int>::const_iterator iter = m_handled_options.begin(); iter != m_handled_options.end(); iter++)
		m_handlers[*iter].erase(this);
}

void COptionChangeEventHandler::RegisterOption(int option)
{
	if (option < 0 || option >= OPTIONS_NUM)
		return;

	m_handled_options.insert(option);
	m_handlers[option].insert(this);
}

void COptionChangeEventHandler::UnregisterOption(int option)
{
	if (m_handled_options.erase(option))
		m_handlers[option].erase(this);
}

void COptionChangeEventHandler::UnregisterAll()
{
	for (int i = 0; i < OPTIONS_NUM; i++)
	{
		for (std::set<COptionChangeEventHandler*>::iterator iter = m_handlers[i].begin(); iter != m_handlers[i].end(); iter++)
		{
			(*iter)->m_handled_options.clear();
		}
		m_handlers[i].clear();
	}
}

void COptionChangeEventHandler::DoNotify(int option)
{
	if (option < 0 || option >= OPTIONS_NUM)
		return;

	for (std::set<COptionChangeEventHandler*>::iterator iter = m_handlers[option].begin(); iter != m_handlers[option].end(); iter++)
	{
		(*iter)->OnOptionChanged(option);
	}
}
