#ifndef __OPTION_CHANGE_EVENT_HANDLER_H__
#define __OPTION_CHANGE_EVENT_HANDLER_H__

#include "Options.h"
#include <set>

class COptionChangeEventHandler
{
	friend class COptions;

public:
	COptionChangeEventHandler();
	virtual ~COptionChangeEventHandler();

	void RegisterOption(int option);
	void UnregisterOption(int option);

protected:
	virtual void OnOptionChanged(int option) = 0;

private:
	std::set<int> m_handled_options;

	static void DoNotify(int option);
	static void UnregisterAll();
	
	static std::set<COptionChangeEventHandler*> m_handlers[OPTIONS_NUM];
	static std::vector<int> m_queuedNotifications;
};

#endif //__OPTION_CHANGE_EVENT_HANDLER_H__
