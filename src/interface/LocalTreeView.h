#ifndef __LOCALTREEVIEW_H__
#define __LOCALTREEVIEW_H__

#include "systemimagelist.h"
#include "state.h"

class CQueueView;

class CLocalTreeView : public wxTreeCtrl, CSystemImageList, CStateEventHandler
{
	DECLARE_CLASS(CLocalTreeView)

	friend class CLocalTreeViewDropTarget;

public:
	CLocalTreeView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueueView);
	virtual ~CLocalTreeView();

protected:
	virtual void OnStateChange(unsigned int event);

	void SetDir(wxString localDir);
	void Refresh();

#ifdef __WXMSW__
	bool DisplayDrives();
#endif

	virtual int OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2);
	
	wxTreeItemId GetNearestParent(wxString& localDir);
	wxTreeItemId GetSubdir(wxTreeItemId parent, const wxString& subDir);
	void DisplayDir(wxTreeItemId parent, const wxString& dirname, const wxString& filterException = _T(""));
	bool HasSubdir(const wxString& dirname);
	wxTreeItemId MakeSubdirs(wxTreeItemId parent, wxString dirname, wxString subDir);
	wxString m_currentDir;

	DECLARE_EVENT_TABLE()
	void OnItemExpanding(wxTreeEvent& event);
	void OnSelectionChanged(wxTreeEvent& event);
	void OnBeginDrag(wxTreeEvent& event);

	wxString GetDirFromItem(wxTreeItemId item);

	CQueueView* m_pQueueView;

	bool m_setSelection;

	wxTreeItemId m_dropHighlight;
};

#endif
