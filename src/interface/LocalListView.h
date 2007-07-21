#ifndef __LOCALLISTVIEW_H__
#define __LOCALLISTVIEW_H__

#include "systemimagelist.h"
#include "state.h"

class CQueueView;
class CLocalListViewDropTarget;

class CLocalListView : public wxListCtrl, CSystemImageList, CStateEventHandler
{
	friend class CLocalListViewDropTarget;

public:
	CLocalListView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueue);
	virtual ~CLocalListView();

protected:
	void OnStateChange(unsigned int event);
	bool DisplayDir(wxString dirname);
	void ApplyCurrentFilter();

	// Declared const due to design error in wxWidgets.
	// Won't be fixed since a fix would break backwards compatibility
	// Both functions use a const_cast<CLocalListView *>(this) and modify
	// the instance.
	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemImage(long item) const;

	// Clears all selections and returns the list of items that were selected
	std::list<wxString> RememberSelectedItems(wxString& focused);

	// Select a list of items based in their names.
	// Sort order may not change between call to RememberSelectedItems and
	// ReselectItems
	void ReselectItems(const std::list<wxString>& selectedNames, wxString focused);

#ifdef __WXMSW__
	void DisplayDrives();
	void DisplayShares(wxString computer);
#endif

public:
	wxString GetType(wxString name, bool dir);

	struct t_fileData
	{
		wxString name;
		bool dir;
		int icon;
		wxLongLong size;
		wxString fileType;
		bool hasTime;
		wxDateTime lastModified;
	};

protected:
	bool IsItemValid(unsigned int item) const;
	t_fileData *GetData(unsigned int item);

	void SortList(int column = -1, int direction = -1);
	void SortList_UpdateSelections(bool* selections, int focus);

	int FindItemWithPrefix(const wxString& prefix, int start);

	wxString m_dir;

	std::vector<t_fileData> m_fileData;
	std::vector<unsigned int> m_indexMapping;
	std::map<wxString, wxString> m_fileTypeMap;

#ifdef __WXMSW__
	wxImageListEx *m_pHeaderImageList;
#endif

	CQueueView *m_pQueue;

	int m_sortColumn;
	int m_sortDirection;

	wxDateTime m_lastKeyPress;
	wxString m_prefix;

	wxDropTarget* m_pDropTarget;
	int m_dropTarget;

	// Event handlers
	DECLARE_EVENT_TABLE();
	void OnItemActivated(wxListEvent& event);
	void OnColumnClicked(wxListEvent& event);
	void OnContextMenu(wxContextMenuEvent& event);
	void OnMenuUpload(wxCommandEvent& event);
	void OnMenuMkdir(wxCommandEvent& event);
	void OnMenuDelete(wxCommandEvent& event);
	void OnMenuRename(wxCommandEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnBeginLabelEdit(wxListEvent& event);
	void OnEndLabelEdit(wxListEvent& event);
	void OnBeginDrag(wxListEvent& event);
};

#endif
