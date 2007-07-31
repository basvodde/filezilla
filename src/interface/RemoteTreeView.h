#ifndef __REMOTETREEVIEW_H__
#define __REMOTETREEVIEW_H__

#include "systemimagelist.h"
#include "state.h"
#include "filter.h"

class CQueueView;
class CRemoteTreeView : public wxTreeCtrl, CSystemImageList, CStateEventHandler
{
	DECLARE_CLASS(CRemoteTreeView)

	friend class CRemoteTreeViewDropTarget;

public:
	CRemoteTreeView(wxWindow* parent, wxWindowID id, CState* pState, CQueueView* pQueue);
	virtual ~CRemoteTreeView();

protected:
	wxTreeItemId MakeParent(CServerPath path, bool select);
	void SetDirectoryListing(const CDirectoryListing* pListing, bool modified);
	virtual void OnStateChange(unsigned int event, const wxString& data);

	void DisplayItem(wxTreeItemId parent, const CDirectoryListing& listing);
	void RefreshItem(wxTreeItemId parent, const CDirectoryListing& listing);

	void SetItemImages(wxTreeItemId item, bool unknown);

	bool HasSubdirs(const CDirectoryListing& listing, const CFilterDialog& filter);

	virtual int OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2);

	CServerPath GetPathFromItem(const wxTreeItemId& item) const;

	CQueueView* m_pQueue;
	const CDirectoryListing* m_pDirectoryListing;

	void CreateImageList();
	wxBitmap CreateIcon(int index, const wxString& overlay = _T(""));
	wxImageList* m_pImageList;

	// Set to true in SetDirectoryListing.
	// Used to suspends event processing in OnItemExpanding for example
	bool m_busy;

	wxTreeItemId m_ExpandAfterList;

	wxTreeItemId m_dropHighlight;

	DECLARE_EVENT_TABLE()
	void OnItemExpanding(wxTreeEvent& event);
	void OnSelectionChanged(wxTreeEvent& event);
	void OnItemActivated(wxTreeEvent& event);
	void OnBeginDrag(wxTreeEvent& event);
#ifndef __WXMSW__
	void OnKeyDown(wxKeyEvent& event);
#endif
	void OnContextMenu(wxTreeEvent& event);
	void OnMenuChmod(wxCommandEvent& event);
	void OnMenuDownload(wxCommandEvent& event);

	wxTreeItemId m_contextMenuItem;
};

#endif
