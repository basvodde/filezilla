#ifndef __FILELISTCTRL_H__
#define __FILELISTCTRL_H__

#include "listctrlex.h"
#include "systemimagelist.h"
#include "listingcomparison.h"

class CQueueView;
class CFileListCtrl_SortComparisonObject;
class CState;
class CFilelistStatusBar;

class CGenericFileData
{
public:
	int icon;
	wxString fileType;

	// t_fileEntryFlags is defined in listingcomparison.h as it will be used for
	// both local and remote listings
	CComparableListing::t_fileEntryFlags flags;
};

class CListViewSort
{
public:
	enum DirSortMode
	{
		dirsort_ontop,
		dirsort_onbottom,
		dirsort_inline
	};

	virtual bool operator()(int a, int b) const = 0;
	virtual ~CListViewSort() { } // Without this empty destructor GCC complains
};

template<class CFileData> class CFileListCtrl : public wxListCtrlEx, public CComparableListing
{
public:
	CFileListCtrl(wxWindow* pParent, CState *pState, CQueueView *pQueue);
	virtual ~CFileListCtrl();

	class CSortComparisonObject : public std::binary_function<int,int,bool>
	{
	public:
		CSortComparisonObject(CListViewSort* pObject)
			: m_pObject(pObject)
		{
		}

		void Destroy()
		{
			delete m_pObject;
		}

		inline bool operator()(int a, int b)
		{
			return m_pObject->operator ()(a, b);
		}
	protected:
		CListViewSort* m_pObject;
	};

	void SetFilelistStatusBar(CFilelistStatusBar* pFilelistStatusBar) { m_pFilelistStatusBar = pFilelistStatusBar; }

protected:
	CQueueView *m_pQueue;

	std::vector<CFileData> m_fileData;
	std::vector<unsigned int> m_indexMapping;
	std::vector<unsigned int> m_originalIndexMapping; // m_originalIndexMapping will only be set on comparisons

	virtual bool ItemIsDir(int index) const = 0;
	virtual wxLongLong ItemGetSize(int index) const = 0;

	std::map<wxString, wxString> m_fileTypeMap;

#ifdef __WXMSW__
	// Has to be set after settings the image list for the items
	void InitHeaderImageList();
	wxImageListEx *m_pHeaderImageList;
#endif

	// The .. item
	bool m_hasParent;

	int m_sortColumn;
	int m_sortDirection;

	void InitSort(int optionID); // Has to be called after initializing columns
	void SortList(int column = -1, int direction = -1, bool updateSelections = true);
	enum CListViewSort::DirSortMode GetDirSortMode();
	virtual CSortComparisonObject GetSortComparisonObject() = 0;

	// An empty path denotes a virtual file
	wxString GetType(wxString name, bool dir, const wxString& path = _T(""));

	// Comparison related
	virtual void ScrollTopItem(int item);
	virtual void OnPostScroll();
	virtual void OnExitComparisonMode();
	virtual void CompareAddFile(t_fileEntryFlags flags);

	int m_comparisonIndex;

	// Remembers which non-fill items are selected if enabling/disabling comparison.
	// Exploit fact that sort order doesn't change -> O(n)
	void ComparisonRememberSelections();
	void ComparisonRestoreSelections();
	std::list<int> m_comparisonSelections;

	CFilelistStatusBar* m_pFilelistStatusBar;

#ifndef __WXMSW__
	// Generic wxListCtrl does not support wxLIST_STATE_DROPHILITED, emulate it
	wxListItemAttr m_dropHighlightAttribute;
#endif

	void SetSelection(int item, bool select);

private:
	void SortList_UpdateSelections(bool* selections, int focus);

	// If this is set to true, don't process selection changed events
	bool m_insideSetSelection; 

#ifdef __WXMSW__
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	WNDPROC m_prevWndproc;
	static std::map<HWND, CFileListCtrl<CFileData>*> m_hwnd_map;
#endif

	DECLARE_EVENT_TABLE()
	void OnColumnClicked(wxListEvent &event);
	void OnColumnRightClicked(wxListEvent& event);
	void OnItemSelected(wxListEvent& event);
	void OnItemDeselected(wxListEvent& event);
};

#ifdef FILELISTCTRL_INCLUDE_TEMPLATE_DEFINITION
#include "filelistctrl.cpp"
#endif

#endif //__FILELISTCTRL_H__
