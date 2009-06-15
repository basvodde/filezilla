#ifndef __LOCALTREEVIEW_H__
#define __LOCALTREEVIEW_H__

#include "systemimagelist.h"
#include "state.h"

class CQueueView;

#ifdef __WXMSW__
class CVolumeDescriptionEnumeratorThread;
#endif

class CLocalTreeView : public wxTreeCtrl, CSystemImageList, CStateEventHandler
{
	DECLARE_CLASS(CLocalTreeView)

	friend class CLocalTreeViewDropTarget;

public:
	CLocalTreeView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueueView);
	virtual ~CLocalTreeView();

#ifdef __WXMSW__
	// React to changed drive letters
	void OnDevicechange(WPARAM wParam, LPARAM lParam);
#endif

protected:
	virtual void OnStateChange(enum t_statechange_notifications notification, const wxString& data);

	void SetDir(wxString localDir);
	void Refresh();

#ifdef __WXMSW__
	bool CreateRoot();
	bool DisplayDrives(wxTreeItemId parent);
	wxString GetSpecialFolder(int folder, int &iconIndex, int &openIconIndex);

	wxTreeItemId m_desktop;
	wxTreeItemId m_drives;
	wxTreeItemId m_documents;
#endif

	virtual int OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2);

	wxTreeItemId GetNearestParent(wxString& localDir);
	wxTreeItemId GetSubdir(wxTreeItemId parent, const wxString& subDir);
	void DisplayDir(wxTreeItemId parent, const wxString& dirname, const wxString& knownSubdir = _T(""));
	wxString HasSubdir(const wxString& dirname);
	wxTreeItemId MakeSubdirs(wxTreeItemId parent, wxString dirname, wxString subDir);
	wxString m_currentDir;

	bool CheckSubdirStatus(wxTreeItemId& item, const wxString& path);

	DECLARE_EVENT_TABLE()
	void OnItemExpanding(wxTreeEvent& event);
	void OnSelectionChanged(wxTreeEvent& event);
	void OnBeginDrag(wxTreeEvent& event);
#ifndef __WXMSW__
	void OnKeyDown(wxKeyEvent& event);
#else
	void OnVolumesEnumerated(wxCommandEvent& event);
	CVolumeDescriptionEnumeratorThread* m_pVolumeEnumeratorThread;
#endif
	void OnContextMenu(wxTreeEvent& event);
	void OnMenuUpload(wxCommandEvent& event);
	void OnMenuMkdir(wxCommandEvent& event);
	void OnMenuRename(wxCommandEvent& event);
	void OnMenuDelete(wxCommandEvent& event);
	void OnBeginLabelEdit(wxTreeEvent& event);
	void OnEndLabelEdit(wxTreeEvent& event);
	void OnChar(wxKeyEvent& event);

#ifdef __WXMSW__
	// React to changed drive letters
	wxTreeItemId AddDrive(wxChar letter);
	void RemoveDrive(wxChar letter);
#endif

	wxString GetDirFromItem(wxTreeItemId item);

	CQueueView* m_pQueueView;

	bool m_setSelection;

	wxTreeItemId m_contextMenuItem;
	wxTreeItemId m_dropHighlight;
};

#endif
