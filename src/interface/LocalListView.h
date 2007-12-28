#ifndef __LOCALLISTVIEW_H__
#define __LOCALLISTVIEW_H__

#include "filelistctrl.h"
#include "state.h"

class CQueueView;
class CLocalListViewDropTarget;

class CLocalFileData : public CGenericFileData
{
public:
	wxString name;
	bool dir;
	wxLongLong size;
	bool hasTime;
	wxDateTime lastModified;
};

class CLocalListView : public CFileListCtrl<CLocalFileData>, CSystemImageList, CStateEventHandler
{
	friend class CLocalListViewDropTarget;
	friend class CLocalListViewSortType;

public:
	CLocalListView(wxWindow* parent, CState *pState, CQueueView *pQueue);
	virtual ~CLocalListView();

protected:
	void OnStateChange(unsigned int event, const wxString& data);
	bool DisplayDir(wxString dirname);
	void ApplyCurrentFilter();

	// Declared const due to design error in wxWidgets.
	// Won't be fixed since a fix would break backwards compatibility
	// Both functions use a const_cast<CLocalListView *>(this) and modify
	// the instance.
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
	void InitDateFormat();

	virtual bool CanStartComparison(wxString* pError);
	virtual void StartComparison();
	virtual bool GetNextFile(wxString& name, bool &dir, wxLongLong &size, wxDateTime& date, bool &hasTime);
	virtual void FinishComparison();

protected:
	virtual wxString GetItemText(int item, unsigned int column);

	bool IsItemValid(unsigned int item) const;
	CLocalFileData *GetData(unsigned int item);

	virtual CSortComparisonObject GetSortComparisonObject();

	void RefreshFile(const wxString& file);

	wxString m_dir;

	wxDropTarget* m_pDropTarget;
	int m_dropTarget;

	wxString m_dateFormat;

	// Event handlers
	DECLARE_EVENT_TABLE();
	void OnItemActivated(wxListEvent& event);
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
