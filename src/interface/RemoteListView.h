#ifndef __REMOTELISTVIEW_H__
#define __REMOTELISTVIEW_H__

#include "systemimagelist.h"
#include "state.h"
#include "listingcomparison.h"

class CQueueView;
class CChmodDialog;
class CInfoText;
class CRemoteListViewDropTarget;

class CRemoteListView : public wxListCtrl, CSystemImageList, CStateEventHandler, public CComparableListing
{
	friend class CRemoteListViewDropTarget;
public:
	CRemoteListView(wxWindow* parent, wxWindowID id, CState* pState, CQueueView* pQueue);
	virtual ~CRemoteListView();

	void ListingFailed();

	bool DownloadDroppedFiles(const CRemoteDataObject* pRemoteDataObject, wxString path, bool queueOnly);

	void InitDateFormat();

	virtual bool CanStartComparison(wxString* pError);
	virtual void StartComparison();
	virtual bool GetNextFile(wxString& name, bool &dir, wxLongLong &size);
	virtual void CompareAddFile(t_fileEntryFlags flags);
	virtual void FinishComparison();

protected:
	// Clears all selections and returns the list of items that were selected
	std::list<wxString> RememberSelectedItems(wxString& focused);

	// Select a list of items based in their names.
	// Sort order may not change between call to RememberSelectedItems and
	// ReselectItems
	void ReselectItems(std::list<wxString>& selectedNames, wxString focused);


	// Declared const due to design error in wxWidgets.
	// Won't be fixed since a fix would break backwards compatibility
	// Both functions use a const_cast<CLocalListView *>(this) and modify
	// the instance.
	virtual wxString OnGetItemText(long item, long column) const;
	virtual wxListItemAttr* OnGetItemAttr(long item) const;
	virtual int OnGetItemImage(long item) const;

	int FindItemWithPrefix(const wxString& prefix, int start);

public:
	struct t_fileData
	{
		int icon;
		wxString fileType;
		t_fileEntryFlags flags;
	};

	wxString GetType(wxString name, bool dir);

protected:
	bool IsItemValid(unsigned int item) const;
	int GetItemIndex(unsigned int item) const;

	void SortList(int column = -1, int direction = -1);
	void SortList_UpdateSelections(bool* selections, int focus);

	virtual void OnStateChange(unsigned int event, const wxString& data);
	void ApplyCurrentFilter();
	void SetDirectoryListing(const CDirectoryListing *pDirectoryListing, bool modified = false);
	bool UpdateDirectoryListing(const CDirectoryListing *pDirectoryListing);
	void UpdateDirectoryListing_Removed(const CDirectoryListing *pDirectoryListing);

#ifdef __WXDEBUG__
	void ValidateIndexMapping();
#endif

	const CDirectoryListing *m_pDirectoryListing;
	std::vector<t_fileData> m_fileData;

	std::vector<unsigned int> m_indexMapping;
	std::vector<unsigned int> m_originalIndexMapping; // m_originalIndexMapping will only be set on comparisons
	std::map<wxString, wxString> m_fileTypeMap;

	int m_comparisonIndex;

	// Caller is responsible to check selection is valid!
	void TransferSelectedFiles(const wxString& localDir, bool queueOnly);

	// Cache icon for directories, no need to calculate it multiple times
	int m_dirIcon;

#ifdef __WXMSW__
	wxImageListEx* m_pHeaderImageList;
#endif

	CQueueView* m_pQueue;

	int m_sortColumn;
	int m_sortDirection;

	wxDateTime m_lastKeyPress;
	wxString m_prefix;

	CInfoText* m_pInfoText;
	void RepositionInfoText();
	void SetInfoText(const wxString& text);

	wxDropTarget* m_pDropTarget;
	int m_dropTarget;

	wxString m_dateFormat;
	wxString m_timeFormat;

	DECLARE_EVENT_TABLE()
	void OnItemActivated(wxListEvent &event);
	void OnColumnClicked(wxListEvent &event);
	void OnContextMenu(wxContextMenuEvent& event);
	void OnMenuDownload(wxCommandEvent& event);
	void OnMenuMkdir(wxCommandEvent& event);
	void OnMenuDelete(wxCommandEvent& event);
	void OnMenuRename(wxCommandEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnBeginLabelEdit(wxListEvent& event);
	void OnEndLabelEdit(wxListEvent& event);
	void OnMenuChmod(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnBeginDrag(wxListEvent& event);
	void OnMenuEdit(wxCommandEvent& event);
};

#endif
