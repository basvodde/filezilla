#include "FileZilla.h"
#include "RemoteListView.h"
#include "commandqueue.h"
#include "queue.h"
#include "filezillaapp.h"
#include "inputdialog.h"
#include "chmoddialog.h"
#include "filter.h"
#include <algorithm>
#include <wx/dnd.h>
#include "dndobjects.h"
#include "Options.h"

#ifdef __WXMSW__
#include "shellapi.h"
#include "commctrl.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CRemoteListViewDropTarget : public wxDropTarget
{
public:
	CRemoteListViewDropTarget(CRemoteListView* pRemoteListView)
		: m_pRemoteListView(pRemoteListView),
		  m_pFileDataObject(new wxFileDataObject()),
		  m_pRemoteDataObject(new CRemoteDataObject()),
		  m_pDataObject(new wxDataObjectComposite())
	{
		m_pDataObject->Add(m_pRemoteDataObject, true);
		m_pDataObject->Add(m_pFileDataObject);
		SetDataObject(m_pDataObject);
	}

	void ClearDropHighlight()
	{
		const int dropTarget = m_pRemoteListView->m_dropTarget;
		if (dropTarget != -1)
		{
			m_pRemoteListView->SetItemState(dropTarget, 0, wxLIST_STATE_DROPHILITED);
			m_pRemoteListView->m_dropTarget = -1;
		}
	}
	
	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
	{
		if (def == wxDragError ||
			def == wxDragNone ||
			def == wxDragCancel)
			return def;

		if (!m_pRemoteListView->m_pDirectoryListing)
			return wxDragError;

		if (!GetData())
			return wxDragError;

		if (m_pDataObject->GetReceivedFormat() == m_pFileDataObject->GetFormat())
		{
			wxString subdir;
			int flags = 0;
			int hit = m_pRemoteListView->HitTest(wxPoint(x, y), flags, 0);
			if (hit != -1 && (flags & wxLIST_HITTEST_ONITEM))
			{
				int index = m_pRemoteListView->GetItemIndex(hit);
				if (index != -1)
				{
					if (index == (int)m_pRemoteListView->m_pDirectoryListing->GetCount())
						subdir = _T("..");
					else if ((*m_pRemoteListView->m_pDirectoryListing)[index].dir)
						subdir = (*m_pRemoteListView->m_pDirectoryListing)[index].name;
				}
			}

			m_pRemoteListView->m_pState->UploadDroppedFiles(m_pFileDataObject, subdir, false);
			return wxDragCopy;
		}
		
		// At this point it's the remote data object.

		if (m_pRemoteDataObject->GetProcessId() != (int)wxGetProcessId())
		{
			wxMessageBox(_("Drag&drop between different instances of FileZilla has not been implemented yet."));
			return wxDragNone;
		}

		if (m_pRemoteDataObject->GetServer() != *m_pRemoteListView->m_pState->GetServer())
		{
			wxMessageBox(_("Drag&drop between different servers has not been implemented yet."));
			return wxDragNone;
		}

		// Find drop directory (if it exists)
		wxString subdir;
		int flags = 0;
		int hit = m_pRemoteListView->HitTest(wxPoint(x, y), flags, 0);
		if (hit != -1 && (flags & wxLIST_HITTEST_ONITEM))
		{
			int index = m_pRemoteListView->GetItemIndex(hit);
			if (index != -1)
			{
				if (index == (int)m_pRemoteListView->m_pDirectoryListing->GetCount())
					subdir = _T("..");
				else if ((*m_pRemoteListView->m_pDirectoryListing)[index].dir)
					subdir = (*m_pRemoteListView->m_pDirectoryListing)[index].name;
			}
		}

		// Get target path
		CServerPath target = m_pRemoteListView->m_pDirectoryListing->path;
		if (subdir == _T(".."))
		{
			if (target.HasParent())
				target = target.GetParent();
		}
		else if (subdir != _T(""))
			target.AddSegment(subdir);

		// Make sure target path is valid
		if (target == m_pRemoteDataObject->GetServerPath())
		{
			wxMessageBox(_("Source and target of the drop operation are identical"));
			return wxDragNone;
		}

		const std::list<CRemoteDataObject::t_fileInfo>& files = m_pRemoteDataObject->GetFiles();
		for (std::list<CRemoteDataObject::t_fileInfo>::const_iterator iter = files.begin(); iter != files.end(); iter++)
		{
			const CRemoteDataObject::t_fileInfo& info = *iter;
			if (info.dir)
			{
				CServerPath dir = m_pRemoteDataObject->GetServerPath();
				dir.AddSegment(info.name);
				if (dir == target || dir.IsParentOf(target, false))
				{
					wxMessageBox(_("A directory cannot be dragged into one if its subdirectories."));
					return wxDragNone;
				}
			}
		}

		for (std::list<CRemoteDataObject::t_fileInfo>::const_iterator iter = files.begin(); iter != files.end(); iter++)
		{
			const CRemoteDataObject::t_fileInfo& info = *iter;
			m_pRemoteListView->m_pState->m_pCommandQueue->ProcessCommand(
				new CRenameCommand(m_pRemoteDataObject->GetServerPath(), info.name, target, info.name)
				);
		}
		m_pRemoteListView->m_pState->m_pCommandQueue->ProcessCommand(
			new CListCommand()
			);

		return wxDragNone;
	}

	virtual bool OnDrop(wxCoord x, wxCoord y)
	{
		ClearDropHighlight();

		if (!m_pRemoteListView->m_pDirectoryListing)
			return false;

		return true;
	}

	void DisplayDropHighlight(wxPoint point)
	{
		int flags;
		int hit = m_pRemoteListView->HitTest(point, flags, 0);
		if (!(flags & wxLIST_HITTEST_ONITEM))
			hit = -1;

		if (hit != -1)
		{
			int index = m_pRemoteListView->GetItemIndex(hit);
			if (index == -1)
				hit = -1;
			else if (index != (int)m_pRemoteListView->m_pDirectoryListing->GetCount())
			{
				if (!(*m_pRemoteListView->m_pDirectoryListing)[index].dir)
					hit = -1;
			}
		}
		if (hit != m_pRemoteListView->m_dropTarget)
		{
			ClearDropHighlight();
			if (hit != -1)
			{
				m_pRemoteListView->SetItemState(hit, wxLIST_STATE_DROPHILITED, wxLIST_STATE_DROPHILITED);
				m_pRemoteListView->m_dropTarget = hit;
			}
		}
	}

	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
	{
		if (def == wxDragError ||
			def == wxDragNone ||
			def == wxDragCancel)
		{
			ClearDropHighlight();
			return def;
		}

		if (!m_pRemoteListView->m_pDirectoryListing)
		{
			ClearDropHighlight();
			return wxDragNone;
		}

		DisplayDropHighlight(wxPoint(x, y));

		return wxDragCopy;
	}

	virtual void OnLeave()
	{
		ClearDropHighlight();
	}

	virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def)
	{
		return OnDragOver(x, y, def);
	}

protected:
	CRemoteListView *m_pRemoteListView;
	wxFileDataObject* m_pFileDataObject;
	CRemoteDataObject* m_pRemoteDataObject;

	wxDataObjectComposite* m_pDataObject;
};

class CInfoText : public wxWindow
{
public:
	CInfoText(wxWindow* parent, const wxString& text)
		: wxWindow(parent, wxID_ANY, wxPoint(0, 60), wxDefaultSize),
		m_text(_T("<") + text + _T(">"))
	{
		SetBackgroundColour(parent->GetBackgroundColour());
	}

	wxString m_text;

protected:
	void OnPaint(wxPaintEvent& event)
	{
		wxPaintDC paintDc(this);

		wxRect rect = GetClientRect();
		paintDc.SetFont(GetFont());

		int width, height;
		paintDc.GetTextExtent(m_text, &width, &height);
		paintDc.DrawText(m_text, rect.x + rect.GetWidth() / 2 - width / 2, rect.GetTop());
	};

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(CInfoText, wxWindow)
EVT_PAINT(CInfoText::OnPaint)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(CRemoteListView, wxListCtrl)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, CRemoteListView::OnItemActivated)
	EVT_LIST_COL_CLICK(wxID_ANY, CRemoteListView::OnColumnClicked)
	EVT_CONTEXT_MENU(CRemoteListView::OnContextMenu)
	// Map both ID_DOWNLOAD and ID_ADDTOQUEUE to OnMenuDownload, code is identical
	EVT_MENU(XRCID("ID_DOWNLOAD"), CRemoteListView::OnMenuDownload)
	EVT_MENU(XRCID("ID_ADDTOQUEUE"), CRemoteListView::OnMenuDownload)
	EVT_MENU(XRCID("ID_MKDIR"), CRemoteListView::OnMenuMkdir)
	EVT_MENU(XRCID("ID_DELETE"), CRemoteListView::OnMenuDelete)
	EVT_MENU(XRCID("ID_RENAME"), CRemoteListView::OnMenuRename)
	EVT_MENU(XRCID("ID_CHMOD"), CRemoteListView::OnMenuChmod)
	EVT_CHAR(CRemoteListView::OnChar)
	EVT_KEY_DOWN(CRemoteListView::OnKeyDown)
	EVT_LIST_BEGIN_LABEL_EDIT(wxID_ANY, CRemoteListView::OnBeginLabelEdit)
	EVT_LIST_END_LABEL_EDIT(wxID_ANY, CRemoteListView::OnEndLabelEdit)
	EVT_SIZE(CRemoteListView::OnSize)
	EVT_LIST_BEGIN_DRAG(wxID_ANY, CRemoteListView::OnBeginDrag)
END_EVENT_TABLE()

CRemoteListView::CRemoteListView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView* pQueue)
	: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxLC_VIRTUAL | wxLC_REPORT | wxNO_BORDER | wxLC_EDIT_LABELS),
	CSystemImageList(16),
	CStateEventHandler(pState, STATECHANGE_REMOTE_DIR | STATECHANGE_REMOTE_DIR_MODIFIED | STATECHANGE_APPLYFILTER)
{
	m_dropTarget = -1;

	m_pInfoText = 0;
	m_pDirectoryListing = 0;
	m_pChmodDlg = 0;

	m_pQueue = pQueue;
	m_operationMode = recursive_none;

	InsertColumn(0, _("Filename"));
	InsertColumn(1, _("Filesize"), wxLIST_FORMAT_RIGHT);
	InsertColumn(2, _("Filetype"));
	InsertColumn(3, _("Last modified"), wxLIST_FORMAT_LEFT, 100);
	InsertColumn(4, _("Permissions"));
	InsertColumn(5, _("Owner / Group"));

	m_sortColumn = 0;
	m_sortDirection = 0;

	m_dirIcon = GetIconIndex(dir);
	SetImageList(GetSystemImageList(), wxIMAGE_LIST_SMALL);

#ifdef __WXMSW__
	// Initialize imagelist for list header
	m_pHeaderImageList = new wxImageListEx(8, 8, true, 3);

	wxBitmap bmp;

	bmp.LoadFile(wxGetApp().GetResourceDir() + _T("empty.png"), wxBITMAP_TYPE_PNG);
	m_pHeaderImageList->Add(bmp);
	bmp.LoadFile(wxGetApp().GetResourceDir() + _T("up.png"), wxBITMAP_TYPE_PNG);
	m_pHeaderImageList->Add(bmp);
	bmp.LoadFile(wxGetApp().GetResourceDir() + _T("down.png"), wxBITMAP_TYPE_PNG);
	m_pHeaderImageList->Add(bmp);

	HWND hWnd = (HWND)GetHandle();
	if (!hWnd)
	{
		delete m_pHeaderImageList;
		m_pHeaderImageList = 0;
		return;
	}

	HWND header = (HWND)SendMessage(hWnd, LVM_GETHEADER, 0, 0);
	if (!header)
	{
		delete m_pHeaderImageList;
		m_pHeaderImageList = 0;
		return;
	}

	wxChar buffer[1000] = {0};
	HDITEM item;
	item.mask = HDI_TEXT;
	item.pszText = buffer;
	item.cchTextMax = 999;
	SendMessage(header, HDM_GETITEM, 0, (LPARAM)&item);

	SendMessage(header, HDM_SETIMAGELIST, 0, (LPARAM)m_pHeaderImageList->GetHandle());
#endif

	SetDirectoryListing(0);

	SetDropTarget(new CRemoteListViewDropTarget(this));
}

CRemoteListView::~CRemoteListView()
{
#ifdef __WXMSW__
	delete m_pHeaderImageList;
#endif
}

// Declared const due to design error in wxWidgets.
// Won't be fixed since a fix would break backwards compatibility
// Both functions use a const_cast<CRemoteListView *>(this) and modify
// the instance.
wxString CRemoteListView::OnGetItemText(long item, long column) const
{
	CRemoteListView *pThis = const_cast<CRemoteListView *>(this);
	int index = pThis->GetItemIndex(item);
	if (index == -1)
		return _T("");

	if (!column)
	{
		if ((unsigned int)index == m_pDirectoryListing->GetCount())
			return _T("..");
		else
			return (*m_pDirectoryListing)[index].name;
	}
	if (!item)
		return _T(""); //.. has no attributes

	if (column == 1)
	{
		const CDirentry& entry = (*m_pDirectoryListing)[index];
		if (entry.dir || entry.size < 0)
			return _T("");
		else
			return entry.size.ToString();
	}
	else if (column == 2)
	{
		t_fileData& data = pThis->m_fileData[index];
		if (data.fileType == _T(""))
		{
			const CDirentry& entry = (*m_pDirectoryListing)[index];
			data.fileType = pThis->GetType(entry.name, entry.dir);
		}

		return data.fileType;
	}
	else if (column == 3)
	{
		const CDirentry& entry = (*m_pDirectoryListing)[index];
		if (!entry.hasDate)
			return _T("");

		if (entry.hasTime)
			return entry.time.Format(_T("%c"));
		else
			return entry.time.Format(_T("%x"));
	}
	else if (column == 4)
		return (*m_pDirectoryListing)[index].permissions;
	else if (column == 5)
		return (*m_pDirectoryListing)[index].ownerGroup;
	return _T("");
}

#ifndef __WXMSW__
// This function converts to the right size with the given background colour
// Defined in LocalListView.cpp
wxBitmap PrepareIcon(wxIcon icon, wxColour colour);
#endif

// See comment to OnGetItemText
int CRemoteListView::OnGetItemImage(long item) const
{
	CRemoteListView *pThis = const_cast<CRemoteListView *>(this);
	int index = GetItemIndex(item);
	if (index == -1)
		return -1;

	int &icon = pThis->m_fileData[index].icon;

	if (icon != -2)
		return icon;

	icon = pThis->GetIconIndex(file, (*m_pDirectoryListing)[index].name, false);
	return icon;
}

int CRemoteListView::GetItemIndex(unsigned int item) const
{
	if (item >= m_indexMapping.size())
		return -1;

	unsigned int index = m_indexMapping[item];
	if (index >= m_fileData.size())
		return -1;

	return index;
}

bool CRemoteListView::IsItemValid(unsigned int item) const
{
	if (item >= m_indexMapping.size())
		return false;

	unsigned int index = m_indexMapping[item];
	if (index >= m_fileData.size())
		return false;

	return true;
}

void CRemoteListView::UpdateDirectoryListing_Removed(const CDirectoryListing *pDirectoryListing)
{
	std::list<unsigned int> removedItems;
	
	unsigned int j = 0;
	for (unsigned int i = 0; i < pDirectoryListing->GetCount(); i++, j++)
	{
		if ((*pDirectoryListing)[i].name == (*m_pDirectoryListing)[j].name)
			continue;
		removedItems.push_back(j++);
	}
	for (; j < m_pDirectoryListing->GetCount(); j++)
		removedItems.push_back(j);

	for (std::list<unsigned int>::reverse_iterator iter = removedItems.rbegin(); iter != removedItems.rend(); iter++)
	{
		m_fileData.erase(m_fileData.begin() + *iter);
	}

	std::list<int> selectedItems;

	// Number of items left to remove
	unsigned int toRemove = m_pDirectoryListing->GetCount() - pDirectoryListing->GetCount();
	wxASSERT(toRemove == removedItems.size());
	
	std::list<int> removedIndexes;

	const int size = m_indexMapping.size();
	for (int i = size - 1; i >= 0; i--)
	{
		bool removed = false;

		// j is the offset to index has to be adjusted
		int j = 0;
		for (std::list<unsigned int>::const_iterator iter = removedItems.begin(); iter != removedItems.end(); iter++, j++)
		{
			if (*iter > m_indexMapping[i])
				break;

			if (*iter == m_indexMapping[i])
			{
				removedIndexes.push_back(i);
				removed = true;
				toRemove--;
				break;
			}
		}
		m_indexMapping[i] -= j;

		// Update selection
		bool isSelected = GetItemState(i, wxLIST_STATE_SELECTED) != 0;
		bool needSelection;
		if (selectedItems.empty())
			needSelection = false;
		else if (selectedItems.front() == i)
		{
			needSelection = true;
			selectedItems.pop_front();
		}
		else
			needSelection = false;

		if (isSelected)
		{
			if (!needSelection && toRemove)
				SetItemState(i, 0, wxLIST_STATE_SELECTED);

			if (!removed)
				selectedItems.push_back(i - toRemove);
		}
		else if (needSelection)
			SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}

	wxASSERT(removedIndexes.size() == removedItems.size());
	for (std::list<int>::reverse_iterator iter = removedIndexes.rbegin(); iter != removedIndexes.rend(); iter++)
	{
		m_indexMapping.erase(m_indexMapping.begin() + *iter);
	}

	m_pDirectoryListing = pDirectoryListing;

	SetItemCount(m_indexMapping.size());
}

bool CRemoteListView::UpdateDirectoryListing(const CDirectoryListing *pDirectoryListing)
{
	if ((pDirectoryListing->m_hasUnsureEntries & UNSURE_CHANGE) == UNSURE_CHANGE)
	{
		if (m_sortColumn && m_sortColumn != 2)
		{
			// If now sorted by file or type, changing file attributes can influence
			// sort order.
			return false;
		}
	}

	if ((pDirectoryListing->m_hasUnsureEntries & UNSURE_CONFUSED) == UNSURE_CONFUSED)
		return false;

	int addremove = UNSURE_ADD | UNSURE_REMOVE;
	if ((pDirectoryListing->m_hasUnsureEntries & addremove) == addremove)
		return false;

	if ((pDirectoryListing->m_hasUnsureEntries & UNSURE_REMOVE) == UNSURE_REMOVE)
	{
		wxASSERT(pDirectoryListing->GetCount() < m_pDirectoryListing->GetCount());
		UpdateDirectoryListing_Removed(pDirectoryListing);
		return true;
	}

	return false;
}

void CRemoteListView::SetDirectoryListing(const CDirectoryListing *pDirectoryListing, bool modified /*=false*/)
{
	bool reset = false;
	if (!pDirectoryListing || !m_pDirectoryListing)
		reset = true;
	else if (m_pDirectoryListing->path != pDirectoryListing->path)
		reset = true;
	else if (m_pDirectoryListing->GetCount() > 200 && m_pDirectoryListing->m_firstListTime == pDirectoryListing->m_firstListTime)
	{
		// Updated directory listing. Check if we can use process it in a different,
		// more efficient way.
		// Makes only sense for big listings though.
		if (UpdateDirectoryListing(pDirectoryListing))
		{
			wxASSERT(GetItemCount() == (int)m_indexMapping.size());
			wxASSERT(GetItemCount() == (int)m_fileData.size());
			wxASSERT(m_pDirectoryListing->GetCount() + 1 >= (unsigned int)GetItemCount());
			wxASSERT(m_indexMapping[0] == m_pDirectoryListing->GetCount());

			Refresh();

			if (!modified)
				ProcessDirectoryListing();
			return;
		}
	}

	wxString prevFocused;
	std::list<wxString> selectedNames;
	if (reset)
	{
		// Clear selection
		int item = -1;
		while (true)
		{
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			if (item == -1)
				break;
			SetItemState(item, 0, wxLIST_STATE_SELECTED);
		}
		item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
		if (item != -1)
		{
			SetItemState(item, 0, wxLIST_STATE_FOCUSED);
			wxASSERT(GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED) == -1);
		}
	}
	else
	{
		// Remember which items were selected
		selectedNames = RememberSelectedItems(prevFocused);
	}

	m_pDirectoryListing = pDirectoryListing;

	m_fileData.clear();
	m_indexMapping.clear();

	bool eraseBackground = false;
	if (m_pDirectoryListing)
	{
		if (m_pDirectoryListing->m_failed)
			SetInfoText(_("Directory listing failed"));
		else if (!m_pDirectoryListing->GetCount())
			SetInfoText(_("Empty directory listing"));
		else
			SetInfoText(_T(""));

		m_indexMapping.push_back(m_pDirectoryListing->GetCount());

		CFilterDialog filter;
		for (unsigned int i = 0; i < m_pDirectoryListing->GetCount(); i++)
		{
			const CDirentry& entry = (*m_pDirectoryListing)[i];
			t_fileData data;
			data.icon = entry.dir ? m_dirIcon : -2;
			m_fileData.push_back(data);

			if (!filter.FilenameFiltered(entry.name, entry.dir, entry.size, false))
				m_indexMapping.push_back(i);
		}

		t_fileData data;
		data.icon = m_dirIcon;
		m_fileData.push_back(data);
	}
	else
	{
		eraseBackground = true;
		StopRecursiveOperation();
		SetInfoText(_("Not connected to any server"));
	}

	if (m_dropTarget != -1)
	{
		bool resetDropTarget = false;
		int index = GetItemIndex(m_dropTarget);
		if (index == -1)
			resetDropTarget = true;
		else if (index != (int)m_pDirectoryListing->GetCount())
			if (!(*m_pDirectoryListing)[index].dir)
				resetDropTarget = true;

		if (resetDropTarget)
		{
			SetItemState(m_dropTarget, 0, wxLIST_STATE_DROPHILITED);
			m_dropTarget = -1;
		}
	}

	if ((unsigned int)GetItemCount() > m_indexMapping.size())
		eraseBackground = true;
	if ((unsigned int)GetItemCount() != m_indexMapping.size())
		SetItemCount(m_indexMapping.size());

	if (GetItemCount() && reset)
	{
		EnsureVisible(0);
		SetItemState(0, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
	}

	SortList();

	ReselectItems(selectedNames, prevFocused);

	Refresh(eraseBackground);

	if (!modified)
		ProcessDirectoryListing();
}

// Helper classes for fast sorting using std::sort
// -----------------------------------------------

class CRemoteListViewSort : public std::binary_function<int,int,bool>
{
public:

	enum DirSortMode
	{
		dirsort_ontop,
		dirsort_onbottom,
		dirsort_inline
	};

	CRemoteListViewSort(const CDirectoryListing* const pDirectoryListing, enum DirSortMode dirSortMode)
		: m_pDirectoryListing(pDirectoryListing), m_dirSortMode(dirSortMode)
	{
	}

	#define CMP(f, data1, data2) \
		{\
			int res = f(data1, data2);\
			if (res == -1)\
				return true;\
			else if (res == 1)\
				return false;\
		}

	#define CMP_LESS(f, data1, data2) \
		{\
			int res = f(data1, data2);\
			if (res == -1)\
				return true;\
			else\
				return false;\
		}

	inline int CmpDir(const CDirentry &data1, const CDirentry &data2) const
	{
		switch (m_dirSortMode)
		{
		default:
		case dirsort_ontop:
			if (data1.dir)
			{
				if (!data2.dir)
					return -1;
				else
					return 0;
			}
			else
			{
				if (data2.dir)
					return 1;
				else
					return 0;
			}
		case dirsort_onbottom:
			if (data1.dir)
			{
				if (!data2.dir)
					return 1;
				else
					return 0;
			}
			else
			{
				if (data2.dir)
					return -1;
				else
					return 0;
			}
		case dirsort_inline:
			return 0;
		}
	}

	inline int CmpName(const CDirentry &data1, const CDirentry &data2) const
	{
#ifdef __WXMSW__
		return data1.name.CmpNoCase(data2.name);
#else
		return data1.name.Cmp(data2.name);
#endif
	}

	inline int CmpSize(const CDirentry &data1, const CDirentry &data2) const
	{
		const wxLongLong diff = data1.size - data2.size;
		if (diff < 0)
			return -1;
		else if (diff > 0)
			return 1;
		else
			return 0;
	}

	inline int CmpStringNoCase(const wxString &data1, const wxString &data2) const
	{
		return data1.CmpNoCase(data2);
	}

	inline int CmpTime(const CDirentry &data1, const CDirentry &data2) const
	{
		if (!data1.hasDate)
		{
			if (data2.hasDate)
				return -1;
			else
				return 0;
		}
		else
		{
			if (!data2.hasDate)
				return 1;

			if (data1.time < data2.time)
				return -1;
			else if (data1.time > data2.time)
				return 1;
			else
				return 0;
		}
	}

protected:
	const CDirectoryListing* const m_pDirectoryListing;

	const enum DirSortMode m_dirSortMode;
};

class CRemoteListViewSortName : public CRemoteListViewSort
{
public:
	CRemoteListViewSortName(const CDirectoryListing* const pDirectoryListing, enum DirSortMode dirSortMode)
		: CRemoteListViewSort(pDirectoryListing, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = (*m_pDirectoryListing)[a];
		const CDirentry& data2 = (*m_pDirectoryListing)[b];

		CMP(CmpDir, data1, data2);

		CMP_LESS(CmpName, data1, data2);
	}
};

class CRemoteListViewSortSize : public CRemoteListViewSort
{
public:
	CRemoteListViewSortSize(const CDirectoryListing* const pDirectoryListing, enum DirSortMode dirSortMode)
		: CRemoteListViewSort(pDirectoryListing, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = (*m_pDirectoryListing)[a];
		const CDirentry& data2 = (*m_pDirectoryListing)[b];

		CMP(CmpDir, data1, data2);

		CMP(CmpSize, data1, data2);

		CMP_LESS(CmpName, data1, data2);
	}
};

class CRemoteListViewSortType : public CRemoteListViewSort
{
public:
	CRemoteListViewSortType(CRemoteListView* const pRemoteListView, enum DirSortMode dirSortMode, const CDirectoryListing* const pDirectoryListing, std::vector<CRemoteListView::t_fileData>& fileData)
		: CRemoteListViewSort(pDirectoryListing, dirSortMode), m_pRemoteListView(pRemoteListView), m_fileData(fileData)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = (*m_pDirectoryListing)[a];
		const CDirentry& data2 = (*m_pDirectoryListing)[b];

		CMP(CmpDir, data1, data2);

		CRemoteListView::t_fileData &type1 = m_fileData[a];
		CRemoteListView::t_fileData &type2 = m_fileData[b];
		if (type1.fileType == _T(""))
			type1.fileType = m_pRemoteListView->GetType(data1.name, data1.dir);
		if (type2.fileType == _T(""))
			type2.fileType = m_pRemoteListView->GetType(data2.name, data2.dir);

		CMP(CmpStringNoCase, type1.fileType, type2.fileType);

		CMP_LESS(CmpName, data1, data2);
	}

protected:
	CRemoteListView* const m_pRemoteListView;
	std::vector<CRemoteListView::t_fileData>& m_fileData;
};

class CRemoteListViewSortTime : public CRemoteListViewSort
{
public:
	CRemoteListViewSortTime(const CDirectoryListing* const pDirectoryListing, enum DirSortMode dirSortMode)
		: CRemoteListViewSort(pDirectoryListing, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = (*m_pDirectoryListing)[a];
		const CDirentry& data2 = (*m_pDirectoryListing)[b];

		CMP(CmpDir, data1, data2);

		CMP(CmpTime, data1, data2);

		CMP_LESS(CmpName, data1, data2);
	}
};

class CRemoteListViewSortPermissions : public CRemoteListViewSort
{
public:
	CRemoteListViewSortPermissions(const CDirectoryListing* const pDirectoryListing, enum DirSortMode dirSortMode)
		: CRemoteListViewSort(pDirectoryListing, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = (*m_pDirectoryListing)[a];
		const CDirentry& data2 = (*m_pDirectoryListing)[b];

		CMP(CmpDir, data1, data2);

		CMP(CmpStringNoCase, data1.permissions, data2.permissions);

		CMP_LESS(CmpName, data1, data2);
	}
};

void CRemoteListView::SortList(int column /*=-1*/, int direction /*=-1*/)
{
	if (column != -1)
	{
#ifdef __WXMSW__
		if (column != m_sortColumn && m_pHeaderImageList)
		{
			HWND hWnd = (HWND)GetHandle();
			HWND header = (HWND)SendMessage(hWnd, LVM_GETHEADER, 0, 0);

			wxChar buffer[100];
			HDITEM item;
			item.mask = HDI_TEXT | HDI_FORMAT;
			item.pszText = buffer;
			item.cchTextMax = 99;
			SendMessage(header, HDM_GETITEM, m_sortColumn, (LPARAM)&item);
			item.mask |= HDI_IMAGE;
			item.fmt |= HDF_IMAGE | HDF_BITMAP_ON_RIGHT;
			item.iImage = 0;
			SendMessage(header, HDM_SETITEM, m_sortColumn, (LPARAM)&item);
		}
#endif
	}
	else
		column = m_sortColumn;

	if (direction == -1)
		direction = m_sortDirection;

#ifdef __WXMSW__
	if (m_pHeaderImageList)
	{
		HWND hWnd = (HWND)GetHandle();
		HWND header = (HWND)SendMessage(hWnd, LVM_GETHEADER, 0, 0);

		wxChar buffer[100];
		HDITEM item;
		item.mask = HDI_TEXT | HDI_FORMAT;
		item.pszText = buffer;
		item.cchTextMax = 99;
		SendMessage(header, HDM_GETITEM, column, (LPARAM)&item);
		item.mask |= HDI_IMAGE;
		item.fmt |= HDF_IMAGE | HDF_BITMAP_ON_RIGHT;
		item.iImage = direction ? 2 : 1;
		SendMessage(header, HDM_SETITEM, column, (LPARAM)&item);
	}
#endif

	// Remember which files are selected
	bool *selected = new bool[GetItemCount()];
	memset(selected, 0, sizeof(bool) * GetItemCount());

	int item = -1;
	while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
		selected[m_indexMapping[item]] = 1;
	int focused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
	if (focused != -1)
		focused = m_indexMapping[focused];

	const int dirSortOption = COptions::Get()->GetOptionVal(OPTION_FILELIST_DIRSORT);

	if (column == m_sortColumn && direction != m_sortDirection && !m_indexMapping.empty() &&
		dirSortOption != 1)
	{
		// Simply reverse everything
		m_sortDirection = direction;
		m_sortColumn = column;
		std::reverse(++m_indexMapping.begin(), m_indexMapping.end());

		SortList_UpdateSelections(selected, focused);
		delete [] selected;

		Refresh(false);
		return;
	}

	m_sortDirection = direction;
	m_sortColumn = column;

	if (GetItemCount() < 3)
	{
		delete [] selected;
		return;
	}

	CRemoteListViewSort::DirSortMode dirSortMode;
	switch (dirSortOption)
	{
	case 0:
	default:
		dirSortMode = CRemoteListViewSort::dirsort_ontop;
		break;
	case 1:
		if (m_sortDirection)
			dirSortMode = CRemoteListViewSort::dirsort_onbottom;
		else
			dirSortMode = CRemoteListViewSort::dirsort_ontop;
		break;
	case 2:
		dirSortMode = CRemoteListViewSort::dirsort_inline;
		break;
	}

	if (!m_sortColumn)
		std::sort(++m_indexMapping.begin(), m_indexMapping.end(), CRemoteListViewSortName(m_pDirectoryListing, dirSortMode));
	else if (m_sortColumn == 1)
		std::sort(++m_indexMapping.begin(), m_indexMapping.end(), CRemoteListViewSortSize(m_pDirectoryListing, dirSortMode));
	else if (m_sortColumn == 2)
		std::sort(++m_indexMapping.begin(), m_indexMapping.end(), CRemoteListViewSortType(this, dirSortMode, m_pDirectoryListing, m_fileData));
	else if (m_sortColumn == 3)
		std::sort(++m_indexMapping.begin(), m_indexMapping.end(), CRemoteListViewSortTime(m_pDirectoryListing, dirSortMode));
	else if (m_sortColumn == 4)
		std::sort(++m_indexMapping.begin(), m_indexMapping.end(), CRemoteListViewSortPermissions(m_pDirectoryListing, dirSortMode));
	
	if (m_sortDirection)
		std::reverse(++m_indexMapping.begin(), m_indexMapping.end());

	SortList_UpdateSelections(selected, focused);
	delete [] selected;
	
	Refresh(false);
}

void CRemoteListView::SortList_UpdateSelections(bool* selections, int focus)
{
	for (int i = 1; i < GetItemCount(); i++)
	{
		const int state = GetItemState(i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		const bool selected = (state & wxLIST_STATE_SELECTED) != 0;
		const bool focused = (state & wxLIST_STATE_FOCUSED) != 0;

		int item = m_indexMapping[i];
		if (selections[item] != selected)
			SetItemState(i, selections[item] ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED);
		if (focused)
		{
			if (item != focus)
				SetItemState(i, 0, wxLIST_STATE_FOCUSED);
		}
		else
		{
			if (item == focus)
				SetItemState(i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
		}
	}
}

void CRemoteListView::OnColumnClicked(wxListEvent &event)
{
	int col = event.GetColumn();
	if (col == -1)
		return;

	int dir;
	if (col == m_sortColumn)
		dir = m_sortDirection ? 0 : 1;
	else
		dir = m_sortDirection;

	SortList(col, dir);
}

wxString CRemoteListView::GetType(wxString name, bool dir)
{
	wxString type;
#ifdef __WXMSW__
	wxString ext = wxFileName(name).GetExt();
	ext.MakeLower();
	std::map<wxString, wxString>::iterator typeIter = m_fileTypeMap.find(ext);
	if (typeIter != m_fileTypeMap.end())
		type = typeIter->second;
	else
	{
		SHFILEINFO shFinfo;
		memset(&shFinfo,0,sizeof(SHFILEINFO));
		if (SHGetFileInfo(name,
			dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL,
			&shFinfo,
			sizeof(shFinfo),
			SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES))
		{
			type = shFinfo.szTypeName;
			if (type == _T(""))
			{
				type = ext;
				type.MakeUpper();
				if (!type.IsEmpty())
				{
					type += _T("-");
					type += _("file");
				}
				else
					type = _("File");
			}
			else
			{
				if (!dir && ext != _T(""))
					m_fileTypeMap[ext.MakeLower()] = type;
			}
		}
		else
		{
			type = ext;
			type.MakeUpper();
			if (!type.IsEmpty())
			{
				type += _T("-");
				type += _("file");
			}
			else
				type = _("File");
		}
	}
#else
	type = dir ? _("Folder") : _("File");
#endif
	return type;
}

void CRemoteListView::OnItemActivated(wxListEvent &event)
{
	if (!m_pState->m_pCommandQueue->Idle() || IsBusy())
	{
		wxBell();
		return;
	}

	int count = 0;
	bool back = false;

	int item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		count++;

		if (!item)
			back = true;
	}
	if (count > 1)
	{
		if (back)
		{
			wxBell();
			return;
		}

		wxCommandEvent cmdEvent;
		OnMenuDownload(cmdEvent);
		return;
	}

	item = event.GetIndex();

	if (item)
	{
		int index = GetItemIndex(item);
		if (index == -1)
			return;

		const CDirentry& entry = (*m_pDirectoryListing)[index];
		const wxString& name = entry.name;

		if (entry.dir)
			m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(m_pDirectoryListing->path, name));
		else
		{
			const CServer* pServer = m_pState->GetServer();
			if (!pServer)
			{
				wxBell();
				return;
			}

			wxFileName fn = wxFileName(m_pState->GetLocalDir(), name);
			m_pQueue->QueueFile(false, true, fn.GetFullPath(), name, m_pDirectoryListing->path, *pServer, entry.size);
		}
	}
	else
		m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(m_pDirectoryListing->path, _T("..")));
}

void CRemoteListView::OnContextMenu(wxContextMenuEvent& event)
{
	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_REMOTEFILELIST"));
	if (!pMenu)
		return;

	if (!m_pDirectoryListing || IsBusy())
	{
		pMenu->Enable(XRCID("ID_DOWNLOAD"), false);
		pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
		pMenu->Enable(XRCID("ID_MKDIR"), false);
		pMenu->Enable(XRCID("ID_DELETE"), false);
		pMenu->Enable(XRCID("ID_RENAME"), false);
		pMenu->Enable(XRCID("ID_CHMOD"), false);
	}
	else if (GetItemState(0, wxLIST_STATE_SELECTED) ||
		GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) == -1)
	{
		pMenu->Enable(XRCID("ID_DOWNLOAD"), false);
		pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
		pMenu->Enable(XRCID("ID_DELETE"), false);
		pMenu->Enable(XRCID("ID_RENAME"), false);
		pMenu->Enable(XRCID("ID_CHMOD"), false);
	}

	PopupMenu(pMenu);
	delete pMenu;
}

void CRemoteListView::OnMenuDownload(wxCommandEvent& event)
{
	if (IsBusy())
	{
		wxBell();
		return;
	}

	// Make sure selection is valid
	bool idle = m_pState->m_pCommandQueue->Idle();

	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item)
		{
			wxBell();
			return;
		}

		int index = GetItemIndex(item);
		if (index == -1)
			continue;
		if ((*m_pDirectoryListing)[index].dir && !idle)
		{
			wxBell();
			return;
		}
	}

	TransferSelectedFiles(m_pState->GetLocalDir(), event.GetId() == XRCID("ID_ADDTOQUEUE"));
}

void CRemoteListView::TransferSelectedFiles(const wxString& localDir, bool queueOnly)
{
	wxASSERT(!IsBusy());

	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		int index = GetItemIndex(item);
		if (index == -1)
			continue;

		const CDirentry& entry = (*m_pDirectoryListing)[index];
		const wxString& name = entry.name;

		const CServer* pServer = m_pState->GetServer();
		if (!pServer)
		{
			wxBell();
			return;
		}

		if (entry.dir)
		{
			wxFileName fn = wxFileName(localDir, _T(""));
			fn.AppendDir(name);
			CServerPath remotePath = m_pDirectoryListing->path;
			if (remotePath.AddSegment(name))
			{
				//m_pQueue->QueueFolder(event.GetId() == XRCID("ID_ADDTOQUEUE"), true, fn.GetFullPath(), remotePath, *pServer);
				m_operationMode = queueOnly ? recursive_addtoqueue : recursive_download;
				t_newDir dirToVisit;
				dirToVisit.doVisit = true;
				dirToVisit.localDir = fn.GetFullPath();
				dirToVisit.parent = m_pDirectoryListing->path;
				dirToVisit.subdir = name;
				m_dirsToVisit.push_back(dirToVisit);
				m_startDir = m_pDirectoryListing->path;
			}
		}
		else
		{
			wxFileName fn = wxFileName(localDir, name);
			m_pQueue->QueueFile(queueOnly, true, fn.GetFullPath(), name, m_pDirectoryListing->path, *pServer, entry.size);
		}
	}
	NextOperation();
}

void CRemoteListView::OnMenuMkdir(wxCommandEvent& event)
{
	if (!m_pDirectoryListing || IsBusy())
	{
		wxBell();
		return;
	}

	CInputDialog dlg;
	if (!dlg.Create(this, _("Create directory"), _("Please enter the name of the directory which should be created:")))
		return;

	CServerPath path = m_pDirectoryListing->path;

	// Append a long segment which does (most likely) not exist in the path and
	// replace it with "New folder" later. This way we get the exact position of
	// "New folder" and can preselect it in the dialog.
	wxString tmpName = _T("25CF809E56B343b5A12D1F0466E3B37A49A9087FDCF8412AA9AF8D1E849D01CF");
	if (path.AddSegment(tmpName))
	{
		wxString pathName = path.GetPath();
		int pos = pathName.Find(tmpName);
		wxASSERT(pos != -1);
		wxString newName = _("New folder");
		pathName.Replace(tmpName, newName);
		dlg.SetValue(pathName);
		dlg.SelectText(pos, pos + newName.Length());
	}

	if (dlg.ShowModal() != wxID_OK)
		return;

	path = m_pDirectoryListing->path;
	if (!path.ChangePath(dlg.GetValue()))
	{
		wxBell();
		return;
	}

	m_pState->m_pCommandQueue->ProcessCommand(new CMkdirCommand(path));
}

void CRemoteListView::OnMenuDelete(wxCommandEvent& event)
{
	if (!m_pState->m_pCommandQueue->Idle() || IsBusy())
	{
		wxBell();
		return;
	}

	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item || !IsItemValid(item))
		{
			wxBell();
			return;
		}
	}

	if (wxMessageBox(_("Really delete all selected files and/or directories?"), _("Confirmation needed"), wxICON_QUESTION | wxYES_NO, this) != wxYES)
		return;

	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		int index = GetItemIndex(item);
		if (index == -1)
			continue;

		const CDirentry& entry = (*m_pDirectoryListing)[index];
		const wxString& name = entry.name;

		if (entry.dir)
		{
			CServerPath remotePath = m_pDirectoryListing->path;
			if (remotePath.AddSegment(name))
			{
				m_operationMode = recursive_delete;
				t_newDir dirToVisit;
				dirToVisit.parent = m_pDirectoryListing->path;
				dirToVisit.subdir = name;
				dirToVisit.doVisit = true;
				m_dirsToVisit.push_back(dirToVisit);
				dirToVisit.doVisit = false;
				m_dirsToVisit.push_back(dirToVisit);
				m_startDir = m_pDirectoryListing->path;
			}
		}
		else
			m_pState->m_pCommandQueue->ProcessCommand(new CDeleteCommand(m_pDirectoryListing->path, name));
	}
	NextOperation();
}

void CRemoteListView::OnMenuRename(wxCommandEvent& event)
{
	if (!m_pState->m_pCommandQueue->Idle() || IsBusy())
	{
		wxBell();
		return;
	}

	int item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item <= 0)
	{
		wxBell();
		return;
	}

	if (GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) != -1)
	{
		wxBell();
		return;
	}

	EditLabel(item);
}

bool CRemoteListView::NextOperation()
{
	if (m_operationMode == recursive_none)
		return false;

	while (!m_dirsToVisit.empty())
	{
		const t_newDir& dirToVisit = m_dirsToVisit.front();
		if (m_operationMode == recursive_delete && !dirToVisit.doVisit)
		{
			m_pState->m_pCommandQueue->ProcessCommand(new CRemoveDirCommand(dirToVisit.parent, dirToVisit.subdir));
			m_dirsToVisit.pop_front();
			continue;
		}

		m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(dirToVisit.parent, dirToVisit.subdir));
		return true;
	}

	StopRecursiveOperation();
	m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(m_startDir));
	return false;
}

void CRemoteListView::ProcessDirectoryListing()
{
	if (!IsBusy())
		return;

	wxASSERT(!m_dirsToVisit.empty());

	if (!m_pDirectoryListing || m_dirsToVisit.empty())
	{
		StopRecursiveOperation();
		return;
	}

	t_newDir dir = m_dirsToVisit.front();
	m_dirsToVisit.pop_front();

	if (!m_pDirectoryListing->path.IsSubdirOf(m_startDir, false))
	{
		NextOperation();
		return;
	}

	// Check if we have already visited the directory
	for (std::list<CServerPath>::const_iterator iter = m_visitedDirs.begin(); iter != m_visitedDirs.end(); iter++)
	{
		if (*iter == m_pDirectoryListing->path)
		{
			NextOperation();
			return;
		}
	}
	m_visitedDirs.push_back(m_pDirectoryListing->path);

	const CServer* pServer = m_pState->GetServer();
	wxASSERT(pServer);

	if (!m_pDirectoryListing->GetCount())
	{
		wxFileName fn(dir.localDir, _T(""));
		if (m_operationMode == recursive_download)
		{
			wxFileName::Mkdir(fn.GetPath(), 0777, wxPATH_MKDIR_FULL);
		}
		else if (m_operationMode == recursive_addtoqueue)
			m_pQueue->QueueFile(true, true, fn.GetFullPath(), _T(""), CServerPath(), *pServer, -1);
	}

	CFilterDialog filter;

	for (int i = m_pDirectoryListing->GetCount() - 1; i >= 0; i--)
	{
		const CDirentry& entry = (*m_pDirectoryListing)[i];

		if (filter.FilenameFiltered(entry.name, entry.dir, entry.size, false))
			continue;

		if (entry.dir)
		{
			t_newDir dirToVisit;
			wxFileName fn(dir.localDir, _T(""));
			fn.AppendDir(entry.name);
			dirToVisit.parent = m_pDirectoryListing->path;
			dirToVisit.subdir = entry.name;
			dirToVisit.localDir = fn.GetFullPath();
			if (m_operationMode == recursive_delete)
			{
				dirToVisit.doVisit = false;
				m_dirsToVisit.push_front(dirToVisit);
			}
			dirToVisit.doVisit = true;
			m_dirsToVisit.push_front(dirToVisit);
		}
		else
		{
			switch (m_operationMode)
			{
			case recursive_addtoqueue:
			case recursive_download:
				{
					wxFileName fn(dir.localDir, entry.name);
					m_pQueue->QueueFile(m_operationMode == recursive_addtoqueue, true, fn.GetFullPath(), entry.name, m_pDirectoryListing->path, *pServer, entry.size);
				}
				break;
			case recursive_delete:
				m_pState->m_pCommandQueue->ProcessCommand(new CDeleteCommand(m_pDirectoryListing->path, entry.name));
				break;
			default:
				break;
			}
		}

		if (m_operationMode == recursive_chmod)
		{
			const int applyType = m_pChmodDlg->GetApplyType();
			if (!applyType ||
				(!entry.dir && applyType == 1) ||
				(entry.dir && applyType == 2))
			{
				char permissions[9];
				bool res = ConvertPermissions(entry.permissions, permissions);
				wxString newPerms = m_pChmodDlg->GetPermissions(res ? permissions : 0);
				m_pState->m_pCommandQueue->ProcessCommand(new CChmodCommand(m_pDirectoryListing->path, entry.name, newPerms));
			}
		}
	}

	NextOperation();
}

void CRemoteListView::ListingFailed()
{
	if (!IsBusy())
		return;

	wxASSERT(!m_dirsToVisit.empty());
	if (m_dirsToVisit.empty())
		return;

	m_dirsToVisit.pop_front();

	NextOperation();
}

void CRemoteListView::StopRecursiveOperation()
{
	m_operationMode = recursive_none;
	m_dirsToVisit.clear();
	m_visitedDirs.clear();

	if (m_pChmodDlg)
	{
		m_pChmodDlg->Destroy();
		m_pChmodDlg = 0;
	}
}

void CRemoteListView::OnChar(wxKeyEvent& event)
{
	int code = event.GetKeyCode();
	if (code == WXK_DELETE)
	{
		if (GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) == -1)
		{
			wxBell();
			return;
		}

		wxCommandEvent tmp;
		OnMenuDelete(tmp);
		return;
	}
	else if (code == WXK_F2)
	{
		wxCommandEvent tmp;
		OnMenuRename(tmp);
	}
	else if (code > 32 && code < 300 && !event.HasModifiers())
	{
		// Keyboard navigation within items
		wxDateTime now = wxDateTime::UNow();
		if (m_lastKeyPress.IsValid())
		{
			wxTimeSpan span = now - m_lastKeyPress;
			if (span.GetSeconds() >= 1)
				m_prefix = _T("");
		}
		m_lastKeyPress = now;

		wxChar tmp[2];
#if wxUSE_UNICODE
		tmp[0] = event.GetUnicodeKey();
#else
		tmp[0] = code;
#endif
		tmp[1] = 0;
		wxString newPrefix = m_prefix + tmp;

		bool beep = false;
		int item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item != -1)
		{
			wxString text;
			if (!item)
				text = _T("..");
			else
			{
				int index = GetItemIndex(item);
				if (index != -1)
					text = (*m_pDirectoryListing)[index].name;
			}
			if (text.Length() >= m_prefix.Length() && !m_prefix.CmpNoCase(text.Left(m_prefix.Length())))
				beep = true;
		}
		else if (m_prefix == _T(""))
			beep = true;

		int start = item;
		if (start < 0)
			start = 0;

		int newPos = FindItemWithPrefix(newPrefix, (item >= 0) ? item : 0);

		if (newPos == -1 && m_prefix == tmp && item != -1 && beep)
		{
			// Search the next item that starts with the same letter
			newPrefix = m_prefix;
			newPos = FindItemWithPrefix(newPrefix, item + 1);
		}

		m_prefix = newPrefix;
		if (newPos == -1)
		{
			if (beep)
				wxBell();
			return;
		}

		item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		while (item != -1)
		{
			SetItemState(item, 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
			item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		}
		SetItemState(newPos, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		EnsureVisible(newPos);
	}
	else
		event.Skip();
}

void CRemoteListView::OnKeyDown(wxKeyEvent& event)
{
	const int code = event.GetKeyCode();
	if (code == 'A' && event.GetModifiers() == wxMOD_CMD)
	{
		for (int i = 1; i < GetItemCount(); i++)
		{
			SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
		}
	}
	else
		event.Skip();
}

int CRemoteListView::FindItemWithPrefix(const wxString& prefix, int start)
{
	for (int i = start; i < (GetItemCount() + start); i++)
	{
		int item = i % GetItemCount();
		wxString fn;
		if (!item)
		{
			fn = _T("..");
			fn = fn.Left(prefix.Length());
		}
		else
		{
			int index = GetItemIndex(item);
			if (index == -1)
				continue;

			fn = (*m_pDirectoryListing)[index].name.Left(prefix.Length());
		}
		if (!fn.CmpNoCase(prefix))
			return i % GetItemCount();
	}
	return -1;
}

void CRemoteListView::OnBeginLabelEdit(wxListEvent& event)
{
	if (!m_pState->m_pCommandQueue->Idle() || IsBusy())
	{
		event.Veto();
		wxBell();
		return;
	}

	if (!m_pDirectoryListing)
	{
		event.Veto();
		wxBell();
		return;
	}

	int item = event.GetIndex();
	if (!item)
	{
		event.Veto();
		return;
	}

	if (!IsItemValid(item))
	{
		event.Veto();
		return;
	}
}

void CRemoteListView::OnEndLabelEdit(wxListEvent& event)
{
	if (event.IsEditCancelled() || event.GetLabel() == _T(""))
	{
		event.Veto();
		return;
	}

	if (!m_pState->m_pCommandQueue->Idle() || IsBusy())
	{
		event.Veto();
		wxBell();
		return;
	}

	if (!m_pDirectoryListing)
	{
		event.Veto();
		wxBell();
		return;
	}

	int item = event.GetIndex();
	if (!item)
	{
		event.Veto();
		return;
	}

	int index = GetItemIndex(item);
	if (index == -1)
	{
		event.Veto();
		return;
	}

	const CDirentry& entry = (*m_pDirectoryListing)[index];

	wxString newFile = event.GetLabel();

	CServerPath newPath = m_pDirectoryListing->path;
	if (!newPath.ChangePath(newFile, true))
	{
		wxMessageBox(_("Filename invalid"), _("Cannot rename file"), wxICON_EXCLAMATION);
		event.Veto();
		return;
	}

	if (newPath == m_pDirectoryListing->path)
	{
		if (entry.name == newFile)
			return;

		// Check if target file already exists
		for (unsigned int i = 0; i < m_pDirectoryListing->GetCount(); i++)
		{
			if (newFile == (*m_pDirectoryListing)[i].name)
			{
				if (wxMessageBox(_("Target filename already exists, really continue?"), _("File exists"), wxICON_QUESTION | wxYES_NO) != wxYES)
				{
					event.Veto();
					return;
				}

				break;
			}
		}
	}

	m_pState->m_pCommandQueue->ProcessCommand(new CRenameCommand(m_pDirectoryListing->path, entry.name, newPath, newFile));
}

void CRemoteListView::OnMenuChmod(wxCommandEvent& event)
{
	if (!m_pState->m_pCommandQueue->Idle() || IsBusy())
	{
		wxBell();
		return;
	}

	int fileCount = 0;
	int dirCount = 0;
	wxString name;

	char permissions[9] = {0};
	const unsigned char permchars[3] = {'r', 'w', 'x'};

	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item)
			return;

		int index = GetItemIndex(item);
		if (index == -1)
			return;

		const CDirentry& entry = (*m_pDirectoryListing)[index];

		if (entry.dir)
			dirCount++;
		else
			fileCount++;
		name = entry.name;

		if (entry.permissions.Length() == 10)
		{
			for (int i = 0; i < 9; i++)
			{
				bool set = entry.permissions[i + 1] == permchars[i % 3];
				if (!permissions[i] || permissions[i] == (set ? 2 : 1))
					permissions[i] = set ? 2 : 1;
				else
					permissions[i] = -1;
			}
		}
	}
	for (int i = 0; i < 9; i++)
		if (permissions[i] == -1)
			permissions[i] = 0;

	m_pChmodDlg = new CChmodDialog;
	if (!m_pChmodDlg->Create(this, fileCount, dirCount, name, permissions))
	{
		m_pChmodDlg->Destroy();
		m_pChmodDlg = 0;
		return;
	}

	if (m_pChmodDlg->ShowModal() != wxID_OK)
	{
		if (m_pChmodDlg)
		{
			m_pChmodDlg->Destroy();
			m_pChmodDlg = 0;
		}
	}
	if (!m_pChmodDlg)
		return;

	const int applyType = m_pChmodDlg->GetApplyType();

	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item)
		{
			m_pChmodDlg->Destroy();
			m_pChmodDlg = 0;
			return;
		}

		int index = GetItemIndex(item);
		if (index == -1)
		{
			m_pChmodDlg->Destroy();
			m_pChmodDlg = 0;
			return;
		}

		const CDirentry& entry = (*m_pDirectoryListing)[index];

		if (!applyType ||
			(!entry.dir && applyType == 1) ||
			(entry.dir && applyType == 2))
		{
			char permissions[9];
			bool res = ConvertPermissions(entry.permissions, permissions);
			wxString newPerms = m_pChmodDlg->GetPermissions(res ? permissions : 0);

			m_pState->m_pCommandQueue->ProcessCommand(new CChmodCommand(m_pDirectoryListing->path, entry.name, newPerms));
		}

		if (m_pChmodDlg->Recursive() && entry.dir)
		{
			m_operationMode = recursive_chmod;
			t_newDir dirToVisit;
			dirToVisit.doVisit = true;
			dirToVisit.parent = m_pDirectoryListing->path;
			dirToVisit.subdir = entry.name;
			m_dirsToVisit.push_back(dirToVisit);
			m_startDir = m_pDirectoryListing->path;
		}
	}
	NextOperation();
}

bool CRemoteListView::ConvertPermissions(const wxString rwx, char* permissions)
{
	if (!permissions)
		return false;

	const unsigned char permchars[3] = {'r', 'w', 'x'};

	if (rwx.Length() != 10)
		return false;

	for (int i = 0; i < 9; i++)
	{
		bool set = rwx[i + 1] == permchars[i % 3];
		permissions[i] = set ? 2 : 1;
	}

	return true;
}


void CRemoteListView::ApplyCurrentFilter()
{
	if (m_fileData.size() <= 1)
		return;

	wxString focused;
	std::list<wxString> selectedNames = RememberSelectedItems(focused);

	CFilterDialog filter;
	m_indexMapping.clear();
	const unsigned int count = m_pDirectoryListing->GetCount();
	m_indexMapping.push_back(count);
	for (unsigned int i = 0; i < count; i++)
	{
		const CDirentry& entry = (*m_pDirectoryListing)[i];
		if (!filter.FilenameFiltered(entry.name, entry.dir, entry.size, false))
			m_indexMapping.push_back(i);
	}
	SetItemCount(m_indexMapping.size());

	SortList();

	ReselectItems(selectedNames, focused);
}

std::list<wxString> CRemoteListView::RememberSelectedItems(wxString& focused)
{
	std::list<wxString> selectedNames;
	// Remember which items were selected
	int item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;
		if (!item)
		{
			selectedNames.push_back(_T(".."));
			continue;
		}
		const CDirentry& entry = (*m_pDirectoryListing)[m_indexMapping[item]];

		if (entry.dir)
			selectedNames.push_back(_T("d") + entry.name);
		else
			selectedNames.push_back(_T("-") + entry.name);
		SetItemState(item, 0, wxLIST_STATE_SELECTED);
	}

	item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
	if (item != -1)
	{
		if (!item)
			focused = _T("..");
		else
			focused = (*m_pDirectoryListing)[m_indexMapping[item]].name;

		SetItemState(item, 0, wxLIST_STATE_FOCUSED);
	}
	
	return selectedNames;
}

void CRemoteListView::ReselectItems(std::list<wxString>& selectedNames, wxString focused)
{
	if (!GetItemCount())
		return;

	if (focused == _T(".."))
	{
		focused = _T("");
		SetItemState(0, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
		return;
	}

	if (selectedNames.empty())
	{
		if (focused == _T(""))
			return;

		for (unsigned int i = 1; i < m_indexMapping.size(); i++)
			if ((*m_pDirectoryListing)[m_indexMapping[i]].name == focused)
			{
				SetItemState(i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
				return;
			}
		return;
	}

	if (selectedNames.front() == _T(".."))
	{
		selectedNames.pop_front();
		SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}

	int firstSelected = -1;

	// Reselect previous items if neccessary.
	// Sorting direction did not change. We just have to scan through items once
	unsigned i = 1;
	for (std::list<wxString>::const_iterator iter = selectedNames.begin(); iter != selectedNames.end(); iter++)
	{
		while (i < m_indexMapping.size())
		{
			const CDirentry& entry = (*m_pDirectoryListing)[m_indexMapping[i]];
			if (entry.name == focused)
			{
				SetItemState(i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
				focused = _T("");
			}
			if (entry.dir && *iter == (_T("d") + entry.name))
			{
				if (firstSelected == -1)
					firstSelected = i;
				SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
				i++;
				break;
			}
			else if (*iter == (_T("-") + entry.name))
			{
				if (firstSelected == -1)
					firstSelected = i;
				SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
				i++;
				break;
			}
			else
				i++;
		}
		if (i == m_indexMapping.size())
			break;
	}
	if (focused != _T(""))
	{
		if (firstSelected != -1)
			SetItemState(firstSelected, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
		else
			SetItemState(0, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
	}
}

void CRemoteListView::OnSize(wxSizeEvent& event)
{
	event.Skip();
	RepositionInfoText();
}

void CRemoteListView::RepositionInfoText()
{
	if (!m_pInfoText)
		return;

	wxRect rect = GetClientRect();

	if (!GetItemCount())
		rect.y = 60;
	else
	{
		wxRect itemRect;
		GetItemRect(0, itemRect);
		rect.y = wxMax(60, itemRect.GetBottom() + 1);
	}

	m_pInfoText->SetSize(rect);
}

void CRemoteListView::OnStateChange(unsigned int event)
{
	wxASSERT(m_pState);
	if (event == STATECHANGE_REMOTE_DIR)
		SetDirectoryListing(m_pState->GetRemoteDir(), false);
	else if (event == STATECHANGE_REMOTE_DIR_MODIFIED)
		SetDirectoryListing(m_pState->GetRemoteDir(), true);
	else if (event == STATECHANGE_APPLYFILTER)
		ApplyCurrentFilter();
}

void CRemoteListView::SetInfoText(const wxString& text)
{
	if (text == _T(""))
	{
		delete m_pInfoText;
		m_pInfoText = 0;
		return;
	}

	if (!m_pInfoText)
	{
		m_pInfoText = new CInfoText(this, text);
		RepositionInfoText();
		return;
	}

	if (m_pInfoText->m_text == _T("<") + text + _T(">"))
		return;

	m_pInfoText->m_text = text;
	m_pInfoText->Refresh();
}

void CRemoteListView::OnBeginDrag(wxListEvent& event)
{
	if (IsBusy())
	{
		wxBell();
		return;
	}

	if (GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) == -1)
	{
		// Nothing selected
		return;
	}

	bool idle = m_pState->m_pCommandQueue->Idle();

	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item)
		{
			// Can't drag ".."
			wxBell();
			return;
		}

		int index = GetItemIndex(item);
		if (index == -1)
			continue;
		if ((*m_pDirectoryListing)[index].dir && !idle)
		{
			// Drag could result in recursive operation, don't allow at this point
			wxBell();
			return;
		}
	}

	wxDataObjectComposite object;

	const CServer* const pServer = m_pState->GetServer();
	if (!pServer)
		return;
	const CServer server = *pServer;
	const CServerPath path = m_pDirectoryListing->path;

	CRemoteDataObject *pRemoteDataObject = new CRemoteDataObject(*pServer, m_pDirectoryListing->path);

	// Add files to remote data object
	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		int index = GetItemIndex(item);
		const CDirentry& entry = (*m_pDirectoryListing)[index];
		
		pRemoteDataObject->AddFile(entry.name, entry.dir, entry.size);
	}

	pRemoteDataObject->Finalize();

	object.Add(pRemoteDataObject, true);

#if FZ3_USESHELLEXT
	CShellExtensionInterface* ext = CShellExtensionInterface::CreateInitialized();
	if (ext)
	{
		const wxString& file = ext->GetDragDirectory();

		wxASSERT(file != _T(""));

		wxFileDataObject *pFileDataObject = new wxFileDataObject;
		pFileDataObject->AddFile(file);

		object.Add(pFileDataObject);
	}
#endif
	
	wxDropSource source(this);
	source.SetData(object);
	if (source.DoDragDrop() != wxDragCopy)
	{
#if FZ3_USESHELLEXT
		delete ext;
		ext = 0;
#endif
		return;
	}

	const wxDataFormat fmt = object.GetReceivedFormat();

#if FZ3_USESHELLEXT
	if (ext)
	{
		if (!pRemoteDataObject->DidSendData())
		{
			const CServer* pServer = m_pState->GetServer();
			if (IsBusy() || 
				!pServer || *pServer != server ||
				!m_pDirectoryListing || m_pDirectoryListing->path != path)
			{
				// Remote listing has changed since drag started
				wxBell();
				delete ext;
				ext = 0;
				return;
			}

			// Same checks as before
			bool idle = m_pState->m_pCommandQueue->Idle();

			long item = -1;
			while (true)
			{
				item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
				if (item == -1)
					break;

				if (!item)
				{
					// Can't drag ".."
					wxBell();
					delete ext;
					ext = 0;
					return;
				}

				int index = GetItemIndex(item);
				if (index == -1)
					continue;
				if ((*m_pDirectoryListing)[index].dir && !idle)
				{
					// Drag could result in recursive operation, don't allow at this point
					wxBell();
					delete ext;
					ext = 0;
					return;
				}
			}

			wxString target = ext->GetTarget();
			if (target == _T(""))
			{
				delete ext;
				ext = 0;
				wxMessageBox(_("Could not determine the target of the Drag&Drop operation.\nEither the shell extension is not installed properly or you didn't drop the files into an Explorer window."));
				return;
			}

			TransferSelectedFiles(target, false);

			delete ext;
			ext = 0;
			return;
		}
		delete ext;
		ext = 0;
	}
#endif
}

bool CRemoteListView::DownloadDroppedFiles(const CRemoteDataObject* pRemoteDataObject, wxString path, bool queueOnly)
{
	if (IsBusy())
		return false;

	const std::list<CRemoteDataObject::t_fileInfo>& files = pRemoteDataObject->GetFiles();
	for (std::list<CRemoteDataObject::t_fileInfo>::const_iterator iter = files.begin(); iter != files.end(); iter++)
	{
		if (!iter->dir)
			continue;

		t_newDir dirToVisit;
		dirToVisit.doVisit = true;
		dirToVisit.localDir = path + iter->name;
		dirToVisit.parent = pRemoteDataObject->GetServerPath();
		dirToVisit.subdir = iter->name;
		m_dirsToVisit.push_back(dirToVisit);
	}
	m_operationMode = queueOnly ? recursive_addtoqueue : recursive_download;
	m_startDir = pRemoteDataObject->GetServerPath();

	NextOperation();

	return true;
}
