#ifndef FILELISTCTRL_INCLUDE_TEMPLATE_DEFINITION
// This works around a bug in GCC, appears to be [Bug pch/12707]
#include "FileZilla.h"
#endif
#include "filelistctrl.h"
#include "filezillaapp.h"
#include "Options.h"
#include "conditionaldialog.h"
#include <algorithm>
#include "filelist_statusbar.h"

#ifndef __WXMSW__
DECLARE_EVENT_TYPE(fz_EVT_FILELIST_FOCUSCHANGE, -1)
DECLARE_EVENT_TYPE(fz_EVT_DEFERRED_MOUSEEVENT, -1)
#ifndef FILELISTCTRL_INCLUDE_TEMPLATE_DEFINITION
DEFINE_EVENT_TYPE(fz_EVT_FILELIST_FOCUSCHANGE)
DEFINE_EVENT_TYPE(fz_EVT_DEFERRED_MOUSEEVENT)
#endif
#endif

BEGIN_EVENT_TABLE_TEMPLATE1(CFileListCtrl, wxListCtrlEx, CFileData)
EVT_LIST_COL_CLICK(wxID_ANY, CFileListCtrl<CFileData>::OnColumnClicked)
EVT_LIST_COL_RIGHT_CLICK(wxID_ANY, CFileListCtrl<CFileData>::OnColumnRightClicked)
EVT_LIST_ITEM_SELECTED(wxID_ANY, CFileListCtrl<CFileData>::OnItemSelected)
EVT_LIST_ITEM_DESELECTED(wxID_ANY, CFileListCtrl<CFileData>::OnItemDeselected)
#ifndef __WXMSW__
EVT_LIST_ITEM_FOCUSED(wxID_ANY, CFileListCtrl<CFileData>::OnFocusChanged)
EVT_COMMAND(wxID_ANY, fz_EVT_FILELIST_FOCUSCHANGE, CFileListCtrl<CFileData>::OnProcessFocusChange)
EVT_LEFT_DOWN(CFileListCtrl<CFileData>::OnLeftDown)
EVT_COMMAND(wxID_ANY, fz_EVT_DEFERRED_MOUSEEVENT, CFileListCtrl<CFileData>::OnProcessMouseEvent)
#endif
END_EVENT_TABLE()

#ifdef __WXMSW__
// wxWidgets does not hnadle LVN_ODSTATECHANGED, work around it

template<class CFileData> std::map<HWND, CFileListCtrl<CFileData>*> CFileListCtrl<CFileData>::m_hwnd_map;

template<class CFileData> LRESULT CALLBACK CFileListCtrl<CFileData>::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	std::map<HWND, CFileListCtrl<CFileData>*>::iterator iter = m_hwnd_map.find(hWnd);
	if (iter == m_hwnd_map.end())
	{
		// This shouldn't happen
        return 0;
	}
	CFileListCtrl<CFileData>* pFileListCtrl = iter->second;

	if (uMsg != WM_NOTIFY)
        return CallWindowProc(pFileListCtrl->m_prevWndproc, hWnd, uMsg, wParam, lParam);

	if (!pFileListCtrl->m_pFilelistStatusBar)
		return CallWindowProc(pFileListCtrl->m_prevWndproc, hWnd, uMsg, wParam, lParam);

	NMHDR* pNmhdr = (NMHDR*)lParam;
	if (pNmhdr->code == LVN_ODSTATECHANGED)
	{
		if (pFileListCtrl->m_insideSetSelection)
			return 0;

		// A range of items go (de)selected
		NMLVODSTATECHANGE* pNmOdStateChange = (NMLVODSTATECHANGE*)lParam;

		if (!pFileListCtrl->m_pFilelistStatusBar)
			return 0;

		wxASSERT(pNmOdStateChange->iFrom <= pNmOdStateChange->iTo);
		for (int i = pNmOdStateChange->iFrom; i <= pNmOdStateChange->iTo; i++)
		{
			const int index = pFileListCtrl->m_indexMapping[i];
			const CFileData& data = pFileListCtrl->m_fileData[index];
			if (data.flags == fill)
				continue;

			if (pFileListCtrl->m_hasParent && !i)
				continue;

			if (pFileListCtrl->ItemIsDir(index))
				pFileListCtrl->m_pFilelistStatusBar->SelectDirectory();
			else
				pFileListCtrl->m_pFilelistStatusBar->SelectFile(pFileListCtrl->ItemGetSize(index));
		}
		return 0;
	}
	else if (pNmhdr->code == LVN_ITEMCHANGED)
	{
		if (pFileListCtrl->m_insideSetSelection)
			return 0;

		NMLISTVIEW* pNmListView = (NMLISTVIEW*)lParam;

		// Item of -1 means change applied to all items
		if (pNmListView->iItem == -1 && !(pNmListView->uNewState & LVIS_SELECTED))
		{
			pFileListCtrl->m_pFilelistStatusBar->UnselectAll();
		}
	}

	return CallWindowProc(pFileListCtrl->m_prevWndproc, hWnd, uMsg, wParam, lParam);
}
#endif

template<class CFileData> CFileListCtrl<CFileData>::CFileListCtrl(wxWindow* pParent, CState* pState, CQueueView* pQueue)
	: wxListCtrlEx(pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxLC_VIRTUAL | wxLC_REPORT | wxNO_BORDER | wxLC_EDIT_LABELS),
	CComparableListing(this)
{
#ifdef __WXMSW__
	m_pHeaderImageList = 0;
#endif
	m_pQueue = pQueue;

	m_sortColumn = 0;
	m_sortDirection = 0;

	m_hasParent = true;

	m_comparisonIndex = -1;

#ifndef __WXMSW__
	m_dropHighlightAttribute.SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW));
#endif

	m_pFilelistStatusBar = 0;

	m_insideSetSelection = false;
#ifdef __WXMSW__
	// Subclass window
	m_hwnd_map[(HWND)pParent->GetHandle()] = this;
	m_prevWndproc = (WNDPROC)SetWindowLongPtr((HWND)pParent->GetHandle(), GWLP_WNDPROC, (LONG_PTR)WindowProc);
#else
	m_pending_focus_processing = 0;
	m_focusItem = -1;
#endif
}

template<class CFileData> CFileListCtrl<CFileData>::~CFileListCtrl()
{
#ifdef __WXMSW__
	delete m_pHeaderImageList;

	// Remove subclass
	if (m_prevWndproc != 0)
		SetWindowLongPtr((HWND)GetParent()->GetHandle(), GWLP_WNDPROC, (LONG_PTR)m_prevWndproc);
#endif
}

#ifdef __WXMSW__
template<class CFileData> void CFileListCtrl<CFileData>::InitHeaderImageList()
{
	// Initialize imagelist for list header
	m_pHeaderImageList = new wxImageListEx(8, 8, true, 3);

	wxBitmap bmp;

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

	TCHAR buffer[1000] = {0};
	HDITEM item;
	item.mask = HDI_TEXT;
	item.pszText = buffer;
	item.cchTextMax = 999;
	SendMessage(header, HDM_GETITEM, 0, (LPARAM)&item);

	SendMessage(header, HDM_SETIMAGELIST, 0, (LPARAM)m_pHeaderImageList->GetHandle());
}
#endif //__WXMSW__

template<class CFileData> void CFileListCtrl<CFileData>::SortList(int column /*=-1*/, int direction /*=-1*/, bool updateSelections /*=true*/)
{
	if (column != -1)
	{
#ifdef __WXMSW__
		if (column != m_sortColumn && m_pHeaderImageList)
		{
			const int oldVisibleColumn = GetColumnVisibleIndex(m_sortColumn);
			if (oldVisibleColumn != -1)
				SetHeaderIconIndex(oldVisibleColumn, -1);
		}
#endif
	}
	else
		column = m_sortColumn;

	if (direction == -1)
		direction = m_sortDirection;

	int newVisibleColumn = GetColumnVisibleIndex(column);
	if (newVisibleColumn == -1)
	{
		newVisibleColumn = 0;
		column = 0;
	}

#ifdef __WXMSW__
	if (m_pHeaderImageList)
		SetHeaderIconIndex(newVisibleColumn, direction ? 1 : 0);
#endif

	// Remember which files are selected
	bool *selected = 0;
	int focused = -1;
	if (updateSelections)
	{
		selected = new bool[m_fileData.size()];
		memset(selected, 0, sizeof(bool) * m_fileData.size());

		int item = -1;
		while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
			selected[m_indexMapping[item]] = 1;
		focused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
		if (focused != -1)
			focused = m_indexMapping[focused];
	}

	const int dirSortOption = COptions::Get()->GetOptionVal(OPTION_FILELIST_DIRSORT);

	if (column == m_sortColumn && direction != m_sortDirection && !m_indexMapping.empty() &&
		dirSortOption != 1)
	{
		// Simply reverse everything
		m_sortDirection = direction;
		m_sortColumn = column;
		std::vector<unsigned int>::iterator start = m_indexMapping.begin();
		if (m_hasParent)
			start++;
		std::reverse(start, m_indexMapping.end());

		if (updateSelections)
		{
			SortList_UpdateSelections(selected, focused);
			delete [] selected;
		}

		return;
	}

	m_sortDirection = direction;
	m_sortColumn = column;

	const unsigned int minsize = m_hasParent ? 3 : 2;
	if (m_indexMapping.size() < minsize)
	{
		if (updateSelections)
			delete [] selected;
		return;
	}

	std::vector<unsigned int>::iterator start = m_indexMapping.begin();
	if (m_hasParent)
		start++;
	CSortComparisonObject object = GetSortComparisonObject();
	std::sort(start, m_indexMapping.end(), object);
	object.Destroy();

	if (updateSelections)
	{
		SortList_UpdateSelections(selected, focused);
		delete [] selected;
	}
}

template<class CFileData> void CFileListCtrl<CFileData>::SortList_UpdateSelections(bool* selections, int focus)
{
	for (unsigned int i = m_hasParent ? 1 : 0; i < m_indexMapping.size(); i++)
	{
		const int state = GetItemState(i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		const bool selected = (state & wxLIST_STATE_SELECTED) != 0;
		const bool focused = (state & wxLIST_STATE_FOCUSED) != 0;

		int item = m_indexMapping[i];
		if (selections[item] != selected)
			SetSelection(i, selections[item]);
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

template<class CFileData> CListViewSort::DirSortMode CFileListCtrl<CFileData>::GetDirSortMode()
{
	const int dirSortOption = COptions::Get()->GetOptionVal(OPTION_FILELIST_DIRSORT);

	enum CListViewSort::DirSortMode dirSortMode;
	switch (dirSortOption)
	{
	case 0:
	default:
		dirSortMode = CListViewSort::dirsort_ontop;
		break;
	case 1:
		if (m_sortDirection)
			dirSortMode = CListViewSort::dirsort_onbottom;
		else
			dirSortMode = CListViewSort::dirsort_ontop;
		break;
	case 2:
		dirSortMode = CListViewSort::dirsort_inline;
		break;
	}

	return dirSortMode;
}

template<class CFileData> void CFileListCtrl<CFileData>::OnColumnClicked(wxListEvent &event)
{
	int col = m_pVisibleColumnMapping[event.GetColumn()];
	if (col == -1)
		return;

	if (IsComparing())
	{
#ifdef __WXMSW__
		ReleaseCapture();
		Refresh();
#endif
		CConditionalDialog dlg(this, CConditionalDialog::compare_changesorting, CConditionalDialog::yesno);
		dlg.SetTitle(_("Directory comparison"));
		dlg.AddText(_("Sort order cannot be changed if comparing directories."));
		dlg.AddText(_("End comparison and change sorting order?"));
		if (!dlg.Run())
			return;
		ExitComparisonMode();
	}

	int dir;
	if (col == m_sortColumn)
		dir = m_sortDirection ? 0 : 1;
	else
		dir = m_sortDirection;

	SortList(col, dir);
	Refresh(false);
}

template<class CFileData> wxString CFileListCtrl<CFileData>::GetType(wxString name, bool dir, const wxString& path /*=_T("")*/)
{
#ifdef __WXMSW__
	wxString ext = wxFileName(name).GetExt();
	ext.MakeLower();
	std::map<wxString, wxString>::iterator typeIter = m_fileTypeMap.find(ext);
	if (typeIter != m_fileTypeMap.end())
		return typeIter->second;

	wxString type;
	int flags = SHGFI_TYPENAME;
	if (path == _T(""))
		flags |= SHGFI_USEFILEATTRIBUTES;
	else if (path == _T("\\"))
		name += _T("\\");
	else
		name = path + name;

	SHFILEINFO shFinfo;
	memset(&shFinfo, 0, sizeof(SHFILEINFO));
	if (SHGetFileInfo(name,
		dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL,
		&shFinfo,
		sizeof(shFinfo),
		flags))
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
	return type;
#else
	if (dir)
		return _("Folder");

	wxFileName fn(name);
	wxString ext = fn.GetExt();
	if (ext == _T(""))
		return _("File");

	std::map<wxString, wxString>::iterator typeIter = m_fileTypeMap.find(ext);
	if (typeIter != m_fileTypeMap.end())
		return typeIter->second;

	wxFileType *pType = wxTheMimeTypesManager->GetFileTypeFromExtension(ext);
	if (!pType)
	{
		wxString desc = ext;
		desc += _T("-");
		desc += _("file");
		m_fileTypeMap[ext] = desc;
		return desc;
	}

	wxString desc;
	if (pType->GetDescription(&desc) && desc != _T(""))
	{
		delete pType;
		m_fileTypeMap[ext] = desc;
		return desc;
	}
	delete pType;

	desc = ext;
	desc += _T("-");
	desc += _("file");
	m_fileTypeMap[ext.MakeLower()] = desc;
	return desc;
#endif
}

template<class CFileData> void CFileListCtrl<CFileData>::ScrollTopItem(int item)
{
	wxListCtrlEx::ScrollTopItem(item);
}

template<class CFileData> void CFileListCtrl<CFileData>::OnPostScroll()
{
	if (!IsComparing())
		return;

	CComparableListing* pOther = GetOther();
	if (!pOther)
		return;

	pOther->ScrollTopItem(GetTopItem());
}

template<class CFileData> void CFileListCtrl<CFileData>::OnExitComparisonMode()
{
	ComparisonRememberSelections();

	wxASSERT(!m_originalIndexMapping.empty());
	m_indexMapping.clear();
	m_indexMapping.swap(m_originalIndexMapping);

	for (unsigned int i = 0; i < m_fileData.size() - 1; i++)
		m_fileData[i].flags = normal;

	SetItemCount(m_indexMapping.size());

	ComparisonRestoreSelections();

	Refresh();
}

template<class CFileData> void CFileListCtrl<CFileData>::CompareAddFile(t_fileEntryFlags flags)
{
	if (flags == fill)
	{
		m_indexMapping.push_back(m_fileData.size() - 1);
		return;
	}

	int index = m_originalIndexMapping[m_comparisonIndex];
	m_fileData[index].flags = flags;

	m_indexMapping.push_back(index);
}

template<class CFileData> void CFileListCtrl<CFileData>::ComparisonRememberSelections()
{
	m_comparisonSelections.clear();

	if (GetItemCount() != (int)m_indexMapping.size())
		return;

	int focus = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
	if (focus != -1)
	{
		SetItemState(focus, 0, wxLIST_STATE_FOCUSED);
		int index = m_indexMapping[focus];
		if (m_fileData[index].flags == fill)
			focus = -1;
		else
			focus = index;
	}
	m_comparisonSelections.push_back(focus);

	int item = -1;
	while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
	{
		int index = m_indexMapping[item];
		if (m_fileData[index].flags == fill)
			continue;
		m_comparisonSelections.push_back(index);
	}
}

template<class CFileData> void CFileListCtrl<CFileData>::ComparisonRestoreSelections()
{
	if (m_comparisonSelections.empty())
		return;

	int focus = m_comparisonSelections.front();
	m_comparisonSelections.pop_front();

	int item = -1;
	if (!m_comparisonSelections.empty())
	{
		item = m_comparisonSelections.front();
		m_comparisonSelections.pop_front();
	}
	if (focus == -1)
		focus = item;

	for (unsigned int i = 0; i < m_indexMapping.size(); i++)
	{
		int index = m_indexMapping[i];
		if (focus == index)
		{
			SetItemState(i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
			focus = -1;
		}

		bool isSelected = GetItemState(i, wxLIST_STATE_SELECTED) == wxLIST_STATE_SELECTED;
		bool shouldSelected = item == index;
		if (isSelected != shouldSelected)
			SetSelection(i, shouldSelected);

		if (shouldSelected)
		{
			if (m_comparisonSelections.empty())
				item = -1;
			else
			{
				item = m_comparisonSelections.front();
				m_comparisonSelections.pop_front();
			}
		}
	}
}

template<class CFileData> void CFileListCtrl<CFileData>::OnColumnRightClicked(wxListEvent& event)
{
	ShowColumnEditor();
}

template<class CFileData> void CFileListCtrl<CFileData>::InitSort(int optionID)
{
	wxString sortInfo = COptions::Get()->GetOption(optionID);
	m_sortDirection = sortInfo[0] - '0';
	if (m_sortDirection < 0 || m_sortDirection > 1)
		m_sortDirection = 0;

	if (sortInfo.Len() == 3)
	{
		m_sortColumn = sortInfo[2] - '0';
		if (GetColumnVisibleIndex(m_sortColumn) == -1)
			m_sortColumn = 0;
	}
	else
		m_sortColumn = 0;
}

template<class CFileData> void CFileListCtrl<CFileData>::OnItemSelected(wxListEvent& event)
{
#ifndef __WXMSW__
	// On MSW this is done in the subclassed window proc
	if (m_insideSetSelection)
		return;
	if (m_pending_focus_processing)
		return;
#endif

	const int item = event.GetIndex();

#ifndef __WXMSW__
	if (m_selections[item])
		return;
	m_selections[item] = true;
#endif

	if (!m_pFilelistStatusBar)
		return;

	if (item < 0 || item >= (int)m_indexMapping.size())
		return;

	if (m_hasParent && !item)
		return;

	const int index = m_indexMapping[item];
	const CFileData& data = m_fileData[index];
	if (data.flags == fill)
		return;

	if (ItemIsDir(index))
		m_pFilelistStatusBar->SelectDirectory();
	else
		m_pFilelistStatusBar->SelectFile(ItemGetSize(index));
}

template<class CFileData> void CFileListCtrl<CFileData>::OnItemDeselected(wxListEvent& event)
{
#ifndef __WXMSW__
	// On MSW this is done in the subclassed window proc
	if (m_insideSetSelection)
		return;
#endif

	const int item = event.GetIndex();

#ifndef __WXMSW__
	if (!m_selections[item])
		return;
	m_selections[item] = false;
#endif

	if (!m_pFilelistStatusBar)
		return;

	if (item < 0 || item >= (int)m_indexMapping.size())
		return;

	if (m_hasParent && !item)
		return;

	const int index = m_indexMapping[item];
	const CFileData& data = m_fileData[index];
	if (data.flags == fill)
		return;

	if (ItemIsDir(index))
		m_pFilelistStatusBar->UnselectDirectory();
	else
		m_pFilelistStatusBar->UnselectFile(ItemGetSize(index));
}

template<class CFileData> void CFileListCtrl<CFileData>::SetSelection(int item, bool select)
{
	m_insideSetSelection = true;
	SetItemState(item, select ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED);
	m_insideSetSelection = false;
#ifndef __WXMSW__
	m_selections[item] = select;
#endif
}

#ifndef __WXMSW__
template<class CFileData> void CFileListCtrl<CFileData>::OnFocusChanged(wxListEvent& event)
{
	const int focusItem = event.GetIndex();

	// Need to defer processing, as focus it set before selection by wxWidgets internally
	wxCommandEvent evt;
	evt.SetEventType(fz_EVT_FILELIST_FOCUSCHANGE);
	evt.SetInt(m_focusItem);
	evt.SetExtraLong((long)focusItem);
	m_pending_focus_processing++;
	AddPendingEvent(evt);

	m_focusItem = focusItem;
}

template<class CFileData> void CFileListCtrl<CFileData>::SetItemCount(int count)
{
	m_selections.resize(count, false);
	if (m_focusItem >= count)
		m_focusItem = -1;
	wxListCtrlEx::SetItemCount(count);
}

template<class CFileData> void CFileListCtrl<CFileData>::OnProcessFocusChange(wxCommandEvent& event)
{
	m_pending_focus_processing--;
	int old_focus = event.GetInt();
	int new_focus = (int)event.GetExtraLong();

	if (old_focus >= GetItemCount())
		return;

	if (old_focus != -1)
	{
		bool selected = GetItemState(old_focus, wxLIST_STATE_SELECTED) == wxLIST_STATE_SELECTED;
		if (!selected && m_selections[old_focus])
		{
			// Need to deselect all
			if (m_pFilelistStatusBar)
				m_pFilelistStatusBar->UnselectAll();
			for (unsigned int i = 0; i < m_selections.size(); i++)
				m_selections[i] = 0;
		}
	}

	int min;
	int max;
	if (new_focus > old_focus)
	{
		min = old_focus;
		max = new_focus;
	}
	else
	{
		min = new_focus;
		max = old_focus;
	}
	if (min == -1)
		min++;
	if (max == -1)
		return;

	if (max >= GetItemCount())
		return;

	for (int i = min; i <= max; i++)
	{
		bool selected = GetItemState(i, wxLIST_STATE_SELECTED) == wxLIST_STATE_SELECTED;
		if (selected == m_selections[i])
			continue;

		m_selections[i] = selected;

		if (!m_pFilelistStatusBar)
			continue;

		if (m_hasParent && !i)
			continue;

		const int index = m_indexMapping[i];
		const CFileData& data = m_fileData[index];
		if (data.flags == fill)
			continue;

		if (selected)
		{
			if (ItemIsDir(index))
				m_pFilelistStatusBar->SelectDirectory();
			else
				m_pFilelistStatusBar->SelectFile(ItemGetSize(index));
		}
		else
		{
			if (ItemIsDir(index))
				m_pFilelistStatusBar->UnselectDirectory();
			else
				m_pFilelistStatusBar->UnselectFile(ItemGetSize(index));
		}
	}
}

template<class CFileData> void CFileListCtrl<CFileData>::OnLeftDown(wxMouseEvent& event)
{
	// Left clicks in the whitespace around the items deselect everything
	// but does not change focus. Defer event.
	event.Skip();
	wxCommandEvent evt;
	evt.SetEventType(fz_EVT_DEFERRED_MOUSEEVENT);
	AddPendingEvent(evt);
}

template<class CFileData> void CFileListCtrl<CFileData>::OnProcessMouseEvent(wxCommandEvent& event)
{
	if (m_pending_focus_processing)
		return;

	if (m_focusItem >= GetItemCount())
		return;
	if (m_focusItem == -1)
		return;

	bool selected = GetItemState(m_focusItem, wxLIST_STATE_SELECTED) == wxLIST_STATE_SELECTED;
	if (!selected && m_selections[m_focusItem])
	{
		// Need to deselect all
		if (m_pFilelistStatusBar)
			m_pFilelistStatusBar->UnselectAll();
		for (unsigned int i = 0; i < m_selections.size(); i++)
			m_selections[i] = 0;
	}
}
#endif
