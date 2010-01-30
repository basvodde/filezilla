#ifndef __TOOLBAR_H__
#define __TOOLBAR_H__

#include "option_change_event_handler.h"
#include "state.h"

class CMainFrame;

class CToolBar : public wxToolBar, public CStateEventHandler, public COptionChangeEventHandler
{
public:
	CToolBar();

	void UpdateToolbarState();

	static CToolBar* Load(CMainFrame* pMainFrame);

protected:
#if defined(EVT_TOOL_DROPDOWN) && defined(__WXMSW__)
	void MakeDropdownTool(int id);
#endif

	virtual void OnStateChange(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2);
	virtual void OnOptionChanged(int option);

	CMainFrame* m_pMainFrame;

	DECLARE_DYNAMIC_CLASS(CToolBar);
};

#endif //__TOOLBAR_H__
