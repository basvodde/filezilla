#ifndef __QUEUEVIEW_SUCCESSFUL_H__
#define __QUEUEVIEW_SUCCESSFUL_H__

#include "queueview_failed.h"

class CQueueViewSuccessful : public CQueueViewFailed
{
public:
	CQueueViewSuccessful(CQueue* parent, int index);

	bool AutoClear() const { return m_autoClear; }

protected:

	bool m_autoClear;

	DECLARE_EVENT_TABLE();
	void OnContextMenu(wxContextMenuEvent& event);
};

#endif //__QUEUEVIEW_SUCCESSFUL_H__
