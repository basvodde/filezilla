#define FILELISTCTRL_INCLUDE_TEMPLATE_DEFINITION

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
#include "recursive_operation.h"
#include "edithandler.h"
#include "dragdropmanager.h"

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
			m_pRemoteListView->m_dropTarget = -1;
#ifdef __WXMSW__
			m_pRemoteListView->SetItemState(dropTarget, 0, wxLIST_STATE_DROPHILITED);
#else
			m_pRemoteListView->RefreshItem(dropTarget);
#endif
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
				if (index != -1 && m_pRemoteListView->m_fileData[index].flags != CComparableListing::fill)
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
			if (index != -1 && m_pRemoteListView->m_fileData[index].flags != CComparableListing::fill)
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

	int DisplayDropHighlight(wxPoint point)
	{
		int flags;
		int hit = m_pRemoteListView->HitTest(point, flags, 0);
		if (!(flags & wxLIST_HITTEST_ONITEM))
			hit = -1;

		if (hit != -1)
		{
			int index = m_pRemoteListView->GetItemIndex(hit);
			if (index == -1 || m_pRemoteListView->m_fileData[index].flags == CComparableListing::fill)
				hit = -1;
			else if (index != (int)m_pRemoteListView->m_pDirectoryListing->GetCount())
			{
				if (!(*m_pRemoteListView->m_pDirectoryListing)[index].dir)
					hit = -1;
				else
				{
					const CDragDropManager* pDragDropManager = CDragDropManager::Get();
					if (pDragDropManager && pDragDropManager->pDragSource == m_pRemoteListView)
					{
						if (m_pRemoteListView->GetItemState(hit, wxLIST_STATE_SELECTED))
							hit = -1;
					}
				}
			}
		}
		if (hit != m_pRemoteListView->m_dropTarget)
		{
			ClearDropHighlight();
			if (hit != -1)
			{
				m_pRemoteListView->m_dropTarget = hit;
#ifdef __WXMSW__
				m_pRemoteListView->SetItemState(hit, wxLIST_STATE_DROPHILITED, wxLIST_STATE_DROPHILITED);
#else
				m_pRemoteListView->RefreshItem(hit);
#endif
			}
		}
		return hit;
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

		const CServer* const pServer = m_pRemoteListView->m_pState->GetServer();
		wxASSERT(pServer);

		int hit = DisplayDropHighlight(wxPoint(x, y));
		const CDragDropManager* pDragDropManager = CDragDropManager::Get();

		if (hit == -1 && pDragDropManager &&
			pDragDropManager->remoteParent == m_pRemoteListView->m_pDirectoryListing->path &&
			*pServer == pDragDropManager->server)
			return wxDragNone;

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
		GetTextExtent(m_text, &m_textSize.x, &m_textSize.y);
	}

	void SetText(wxString text)
	{
		text = _T("<") + text + _T(">");
		if (text == m_text)
			return;

		m_text = text;

		GetTextExtent(m_text, &m_textSize.x, &m_textSize.y);
	}

	wxSize GetTextSize() const { return m_textSize; }

protected:
	wxString m_text;

	void OnPaint(wxPaintEvent& event)
	{
		wxPaintDC paintDc(this);

		paintDc.SetFont(GetFont());
		paintDc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));

		paintDc.DrawText(m_text, 0, 0);
	};

	wxSize m_textSize;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(CInfoText, wxWindow)
EVT_PAINT(CInfoText::OnPaint)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(CRemoteListView, CFileListCtrl<CGenericFileData>)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, CRemoteListView::OnItemActivated)
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
	EVT_MENU(XRCID("ID_EDIT"), CRemoteListView::OnMenuEdit)
END_EVENT_TABLE()

CRemoteListView::CRemoteListView(wxWindow* pParent, CState *pState, CQueueView* pQueue)
	: CFileListCtrl<CGenericFileData>(pParent, pState, pQueue),
	CSystemImageList(16),
	CStateEventHandler(pState)
{
	pState->RegisterHandler(this, STATECHANGE_REMOTE_DIR);
	pState->RegisterHandler(this, STATECHANGE_REMOTE_DIR_MODIFIED);
	pState->RegisterHandler(this, STATECHANGE_APPLYFILTER);

	m_dropTarget = -1;

	m_pInfoText = 0;
	m_pDirectoryListing = 0;

	const unsigned long widths[6] = { 80, 75, 80, 100, 80, 80 };

	AddColumn(_("Filename"), wxLIST_FORMAT_LEFT, widths[0]);
	AddColumn(_("Filesize"), wxLIST_FORMAT_RIGHT, widths[1]);
	AddColumn(_("Filetype"), wxLIST_FORMAT_LEFT, widths[2]);
	AddColumn(_("Last modified"), wxLIST_FORMAT_LEFT, widths[3]);
	AddColumn(_("Permissions"), wxLIST_FORMAT_LEFT, widths[4]);
	AddColumn(_("Owner / Group"), wxLIST_FORMAT_LEFT, widths[5]);
	LoadColumnSettings(OPTION_REMOTEFILELIST_COLUMN_WIDTHS, OPTION_REMOTEFILELIST_COLUMN_SHOWN, OPTION_REMOTEFILELIST_COLUMN_ORDER);

	InitSort(OPTION_REMOTEFILELIST_SORTORDER);

	m_dirIcon = GetIconIndex(dir);
	SetImageList(GetSystemImageList(), wxIMAGE_LIST_SMALL);

#ifdef __WXMSW__
	InitHeaderImageList();
#endif

	SetDirectoryListing(0);

	SetDropTarget(new CRemoteListViewDropTarget(this));

	InitDateFormat();

	EnablePrefixSearch(true);
}

CRemoteListView::~CRemoteListView()
{
	wxString str = wxString::Format(_T("%d %d"), m_sortDirection, m_sortColumn);
	COptions::Get()->SetOption(OPTION_REMOTEFILELIST_SORTORDER, str);
}

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
	const unsigned int removed = m_pDirectoryListing->GetCount() - pDirectoryListing->GetCount();
	wxASSERT(removed);
	wxASSERT(!IsComparing());

	std::list<unsigned int> removedItems;

	// Get indexes of the removed items in the listing
	unsigned int j = 0;
	unsigned int i = 0;
	while (i < pDirectoryListing->GetCount() && j < m_pDirectoryListing->GetCount())
	{
		const CDirentry& oldEntry = (*m_pDirectoryListing)[j];
		const wxString& oldName = oldEntry.name;
		const wxString& newName = (*pDirectoryListing)[i].name;
		if (oldName == newName)
		{
			i++;
			j++;
			continue;
		}

		if (m_pFilelistStatusBar)
		{
			if (oldEntry.dir)
				m_pFilelistStatusBar->RemoveDirectory();
			else
				m_pFilelistStatusBar->RemoveFile(oldEntry.size);
		}

		removedItems.push_back(j++);
	}
	for (; j < m_pDirectoryListing->GetCount(); j++)
		removedItems.push_back(j);

	wxASSERT(removedItems.size() == removed);

	std::list<int> selectedItems;

	// Number of items left to remove
	unsigned int toRemove = removed;

	std::list<int> removedIndexes;

	const int size = m_indexMapping.size();
	for (int i = size - 1; i >= 0; i--)
	{
		bool removed = false;

		unsigned int& index = m_indexMapping[i];

		// j is the offset the index has to be adjusted
		int j = 0;
		for (std::list<unsigned int>::const_iterator iter = removedItems.begin(); iter != removedItems.end(); iter++, j++)
		{
			if (*iter > index)
				break;

			if (*iter == index)
			{
				removedIndexes.push_back(i);
				removed = true;
				toRemove--;
				break;
			}
		}

		// Get old selection
		bool isSelected = GetItemState(i, wxLIST_STATE_SELECTED) != 0;
		
		// Update statusbar info
		if (isSelected && removed && m_pFilelistStatusBar)
		{
			if ((*m_pDirectoryListing)[index].dir)
				m_pFilelistStatusBar->UnselectDirectory();
			else
				m_pFilelistStatusBar->UnselectFile((*m_pDirectoryListing)[index].size);
		}

		// Update index
		index -= j;

		// Update selections
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
			if (!needSelection && (toRemove || removed))
				SetSelection(i, false);

			if (!removed)
				selectedItems.push_back(i - toRemove);
		}
		else if (needSelection)
			SetSelection(i, true);
	}

	// Erase file data
	for (std::list<unsigned int>::reverse_iterator iter = removedItems.rbegin(); iter != removedItems.rend(); iter++)
	{
		m_fileData.erase(m_fileData.begin() + *iter);
	}

	// Erase indexes
	wxASSERT(!toRemove);
	wxASSERT(removedIndexes.size() == removed);
	for (std::list<int>::iterator iter = removedIndexes.begin(); iter != removedIndexes.end(); iter++)
	{
		m_indexMapping.erase(m_indexMapping.begin() + *iter);
	}

	wxASSERT(m_indexMapping.size() == pDirectoryListing->GetCount() + 1);

	m_pDirectoryListing = pDirectoryListing;

	SetItemCount(m_indexMapping.size());
}

bool CRemoteListView::UpdateDirectoryListing(const CDirectoryListing *pDirectoryListing)
{
	wxASSERT(!IsComparing());

	const int unsure = pDirectoryListing->m_hasUnsureEntries & ~(CDirectoryListing::unsure_unknown);

	if (!unsure)
		return false;

	if (unsure & CDirectoryListing::unsure_invalid)
		return false;

	if (!(unsure & ~(CDirectoryListing::unsure_dir_changed | CDirectoryListing::unsure_file_changed)))
	{
		if (m_sortColumn && m_sortColumn != 2)
		{
			// If not sorted by file or type, changing file attributes can influence
			// sort order.
			return false;
		}
		wxASSERT(pDirectoryListing->GetCount() == m_pDirectoryListing->GetCount());
		if (pDirectoryListing->GetCount() != m_pDirectoryListing->GetCount())
			return false;
		
		m_pDirectoryListing = pDirectoryListing;

		// We don't have to do anything
		return true;
	}

	if (unsure & (CDirectoryListing::unsure_dir_added | CDirectoryListing::unsure_file_added))
	{
		// Todo: Would be nice to handle this too
		return false;
	}

	wxASSERT(pDirectoryListing->GetCount() < m_pDirectoryListing->GetCount());
	UpdateDirectoryListing_Removed(pDirectoryListing);
	return true;
}

void CRemoteListView::SetDirectoryListing(const CDirectoryListing *pDirectoryListing, bool modified /*=false*/)
{
#ifdef __WXMSW__
	EndEditLabel(true);
#endif

	bool reset = false;
	if (!pDirectoryListing || !m_pDirectoryListing)
		reset = true;
	else if (m_pDirectoryListing->path != pDirectoryListing->path)
		reset = true;
	else if (m_pDirectoryListing->m_firstListTime == pDirectoryListing->m_firstListTime && !IsComparing()
#ifndef __WXDEBUG__
		&& m_pDirectoryListing->GetCount() > 200
#endif
		)
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

			return;
		}
	}

	wxString prevFocused;
	std::list<wxString> selectedNames;
	if (reset)
	{
		ResetSearchPrefix();

		if (IsComparing())
			ExitComparisonMode();

		// Clear selection
		int item = -1;
		while (true)
		{
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			if (item == -1)
				break;
			SetSelection(item, false);
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

	if (m_pFilelistStatusBar)
		m_pFilelistStatusBar->UnselectAll();

	m_pDirectoryListing = pDirectoryListing;

	m_fileData.clear();
	m_indexMapping.clear();

	wxLongLong totalSize;
	int unknown_sizes = 0;
	int totalFileCount = 0;
	int totalDirCount = 0;

	bool eraseBackground = false;
	if (m_pDirectoryListing)
	{
		SetInfoText();

		m_indexMapping.push_back(m_pDirectoryListing->GetCount());

		CFilterManager filter;
		for (unsigned int i = 0; i < m_pDirectoryListing->GetCount(); i++)
		{
			const CDirentry& entry = (*m_pDirectoryListing)[i];
			CGenericFileData data;
			data.flags = normal;
			data.icon = entry.dir ? m_dirIcon : -2;
			m_fileData.push_back(data);

			if (filter.FilenameFiltered(entry.name, entry.dir, entry.size, false, 0))
				continue;

			if (entry.dir)
				totalDirCount++;
			else
			{
				if (entry.size == -1)
					unknown_sizes++;
				else
					totalSize += entry.size;
				totalFileCount++;
			}

			m_indexMapping.push_back(i);
		}

		CGenericFileData data;
		data.flags = normal;
		data.icon = m_dirIcon;
		m_fileData.push_back(data);
	}
	else
	{
		eraseBackground = true;
		SetInfoText();
	}

	if (m_pFilelistStatusBar)
		m_pFilelistStatusBar->SetDirectoryContents(totalFileCount, totalDirCount, totalSize, unknown_sizes);

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

	if (!IsComparing())
	{
		if ((unsigned int)GetItemCount() > m_indexMapping.size())
			eraseBackground = true;
		if ((unsigned int)GetItemCount() != m_indexMapping.size())
			SetItemCount(m_indexMapping.size());

		if (GetItemCount() && reset)
		{
			EnsureVisible(0);
			SetItemState(0, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
		}
	}

	SortList(-1, -1, false);

	if (IsComparing())
	{
		m_originalIndexMapping.clear();
		RefreshComparison();
		ReselectItems(selectedNames, prevFocused);
	}
	else
	{
		ReselectItems(selectedNames, prevFocused);
		Refresh(eraseBackground);
	}
}

// Helper classes for fast sorting using std::sort
// -----------------------------------------------

class CRemoteListViewSort : public CListViewSort
{
public:
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

template<class T> class CReverseSort : public T
{
public:
	CReverseSort(const CDirectoryListing* const pDirectoryListing, std::vector<CGenericFileData>& fileData, enum CRemoteListViewSort::DirSortMode dirSortMode, CRemoteListView* const pRemoteListView)
		: T(pDirectoryListing, fileData, dirSortMode, pRemoteListView)
	{
	}

	inline bool operator()(int a, int b) const
	{
		return T::operator()(b, a);
	}
};

class CRemoteListViewSortName : public CRemoteListViewSort
{
public:
	CRemoteListViewSortName(const CDirectoryListing* const pDirectoryListing, std::vector<CGenericFileData>& fileData, enum DirSortMode dirSortMode, CRemoteListView* const pRemoteListView)
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
typedef CReverseSort<CRemoteListViewSortName> CRemoteListViewSortName_Reverse;

class CRemoteListViewSortSize : public CRemoteListViewSort
{
public:
	CRemoteListViewSortSize(const CDirectoryListing* const pDirectoryListing, std::vector<CGenericFileData>& fileData, enum DirSortMode dirSortMode, CRemoteListView* const pRemoteListView)
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
typedef CReverseSort<CRemoteListViewSortSize> CRemoteListViewSortSize_Reverse;

class CRemoteListViewSortType : public CRemoteListViewSort
{
public:
	CRemoteListViewSortType(const CDirectoryListing* const pDirectoryListing, std::vector<CGenericFileData>& fileData, enum DirSortMode dirSortMode, CRemoteListView* const pRemoteListView)
		: CRemoteListViewSort(pDirectoryListing, dirSortMode), m_pRemoteListView(pRemoteListView), m_fileData(fileData)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = (*m_pDirectoryListing)[a];
		const CDirentry& data2 = (*m_pDirectoryListing)[b];

		CMP(CmpDir, data1, data2);

		CGenericFileData &type1 = m_fileData[a];
		CGenericFileData &type2 = m_fileData[b];
		if (type1.fileType == _T(""))
			type1.fileType = m_pRemoteListView->GetType(data1.name, data1.dir);
		if (type2.fileType == _T(""))
			type2.fileType = m_pRemoteListView->GetType(data2.name, data2.dir);

		CMP(CmpStringNoCase, type1.fileType, type2.fileType);

		CMP_LESS(CmpName, data1, data2);
	}

protected:
	CRemoteListView* const m_pRemoteListView;
	std::vector<CGenericFileData>& m_fileData;
};
typedef CReverseSort<CRemoteListViewSortType> CRemoteListViewSortType_Reverse;

class CRemoteListViewSortTime : public CRemoteListViewSort
{
public:
	CRemoteListViewSortTime(const CDirectoryListing* const pDirectoryListing, std::vector<CGenericFileData>& fileData, enum DirSortMode dirSortMode, CRemoteListView* const pRemoteListView)
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
typedef CReverseSort<CRemoteListViewSortTime> CRemoteListViewSortTime_Reverse;

class CRemoteListViewSortPermissions : public CRemoteListViewSort
{
public:
	CRemoteListViewSortPermissions(const CDirectoryListing* const pDirectoryListing, std::vector<CGenericFileData>& fileData, enum DirSortMode dirSortMode, CRemoteListView* const pRemoteListView)
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
typedef CReverseSort<CRemoteListViewSortPermissions> CRemoteListViewSortPermissions_Reverse;

void CRemoteListView::OnItemActivated(wxListEvent &event)
{
	if (!m_pState->IsRemoteIdle())
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

		int index = GetItemIndex(item);
		if (index == -1 || m_fileData[index].flags == fill)
			continue;

		count++;

		if (!item)
			back = true;
	}
	if (!count)
	{
		wxBell();
		return;
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
		if (m_fileData[index].flags == fill)
			return;

		const CDirentry& entry = (*m_pDirectoryListing)[index];
		const wxString& name = entry.name;

		if (entry.dir)
		{
			if (IsComparing())
				ExitComparisonMode();
			m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(m_pDirectoryListing->path, name));
		}
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
	{
		if (IsComparing())
			ExitComparisonMode();

		m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(m_pDirectoryListing->path, _T("..")));
	}
}

void CRemoteListView::OnContextMenu(wxContextMenuEvent& event)
{
	if (GetEditControl())
	{
		event.Skip();
		return;
	}

	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_REMOTEFILELIST"));
	if (!pMenu)
		return;

	if (!m_pState->IsRemoteConnected() || !m_pState->IsRemoteIdle())
	{
		pMenu->Enable(XRCID("ID_DOWNLOAD"), false);
		pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
		pMenu->Enable(XRCID("ID_MKDIR"), false);
		pMenu->Enable(XRCID("ID_DELETE"), false);
		pMenu->Enable(XRCID("ID_RENAME"), false);
		pMenu->Enable(XRCID("ID_CHMOD"), false);
		pMenu->Enable(XRCID("ID_EDIT"), false);
	}
	else if ((GetItemCount() && GetItemState(0, wxLIST_STATE_SELECTED)) ||
		GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) == -1)
	{
		pMenu->Enable(XRCID("ID_DOWNLOAD"), false);
		pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
		pMenu->Enable(XRCID("ID_DELETE"), false);
		pMenu->Enable(XRCID("ID_RENAME"), false);
		pMenu->Enable(XRCID("ID_CHMOD"), false);
		pMenu->Enable(XRCID("ID_EDIT"), false);
	}
	else
	{
		int count = 0;
		int fillCount = 0;
		bool selectedDir = false;
		int item = -1;;
		while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
		{
			int index = GetItemIndex(item);
			if (index == -1)
				continue;
			count++;
			if (m_fileData[index].flags == fill)
			{
				fillCount++;
				continue;
			}
			if ((*m_pDirectoryListing)[index].dir)
				selectedDir = true;
		}
		if (!count || fillCount == count)
		{
			pMenu->Enable(XRCID("ID_DOWNLOAD"), false);
			pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
			pMenu->Enable(XRCID("ID_DELETE"), false);
			pMenu->Enable(XRCID("ID_RENAME"), false);
			pMenu->Enable(XRCID("ID_CHMOD"), false);
			pMenu->Enable(XRCID("ID_EDIT"), false);
		}
		else
		{
			if (selectedDir)
				pMenu->Enable(XRCID("ID_EDIT"), false);
			if (count > 1)
				pMenu->Enable(XRCID("ID_RENAME"), false);

			const wxString& localDir = m_pState->GetLocalDir();
			if (!m_pState->LocalDirIsWriteable(localDir))
			{
				pMenu->Enable(XRCID("ID_DOWNLOAD"), false);
				pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
			}
		}
	}

	PopupMenu(pMenu);
	delete pMenu;
}

void CRemoteListView::OnMenuDownload(wxCommandEvent& event)
{
	// Make sure selection is valid
	bool idle = m_pState->IsRemoteIdle();

	const wxString& localDir = m_pState->GetLocalDir();
	if (!m_pState->LocalDirIsWriteable(localDir))
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

		if (!item)
		{
			wxBell();
			return;
		}

		int index = GetItemIndex(item);
		if (index == -1)
			continue;
		if (m_fileData[index].flags == fill)
			continue;
		if ((*m_pDirectoryListing)[index].dir && !idle)
		{
			wxBell();
			return;
		}
	}

	TransferSelectedFiles(localDir, event.GetId() == XRCID("ID_ADDTOQUEUE"));
}

void CRemoteListView::TransferSelectedFiles(const wxString& localDir, bool queueOnly)
{
	bool idle = m_pState->IsRemoteIdle();

	CRecursiveOperation* pRecursiveOperation = m_pState->GetRecursiveOperationHandler();
	wxASSERT(pRecursiveOperation);

	wxASSERT(m_pState->LocalDirIsWriteable(localDir));

	bool startRecursive = false;
	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		int index = GetItemIndex(item);
		if (index == -1)
			continue;
		if (m_fileData[index].flags == fill)
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
			if (!idle)
				continue;
			wxFileName fn = wxFileName(localDir, _T(""));
			fn.AppendDir(name);
			CServerPath remotePath = m_pDirectoryListing->path;
			if (remotePath.AddSegment(name))
			{
				//m_pQueue->QueueFolder(event.GetId() == XRCID("ID_ADDTOQUEUE"), true, fn.GetFullPath(), remotePath, *pServer);
				pRecursiveOperation->AddDirectoryToVisit(m_pDirectoryListing->path, name, fn.GetFullPath());
				startRecursive = true;
			}
		}
		else
		{
			wxFileName fn = wxFileName(localDir, name);
			m_pQueue->QueueFile(queueOnly, true, fn.GetFullPath(), name, m_pDirectoryListing->path, *pServer, entry.size);
		}
	}

	if (startRecursive)
	{
		if (IsComparing())
			ExitComparisonMode();
		pRecursiveOperation->StartRecursiveOperation(queueOnly ? CRecursiveOperation::recursive_addtoqueue : CRecursiveOperation::recursive_download, m_pDirectoryListing->path);
	}
}

void CRemoteListView::OnMenuMkdir(wxCommandEvent& event)
{
	if (!m_pDirectoryListing || !m_pState->IsRemoteIdle())
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

	const CServerPath oldPath = m_pDirectoryListing->path;

	if (dlg.ShowModal() != wxID_OK)
		return;

	if (!m_pDirectoryListing || oldPath != m_pDirectoryListing->path ||
		!m_pState->IsRemoteIdle())
	{
		wxBell();
		return;
	}

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
	if (!m_pState->IsRemoteIdle())
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

	CRecursiveOperation* pRecursiveOperation = m_pState->GetRecursiveOperationHandler();
	wxASSERT(pRecursiveOperation);

	std::list<wxString> filesToDelete;

	bool startRecursive = false;
	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		int index = GetItemIndex(item);
		if (index == -1)
			continue;
		if (m_fileData[index].flags == fill)
			continue;

		const CDirentry& entry = (*m_pDirectoryListing)[index];
		const wxString& name = entry.name;

		if (entry.dir)
		{
			CServerPath remotePath = m_pDirectoryListing->path;
			if (remotePath.AddSegment(name))
			{
				pRecursiveOperation->AddDirectoryToVisit(m_pDirectoryListing->path, name);
				startRecursive = true;
			}
		}
		else
			filesToDelete.push_back(name);
	}

	if (!filesToDelete.empty())
		m_pState->m_pCommandQueue->ProcessCommand(new CDeleteCommand(m_pDirectoryListing->path, filesToDelete));

	if (startRecursive)
	{
		if (IsComparing())
			ExitComparisonMode();
		pRecursiveOperation->StartRecursiveOperation(CRecursiveOperation::recursive_delete, m_pDirectoryListing->path);
	}
}

void CRemoteListView::OnMenuRename(wxCommandEvent& event)
{
	if (!m_pState->IsRemoteIdle())
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

	int index = GetItemIndex(item);
	if (index == -1 || m_fileData[index].flags == fill)
	{
		wxBell();
		return;
	}

	EditLabel(item);
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
	else if (code == WXK_BACK)
	{
		if (!m_pState->IsRemoteIdle())
		{
			wxBell();
			return;
		}

		if (!m_pDirectoryListing)
		{
			wxBell();
			return;
		}

		if (IsComparing())
			ExitComparisonMode();

		m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(m_pDirectoryListing->path, _T("..")));
	}
	else
		event.Skip();
}

void CRemoteListView::OnKeyDown(wxKeyEvent& event)
{
	const int code = event.GetKeyCode();
	const int mods = event.GetModifiers();
	if (code == 'A' && (mods == wxMOD_CMD || mods == (wxMOD_CONTROL | wxMOD_META)))
	{
		for (unsigned int i = 1; i < m_indexMapping.size(); i++)
		{
			int index = GetItemIndex(i);
			if (index != -1 && m_fileData[index].flags != fill)
				SetSelection(i, true);
			else
				SetSelection(i, false);
		}
		if (m_pFilelistStatusBar)
			m_pFilelistStatusBar->SelectAll();
	}
	else
		OnChar(event);
}

void CRemoteListView::OnBeginLabelEdit(wxListEvent& event)
{
	if (!m_pState->IsRemoteIdle())
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
	if (index == -1 || m_fileData[index].flags == fill)
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

	if (!m_pState->IsRemoteIdle())
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
	if (index == -1 || m_fileData[index].flags == fill)
	{
		wxBell();
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
	if (!m_pState->IsRemoteConnected() || !m_pState->IsRemoteIdle())
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
		if (m_fileData[index].flags == fill)
			continue;

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
			if (entry.permissions[6] == 's')
				permissions[5] = 2;
			if (entry.permissions[9] == 't')
				permissions[8] = 2;

		}
	}
	if (!dirCount && !fileCount)
	{
		wxBell();
		return;
	}

	for (int i = 0; i < 9; i++)
		if (permissions[i] == -1)
			permissions[i] = 0;

	CChmodDialog* pChmodDlg = new CChmodDialog;
	if (!pChmodDlg->Create(this, fileCount, dirCount, name, permissions))
	{
		pChmodDlg->Destroy();
		pChmodDlg = 0;
		return;
	}

	if (pChmodDlg->ShowModal() != wxID_OK)
	{
		pChmodDlg->Destroy();
		pChmodDlg = 0;
		return;
	}

	// State may have changed while chmod dialog was shown
	if (!m_pState->IsRemoteConnected() || !m_pState->IsRemoteIdle())
	{
		pChmodDlg->Destroy();
		pChmodDlg = 0;
		wxBell();
		return;
	}

	const int applyType = pChmodDlg->GetApplyType();

	CRecursiveOperation* pRecursiveOperation = m_pState->GetRecursiveOperationHandler();
	wxASSERT(pRecursiveOperation);

	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item)
		{
			pChmodDlg->Destroy();
			pChmodDlg = 0;
			return;
		}

		int index = GetItemIndex(item);
		if (index == -1)
		{
			pChmodDlg->Destroy();
			pChmodDlg = 0;
			return;
		}
		if (m_fileData[index].flags == fill)
			continue;

		const CDirentry& entry = (*m_pDirectoryListing)[index];

		if (!applyType ||
			(!entry.dir && applyType == 1) ||
			(entry.dir && applyType == 2))
		{
			char permissions[9];
			bool res = pChmodDlg->ConvertPermissions(entry.permissions, permissions);
			wxString newPerms = pChmodDlg->GetPermissions(res ? permissions : 0);

			m_pState->m_pCommandQueue->ProcessCommand(new CChmodCommand(m_pDirectoryListing->path, entry.name, newPerms));
		}

		if (pChmodDlg->Recursive() && entry.dir)
			pRecursiveOperation->AddDirectoryToVisit(m_pDirectoryListing->path, entry.name);
	}

	if (pChmodDlg->Recursive())
	{
		if (IsComparing())
			ExitComparisonMode();

		pRecursiveOperation->SetChmodDialog(pChmodDlg);
		pRecursiveOperation->StartRecursiveOperation(CRecursiveOperation::recursive_chmod, m_pDirectoryListing->path);

		// Refresh listing. This gets done implicitely by the recursive operation, so
		// only it if not recursing.
		if (pRecursiveOperation->GetOperationMode() != CRecursiveOperation::recursive_chmod)
			m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(m_pDirectoryListing->path));
	}
	else
	{
		pChmodDlg->Destroy();
		m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(m_pDirectoryListing->path));
	}

}

void CRemoteListView::ApplyCurrentFilter()
{
	CFilterManager filter;

	if (!filter.HasSameLocalAndRemoteFilters() && IsComparing())
		ExitComparisonMode();

	if (m_fileData.size() <= 1)
		return;

	wxString focused;
	std::list<wxString> selectedNames = RememberSelectedItems(focused);

	if (m_pFilelistStatusBar)
		m_pFilelistStatusBar->UnselectAll();

	m_indexMapping.clear();
	const unsigned int count = m_pDirectoryListing->GetCount();
	m_indexMapping.push_back(count);
	for (unsigned int i = 0; i < count; i++)
	{
		const CDirentry& entry = (*m_pDirectoryListing)[i];
		if (!filter.FilenameFiltered(entry.name, entry.dir, entry.size, false, 0))
			m_indexMapping.push_back(i);
	}

	SetItemCount(m_indexMapping.size());

	SortList(-1, -1, false);

	if (IsComparing())
	{
		m_originalIndexMapping.clear();
		RefreshComparison();
	}

	ReselectItems(selectedNames, focused);
	if (!IsComparing())
		Refresh();
}

std::list<wxString> CRemoteListView::RememberSelectedItems(wxString& focused)
{
	wxASSERT(GetItemCount() == (int)m_indexMapping.size());
	std::list<wxString> selectedNames;
	// Remember which items were selected
	int item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while (item != -1)
	{
		SetSelection(item, false);
		if (!item)
		{
			selectedNames.push_back(_T(".."));
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			continue;
		}
		int index = GetItemIndex(item);
		if (index == -1 || m_fileData[index].flags == fill)
		{
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			continue;
		}
		const CDirentry& entry = (*m_pDirectoryListing)[index];

		if (entry.dir)
			selectedNames.push_back(_T("d") + entry.name);
		else
			selectedNames.push_back(_T("-") + entry.name);

		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}

	item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
	if (item != -1)
	{
		int index = GetItemIndex(item);
		if (index != -1 && m_fileData[index].flags != fill)
		{
			if (!item)
				focused = _T("..");
			else
				focused = (*m_pDirectoryListing)[index].name;
		}

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
	}

	if (selectedNames.empty())
	{
		if (focused == _T(""))
			return;

		for (unsigned int i = 1; i < m_indexMapping.size(); i++)
		{
			const int index = m_indexMapping[i];
			if (m_fileData[index].flags == fill)
				continue;

			if ((*m_pDirectoryListing)[index].name == focused)
			{
				SetItemState(i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
				return;
			}
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
	unsigned int i = 0;
	for (std::list<wxString>::const_iterator iter = selectedNames.begin(); iter != selectedNames.end(); iter++)
	{
		while (++i < m_indexMapping.size())
		{
			int index = GetItemIndex(i);
			if (index == -1 || m_fileData[index].flags == fill)
				continue;
			const CDirentry& entry = (*m_pDirectoryListing)[index];
			if (entry.name == focused)
			{
				SetItemState(i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
				focused = _T("");
			}
			if (entry.dir && *iter == (_T("d") + entry.name))
			{
				if (firstSelected == -1)
					firstSelected = i;
				if (m_pFilelistStatusBar)
					m_pFilelistStatusBar->SelectDirectory();
                SetSelection(i, true);
				break;
			}
			else if (*iter == (_T("-") + entry.name))
			{
				if (firstSelected == -1)
					firstSelected = i;
				if (m_pFilelistStatusBar)
					m_pFilelistStatusBar->SelectFile(entry.size);
				SetSelection(i, true);
				break;
			}
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

	wxSize size = m_pInfoText->GetTextSize();

	if (!m_indexMapping.size())
		rect.y = 60;
	else
	{
		wxRect itemRect;
		GetItemRect(0, itemRect);
		rect.y = wxMax(60, itemRect.GetBottom() + 1);
	}
	rect.x = rect.x + (rect.width - size.x) / 2;
	rect.width = size.x;
	rect.height = size.y;

	m_pInfoText->SetSize(rect);
	m_pInfoText->Refresh(false);
}

void CRemoteListView::OnStateChange(enum t_statechange_notifications notification, const wxString& data)
{
	wxASSERT(m_pState);
	if (notification == STATECHANGE_REMOTE_DIR)
		SetDirectoryListing(m_pState->GetRemoteDir(), false);
	else if (notification == STATECHANGE_REMOTE_DIR_MODIFIED)
		SetDirectoryListing(m_pState->GetRemoteDir(), true);
	else
	{
		wxASSERT(notification == STATECHANGE_APPLYFILTER);
		ApplyCurrentFilter();
	}
}

void CRemoteListView::SetInfoText()
{
	wxString text;
	if (!IsComparing())
	{
		if (!m_pDirectoryListing)
			text = _("Not connected to any server");
		else if (m_pDirectoryListing->m_failed)
			text = _("Directory listing failed");
		else if (!m_pDirectoryListing->GetCount())
			text = _("Empty directory listing");
	}

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

	m_pInfoText->SetText(text);
	RepositionInfoText();
}

void CRemoteListView::OnBeginDrag(wxListEvent& event)
{
	if (!m_pState->IsRemoteIdle())
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
	int count = 0;
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
		if (index == -1 || m_fileData[index].flags == fill)
			continue;
		if ((*m_pDirectoryListing)[index].dir && !idle)
		{
			// Drag could result in recursive operation, don't allow at this point
			wxBell();
			return;
		}
		count++;
	}
	if (!count)
	{
		wxBell();
		return;
	}

	wxDataObjectComposite object;

	const CServer* const pServer = m_pState->GetServer();
	if (!pServer)
		return;
	const CServer server = *pServer;
	const CServerPath path = m_pDirectoryListing->path;

	CRemoteDataObject *pRemoteDataObject = new CRemoteDataObject(*pServer, m_pDirectoryListing->path);

	CDragDropManager* pDragDropManager = CDragDropManager::Init();
	pDragDropManager->pDragSource = this;
	pDragDropManager->server = server;
	pDragDropManager->remoteParent = m_pDirectoryListing->path;

	// Add files to remote data object
	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		int index = GetItemIndex(item);
		if (index == -1 || m_fileData[index].flags == fill)
			continue;
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

	int res = source.DoDragDrop();

	pDragDropManager->Release();

	if (res != wxDragCopy)
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
			if (!m_pState->IsRemoteIdle() ||
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
				if (index == -1 || m_fileData[index].flags == fill)
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
	if (!m_pState->IsRemoteIdle())
		return false;

	CRecursiveOperation* pRecursiveOperation = m_pState->GetRecursiveOperationHandler();
	wxASSERT(pRecursiveOperation);

	const std::list<CRemoteDataObject::t_fileInfo>& files = pRemoteDataObject->GetFiles();
	for (std::list<CRemoteDataObject::t_fileInfo>::const_iterator iter = files.begin(); iter != files.end(); iter++)
	{
		if (!iter->dir)
			continue;

		pRecursiveOperation->AddDirectoryToVisit(pRemoteDataObject->GetServerPath(), iter->name, path + iter->name);
	}

	if (IsComparing())
		ExitComparisonMode();

	pRecursiveOperation->StartRecursiveOperation(queueOnly ? CRecursiveOperation::recursive_addtoqueue : CRecursiveOperation::recursive_download, pRemoteDataObject->GetServerPath());

	return true;
}

void CRemoteListView::InitDateFormat()
{
	const wxString& dateFormat = COptions::Get()->GetOption(OPTION_DATE_FORMAT);
	const wxString& timeFormat = COptions::Get()->GetOption(OPTION_TIME_FORMAT);

	if (dateFormat == _T("1"))
		m_dateFormat = _T("%Y-%m-%d");
	else if (dateFormat[0] == '2')
		m_dateFormat = dateFormat.Mid(1);
	else
		m_dateFormat = _T("%x");

	if (timeFormat == _T("1"))
		m_timeFormat = m_dateFormat + _T(" %H:%M");
	else if (timeFormat[0] == '2')
		m_timeFormat = m_dateFormat + _T(" ") + timeFormat.Mid(1);
	else
		m_timeFormat = m_dateFormat + _T(" %X");
}

void CRemoteListView::OnMenuEdit(wxCommandEvent& event)
{
	if (!m_pState->IsRemoteConnected())
	{
		wxBell();
		return;
	}

	long item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item < 1)
	{
		wxBell();
		return;
	}

	if (GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) != -1)
	{
		wxBell();
		return;
	}

	int index = GetItemIndex(item);
	if (index == -1 || m_fileData[index].flags == fill)
	{
		wxBell();
		return;
	}

	const CDirentry entry = (*m_pDirectoryListing)[index];
	if (entry.dir)
	{
		wxBell();
		return;
	}

	CEditHandler* pEditHandler = CEditHandler::Get();
	if (!pEditHandler)
	{
		wxBell();
		return;
	}

	const wxString& localDir = pEditHandler->GetLocalDirectory();
	if (localDir == _T(""))
	{
		wxMessageBox(_("Could not get temporary directory to download file into."), _("Cannot edit file"), wxICON_STOP);
		return;
	}

	bool dangerous = false;
	if (!pEditHandler->CanOpen(CEditHandler::remote, entry.name, dangerous))
	{
		wxMessageBox(_("Selected file cannot be opened.\nNo default editor has been set or filetype association is missing or incorrect."), _("Cannot edit file"), wxICON_STOP);
		return;
	}
	if (dangerous)
	{
		int res = wxMessageBox(_("The selected file would be executed directly.\nThis can be dangerous and damage your system.\nDo you really want to continue?"), _("Dangerous filetype"), wxICON_EXCLAMATION | wxYES_NO);
		if (res != wxYES)
		{
			wxBell();
			return;
		}
	}

	CEditHandler::fileState state = pEditHandler->GetFileState(CEditHandler::remote, entry.name);
	switch (state)
	{
	case CEditHandler::download:
	case CEditHandler::upload:
	case CEditHandler::upload_and_remove:
	case CEditHandler::upload_and_remove_failed:
		wxMessageBox(_("A file with that name is already being transferred."), _("Cannot view / edit selected file"), wxICON_EXCLAMATION);
		return;
	case CEditHandler::removing:
		if (!pEditHandler)
		{
			wxMessageBox(_("A file with that name is still being edited. Please close it and try again."), _("Selected file already opened."), wxICON_EXCLAMATION);
			return;
		}
		break;
	case CEditHandler::edit:
		{
			int res = wxMessageBox(_("A file with that name is already being edited. Discard old file and download the new file?"), _("Selected file already being edited"), wxICON_QUESTION | wxYES_NO);
			if (res != wxYES)
			{
				wxBell();
				return;
			}
			if (!pEditHandler->Remove(CEditHandler::remote, entry.name))
			{
				wxMessageBox(_("The selected file is still opened in some other program, please close it."), _("Selected file still being edited"), wxICON_EXCLAMATION);
				return;
			}
		}
		break;
	default:
		break;
	}

	if (!pEditHandler->AddFile(CEditHandler::remote, entry.name, m_pDirectoryListing->path, *m_pState->GetServer()))
	{
		wxFAIL;
		wxBell();
		return;
	}

	wxFileName fn = wxFileName(localDir, entry.name);
	m_pQueue->QueueFile(false, true, fn.GetFullPath(), entry.name, m_pDirectoryListing->path, *m_pState->GetServer(), entry.size, CEditHandler::remote);
}

#ifdef __WXDEBUG__
void CRemoteListView::ValidateIndexMapping()
{
	// This ensures that the index mapping is a bijection.
    // Beware:
	// - NO filter may be used!
	// - Doesn't work in comparison mode

	char* buffer = new char[m_pDirectoryListing->GetCount() + 1];
	memset(buffer, 0, m_pDirectoryListing->GetCount() + 1);

	// Injectivity
	for (unsigned int i = 0; i < m_indexMapping.size(); i++)
	{
		unsigned int item = m_indexMapping[i];
		if (item > m_pDirectoryListing->GetCount())
		{
			int *x = 0;
			*x = 0;
		}
		if (buffer[item])
		{
			int *x = 0;
			*x = 0;
		}

		buffer[item] = 1;
	}

	// Surjectivity
	for (unsigned int i = 0; i < m_pDirectoryListing->GetCount() + 1; i++)
	{
		wxASSERT(buffer[i] != 0);
	}

	delete [] buffer;
}
#endif

bool CRemoteListView::CanStartComparison(wxString* pError)
{
	if (!m_pDirectoryListing)
	{
		if (pError)
			*pError = _("Cannot compare directories, not connected to a server.");
		return false;
	}

	return true;
}

void CRemoteListView::StartComparison()
{
	if (m_sortDirection || m_sortColumn)
	{
		wxASSERT(m_originalIndexMapping.empty());
		SortList(0, 0);
	}

	ComparisonRememberSelections();

	if (m_originalIndexMapping.empty())
		m_originalIndexMapping.swap(m_indexMapping);
	else
		m_indexMapping.clear();

	m_comparisonIndex = -1;

	const CGenericFileData& last = m_fileData[m_fileData.size() - 1];
	if (last.flags != fill)
	{
		CGenericFileData data;
		data.icon = -1;
		data.flags = fill;
		m_fileData.push_back(data);
	}
}

bool CRemoteListView::GetNextFile(wxString& name, bool& dir, wxLongLong& size, wxDateTime& date, bool &hasTime)
{
	if (++m_comparisonIndex >= (int)m_originalIndexMapping.size())
		return false;

	const unsigned int index = m_originalIndexMapping[m_comparisonIndex];
	if (index >= m_fileData.size())
		return false;

	if (index == m_pDirectoryListing->GetCount())
	{
		name = _T("..");
		dir = true;
		size = -1;
		return true;
	}

	const CDirentry& entry = (*m_pDirectoryListing)[index];

	name = entry.name;
	dir = entry.dir;
	size = entry.size;
	if (entry.hasDate)
	{
		date = entry.time;
		hasTime = entry.hasTime;
	}
	else
	{
		date = wxDateTime();
		hasTime = false;
	}

	return true;
}

void CRemoteListView::FinishComparison()
{
	SetInfoText();

	SetItemCount(m_indexMapping.size());

	ComparisonRestoreSelections();

	Refresh();
}

wxListItemAttr* CRemoteListView::OnGetItemAttr(long item) const
{
	CRemoteListView *pThis = const_cast<CRemoteListView *>(this);
	int index = GetItemIndex(item);

	if (index == -1)
		return 0;

#ifndef __WXMSW__
	if (item == m_dropTarget)
		return &pThis->m_dropHighlightAttribute;
#endif

	const CGenericFileData& data = m_fileData[index];

	switch (data.flags)
	{
	case different:
		return &pThis->m_comparisonBackgrounds[0];
	case lonely:
		return &pThis->m_comparisonBackgrounds[1];
	case newer:
		return &pThis->m_comparisonBackgrounds[2];
	default:
		return 0;
	}

	return 0;
}

// Defined in LocalListView.cpp
extern wxString FormatSize(const wxLongLong& size, bool add_bytes_suffix = false);

// Filenames on VMS systems have a revision suffix, e.g.
// foo.bar;1
// foo.bar;2
// foo.bar;3
wxString StripVMSRevision(const wxString& name)
{
	int pos = name.Find(';', true);
	if (pos < 1)
		return name;

	const int len = name.Len();
	if (pos == len - 1)
		return name;

	int p = pos;
	while (++p < len)
	{
		const wxChar& c = name[p];
		if (c < '0' || c > '9')
			return name;
	}

	return name.Left(pos);
}

wxString CRemoteListView::GetItemText(int item, unsigned int column)
{
	int index = GetItemIndex(item);
	if (index == -1)
		return _T("");

	if (!column)
	{
		if ((unsigned int)index == m_pDirectoryListing->GetCount())
			return _T("..");
		else if ((unsigned int)index < m_pDirectoryListing->GetCount())
			return (*m_pDirectoryListing)[index].name;
		else
			return _T("");
	}
	if (!item)
		return _T(""); //.. has no attributes

	if ((unsigned int)index >= m_pDirectoryListing->GetCount())
		return _T("");

	if (column == 1)
	{
		const CDirentry& entry = (*m_pDirectoryListing)[index];
		if (entry.dir || entry.size < 0)
			return _T("");
		else
			return FormatSize(entry.size);
	}
	else if (column == 2)
	{
		CGenericFileData& data = m_fileData[index];
		if (data.fileType == _T(""))
		{
			const CDirentry& entry = (*m_pDirectoryListing)[index];
			if (m_pDirectoryListing->path.GetType() == VMS)
				data.fileType = GetType(StripVMSRevision(entry.name), entry.dir);
			else
				data.fileType = GetType(entry.name, entry.dir);
		}

		return data.fileType;
	}
	else if (column == 3)
	{
		const CDirentry& entry = (*m_pDirectoryListing)[index];
		if (!entry.hasDate)
			return _T("");

		if (entry.hasTime)
			return entry.time.Format(m_timeFormat);
		else
			return entry.time.Format(m_dateFormat);
	}
	else if (column == 4)
		return (*m_pDirectoryListing)[index].permissions;
	else if (column == 5)
		return (*m_pDirectoryListing)[index].ownerGroup;
	return _T("");
}

CFileListCtrl<CGenericFileData>::CSortComparisonObject CRemoteListView::GetSortComparisonObject()
{
	CRemoteListViewSort::DirSortMode dirSortMode = GetDirSortMode();

	if (!m_sortDirection)
	{
		if (m_sortColumn == 1)
			return CFileListCtrl<CGenericFileData>::CSortComparisonObject(new CRemoteListViewSortSize(m_pDirectoryListing, m_fileData, dirSortMode, this));
		else if (m_sortColumn == 2)
			return CFileListCtrl<CGenericFileData>::CSortComparisonObject(new CRemoteListViewSortType(m_pDirectoryListing, m_fileData, dirSortMode, this));
		else if (m_sortColumn == 3)
			return CFileListCtrl<CGenericFileData>::CSortComparisonObject(new CRemoteListViewSortTime(m_pDirectoryListing, m_fileData, dirSortMode, this));
		else if (m_sortColumn == 3)
			return CFileListCtrl<CGenericFileData>::CSortComparisonObject(new CRemoteListViewSortPermissions(m_pDirectoryListing, m_fileData, dirSortMode, this));
		else
			return CFileListCtrl<CGenericFileData>::CSortComparisonObject(new CRemoteListViewSortName(m_pDirectoryListing, m_fileData, dirSortMode, this));
	}
	else
	{
		if (m_sortColumn == 1)
			return CFileListCtrl<CGenericFileData>::CSortComparisonObject(new CRemoteListViewSortSize_Reverse(m_pDirectoryListing, m_fileData, dirSortMode, this));
		else if (m_sortColumn == 2)
			return CFileListCtrl<CGenericFileData>::CSortComparisonObject(new CRemoteListViewSortType_Reverse(m_pDirectoryListing, m_fileData, dirSortMode, this));
		else if (m_sortColumn == 3)
			return CFileListCtrl<CGenericFileData>::CSortComparisonObject(new CRemoteListViewSortTime_Reverse(m_pDirectoryListing, m_fileData, dirSortMode, this));
		else if (m_sortColumn == 4)
			return CFileListCtrl<CGenericFileData>::CSortComparisonObject(new CRemoteListViewSortPermissions_Reverse(m_pDirectoryListing, m_fileData, dirSortMode, this));
		else
			return CFileListCtrl<CGenericFileData>::CSortComparisonObject(new CRemoteListViewSortName_Reverse(m_pDirectoryListing, m_fileData, dirSortMode, this));
	}
}

void CRemoteListView::OnExitComparisonMode()
{
	CFileListCtrl<CGenericFileData>::OnExitComparisonMode();
	SetInfoText();
}

bool CRemoteListView::ItemIsDir(int index) const
{
	return (*m_pDirectoryListing)[index].dir;
}

wxLongLong CRemoteListView::ItemGetSize(int index) const
{
	return (*m_pDirectoryListing)[index].size;
}
