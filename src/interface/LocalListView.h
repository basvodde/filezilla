#ifndef __LOCALLISTVIEW_H__
#define __LOCALLISTVIEW_H__

#include "systemimagelist.h"
#include "state.h"
#include "listingcomparison.h"

class CQueueView;
class CLocalListViewDropTarget;
class CLocalListViewSortObject;

class CLocalListView : public wxListCtrl, CSystemImageList, CStateEventHandler, public CComparableListing
{
	friend class CLocalListViewDropTarget;

public:
	CLocalListView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueue);
	virtual ~CLocalListView();

protected:
	void OnStateChange(unsigned int event, const wxString& data);
	bool DisplayDir(wxString dirname);
	void ApplyCurrentFilter();

	// Declared const due to design error in wxWidgets.
	// Won't be fixed since a fix would break backwards compatibility
	// Both functions use a const_cast<CLocalListView *>(this) and modify
	// the instance.
	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemImage(long item) const;
	virtual wxListItemAttr* OnGetItemAttr(long item) const;

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

		// t_fileEntryFlags is defined in state.h as it will be used for
		// both local and remote listings
		t_fileEntryFlags flags;
	};

	void InitDateFormat();

	virtual bool CanStartComparison(wxString* pError);
	virtual void StartComparison();
	virtual bool GetNextFile(wxString& name, bool &dir, wxLongLong &size);
	virtual void CompareAddFile(t_fileEntryFlags flags);
	virtual void FinishComparison();
	virtual void ScrollTopItem(int item);

protected:
	bool IsItemValid(unsigned int item) const;
	t_fileData *GetData(unsigned int item);

	void SortList(int column = -1, int direction = -1);
	void SortList_UpdateSelections(bool* selections, int focus);
	CLocalListViewSortObject GetComparisonObject();

	int FindItemWithPrefix(const wxString& prefix, int start);

	void RefreshFile(const wxString& file);

	wxString m_dir;

	std::vector<t_fileData> m_fileData;
	std::vector<unsigned int> m_indexMapping;
	std::vector<unsigned int> m_originalIndexMapping;
	std::map<wxString, wxString> m_fileTypeMap;

	int m_comparisonIndex;

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

	bool m_hasParent;

	wxString m_dateFormat;

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
	void OnPostScroll(wxCommandEvent& event);
	void OnScrollEvent(wxScrollWinEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnSelectionChanged(wxListEvent& event);
};

#endif
