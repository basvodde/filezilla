#ifndef __LOCALTREEVIEW_H__
#define __LOCALTREEVIEW_H__

#include "systemimagelist.h"

class CState;
class CQueueView;

class CLocalTreeView : public wxTreeCtrl, CSystemImageList
{
public:
	CLocalTreeView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueueView);
	virtual ~CLocalTreeView();

	void SetDir(wxString localDir);
	void Refresh();

#ifdef __WXMSW__
	bool DisplayDrives();
#endif

protected:
	virtual int OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2);
	
	wxTreeItemId GetNearestParent(wxString& localDir);
	wxTreeItemId GetSubdir(wxTreeItemId parent, const wxString& subDir);
	void DisplayDir(wxTreeItemId parent, const wxString& dirname);
	bool HasSubdir(const wxString& dirname);
	wxTreeItemId MakeSubdirs(wxTreeItemId parent, wxString dirname, wxString subDir);
	wxString m_currentDir;

	DECLARE_EVENT_TABLE()
	void OnItemExpanding(wxTreeEvent& event);
	void OnSelectionChanged(wxTreeEvent& event);

	wxString GetDirFromItem(wxTreeItemId item);

	CState* m_pState;
	CQueueView* m_pQueueView;

	bool m_setSelection;
};

#endif
