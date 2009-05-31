#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "filter_conditions_dialog.h"
#include "state.h"

class CWindowStateManager;
class CSearchDialogFileList;
class CSearchDialog : protected CFilterConditionsDialog, public CStateEventHandler
{
	friend class CSearchDialogFileList;
public:
	CSearchDialog(wxWindow* parent, CState* pState);
	virtual ~CSearchDialog();

	bool Load();
	void Run();

protected:
	void ProcessDirectoryListing();

	void SetCtrlState();

	wxWindow* m_parent;
	CSearchDialogFileList *m_results;

	virtual void OnStateChange(enum t_statechange_notifications notification, const wxString& data);

	CWindowStateManager* m_pWindowStateManager;

	CFilter m_search_filter;

	DECLARE_EVENT_TABLE()
	void OnSearch(wxCommandEvent& event);
	void OnStop(wxCommandEvent& event);
};

#endif //__SEARCH_H__
