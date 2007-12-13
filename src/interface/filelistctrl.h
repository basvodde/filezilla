#ifndef __FILELISTCTRL_H__
#define __FILELISTCTRL_H__

#include "listctrlex.h"
#include "systemimagelist.h"
#include "listingcomparison.h"

class CQueueView;
class CFileListCtrl_SortComparisonObject;
class CState;

class CGenericFileData
{
public:
	int icon;
	wxString fileType;

	// t_fileEntryFlags is defined in listingcomparison.h as it will be used for
	// both local and remote listings
	CComparableListing::t_fileEntryFlags flags;
};

template<class CFileData> class CFileListCtrl : public wxListCtrlEx
{
public:
	CFileListCtrl(wxWindow* pParent, CState *pState, CQueueView *pQueue);
	virtual ~CFileListCtrl();

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
	};

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

protected:
	CQueueView *m_pQueue;

	std::vector<CFileData> m_fileData;
	std::vector<unsigned int> m_indexMapping;

#ifdef __WXMSW__
	// Has to be set after settings the image list for the items
	void InitHeaderImageList();
	wxImageListEx *m_pHeaderImageList;
#endif

	// The .. item
	bool m_hasParent;

	int m_sortColumn;
	int m_sortDirection;

	void SortList(int column = -1, int direction = -1, bool updateSelections = true);
	CListViewSort::DirSortMode GetDirSortMode();
	virtual CSortComparisonObject GetSortComparisonObject() = 0;

private:
	void SortList_UpdateSelections(bool* selections, int focus);
};

#ifdef FILELISTCTRL_INCLUDE_TEMPLATE_DEFINITION
#include "filelistctrl.cpp"
#endif

#endif //__FILELISTCTRL_H__
