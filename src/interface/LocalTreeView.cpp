#include "FileZilla.h"
#include "LocalTreeView.h"
#include "QueueView.h"
#include "filezillaapp.h"
#include "filter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef __WXMSW__
#include <shlobj.h>
#endif

BEGIN_EVENT_TABLE(CLocalTreeView, wxTreeCtrl)
EVT_TREE_ITEM_EXPANDING(wxID_ANY, CLocalTreeView::OnItemExpanding)
EVT_TREE_SEL_CHANGED(wxID_ANY, CLocalTreeView::OnSelectionChanged)
END_EVENT_TABLE()

CLocalTreeView::CLocalTreeView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueueView)
	: wxTreeCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxTR_EDIT_LABELS | wxTR_LINES_AT_ROOT | wxTR_HAS_BUTTONS | wxNO_BORDER),
	CSystemImageList(16), m_pQueueView(pQueueView),
	CStateEventHandler(pState, STATECHANGE_LOCAL_DIR | STATECHANGE_APPLYFILTER)
{
	m_setSelection = false;

	SetImageList(GetSystemImageList());

#ifdef __WXMSW__
	LPITEMIDLIST list;
	SHGetSpecialFolderLocation(GetHwnd(), CSIDL_DRIVES, &list);
	SHFILEINFO shFinfo;

	SHGetFileInfo((LPCTSTR)list,0,&shFinfo,sizeof(shFinfo),SHGFI_PIDL|SHGFI_ICON|SHGFI_SMALLICON);
	DestroyIcon(shFinfo.hIcon);
	int iIcon=shFinfo.iIcon;
	SHGetFileInfo((LPCTSTR)list,0,&shFinfo,sizeof(shFinfo),SHGFI_PIDL|SHGFI_ICON|SHGFI_SMALLICON|SHGFI_OPENICON|SHGFI_DISPLAYNAME);
	DestroyIcon(shFinfo.hIcon);

	AddRoot(shFinfo.szDisplayName, iIcon, shFinfo.iIcon);

	LPMALLOC pMalloc;
    SHGetMalloc(&pMalloc);

	if (pMalloc)
	{
		pMalloc->Free(list);
		pMalloc->Release();
	}
	else
		wxLogLastError(wxT("SHGetMalloc"));

	DisplayDrives();
	Expand(GetRootItem());
#else
	AddRoot(_T("/"), GetIconIndex(dir), GetIconIndex(opened_dir));
	SetDir(_T("/"));
#endif
}

CLocalTreeView::~CLocalTreeView()
{
}

void CLocalTreeView::SetDir(wxString localDir)
{
	if (m_currentDir == localDir)
	{
		Refresh();
		return;
	}
	m_currentDir = localDir;

#ifdef __WXMSW__
	if (localDir == _T("\\"))
	{
		m_setSelection = true;
		SelectItem(GetRootItem());
		m_setSelection = false;
		return;
	}
#endif

	wxString subDirs = localDir;
	wxTreeItemId parent = GetNearestParent(subDirs);
	if (!parent)
		return;

	if (subDirs == _T(""))
	{
		wxTreeItemIdValue value;
		wxTreeItemId child = GetFirstChild(parent, value);
		if (child && GetItemText(child) == _T(""))
			DisplayDir(parent, localDir);

		m_setSelection = true;
		SelectItem(parent);
		m_setSelection = false;
		if (parent != GetRootItem())
			Expand(GetItemParent(parent));
		return;
	}
	wxTreeItemId item = MakeSubdirs(parent, localDir.Left(localDir.Length() - subDirs.Length()), subDirs);
	if (!item)
		return;

	m_setSelection = true;
	SelectItem(item);
	m_setSelection = false;
	if (item != GetRootItem())
		Expand(GetItemParent(item));
}

wxTreeItemId CLocalTreeView::GetNearestParent(wxString& localDir)
{
	const wxString separator = wxFileName::GetPathSeparator();
#ifdef __WXMSW__
	int pos = localDir.Find(separator);
	if (pos == -1)
		return wxTreeItemId();

	wxString drive = localDir.Left(pos);
	localDir = localDir.Mid(pos + 1);

	wxTreeItemIdValue value;
	wxTreeItemId root = GetFirstChild(GetRootItem(), value);
	while (root)
	{
		if (!GetItemText(root).Left(drive.Length()).CmpNoCase(drive))
			break;

		root = GetNextSibling(root);
	}
	if (!root)
		// TODO: Number is drives changed
		return wxTreeItemId();
#else
		if (localDir[0] == '/')
			localDir = localDir.Mid(1);
		wxTreeItemId root = GetRootItem();
#endif

	while (localDir != _T(""))
	{
		wxString subDir;
		int pos = localDir.Find(separator);
		if (pos == -1)
			subDir = localDir;
		else
			subDir = localDir.Left(pos);

		wxTreeItemId child = GetSubdir(root, subDir);
		if (!child)
			return root;

		if (!pos)
			return child;

		root = child;
		localDir = localDir.Mid(pos + 1);
	}

	return root;
}

wxTreeItemId CLocalTreeView::GetSubdir(wxTreeItemId parent, const wxString& subDir)
{
	wxTreeItemIdValue value;
	wxTreeItemId child = GetFirstChild(parent, value);
	while (child)
	{
#ifdef __WXMSW__
		if (!GetItemText(child).CmpNoCase(subDir))
#else
		if (GetItemText(child) == subDir)
#endif
			return child;

		child = GetNextSibling(child);
	}
	return wxTreeItemId();
}

#ifdef __WXMSW__

bool CLocalTreeView::DisplayDrives()
{
	wxTreeItemId root = GetRootItem();

	wxChar* pDrive;

	int len = GetLogicalDriveStrings(0, 0);
	if (!len)
		return false;

	wxChar* drives = new wxChar[len + 1];

	if (!GetLogicalDriveStrings(len, drives))
	{
		delete [] drives;
		return false;
	}

	pDrive = drives;
	while (*pDrive)
	{
		wxString drive = pDrive;
		if (drive.Right(1) == _T("\\"))
			drive = drive.RemoveLast();

		// Get the label of the drive
		wxChar buffer[501];
		BOOL res = GetVolumeInformation(pDrive, buffer, 500, 0, 0, 0, 0, 0);
		int oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
		SetErrorMode(oldErrorMode);
		if (res && buffer[0])
		{
			drive += _T(" (");
			drive += buffer;
			drive += _T(")");
		}
		wxTreeItemId item = AppendItem(GetRootItem(), drive, GetIconIndex(dir, pDrive));
		AppendItem(item, _T(""));
		pDrive += _tcslen( pDrive ) + 1;
		SortChildren(GetRootItem());
	}

	delete [] drives;

	return true;
}

#endif

void CLocalTreeView::DisplayDir(wxTreeItemId parent, const wxString& dirname, const wxString& filterException /*=_T("")*/)
{
	wxASSERT(parent);
	m_setSelection = true;
	DeleteChildren(parent);
	m_setSelection = false;
	wxDir dir(dirname);
	wxString file;

	CFilterDialog filter;

	for (bool found = dir.GetFirst(&file, _T(""), wxDIR_DIRS | wxDIR_HIDDEN); found; found = dir.GetNext(&file))
	{
		if (file == _T(""))
		{
			wxGetApp().DisplayEncodingWarning();
			continue;
		}

		wxString fullName = dirname + file;
#ifdef __WXMSW__
		if (file.CmpNoCase(filterException))
#else
		if (file != filterException)
#endif
		{
			wxFileName fn(fullName);
			const bool isDir = fn.DirExists();

			wxLongLong size;

			wxStructStat buf;
			if (!isDir && !wxStat(fn.GetFullPath(), &buf))
				size = buf.st_size;
			else
				size = -1;
	
			if (filter.FilenameFiltered(file, isDir, size, true))
				continue;
		}

		wxTreeItemId item = AppendItem(parent, file, GetIconIndex(::dir, fullName), GetIconIndex(opened_dir, fullName));
		if (HasSubdir(fullName))
			AppendItem(item, _T(""));		
	}
	SortChildren(parent);
}

bool CLocalTreeView::HasSubdir(const wxString& dirname)
{
	wxLogNull nullLog;
	wxDir dir(dirname);
	if (!dir.IsOpened())
		return false;

	CFilterDialog filter;
	wxString file;
	for (bool found = dir.GetFirst(&file, _T(""), wxDIR_DIRS | wxDIR_HIDDEN); found; found = dir.GetNext(&file))
	{
		if (!filter.FilenameFiltered(file, true, -1, true))
			return true;
	}

	return false;
}

wxTreeItemId CLocalTreeView::MakeSubdirs(wxTreeItemId parent, wxString dirname, wxString subDir)
{
	const wxString& separator = wxFileName::GetPathSeparator();

	while (subDir != _T(""))
	{
		int pos = subDir.Find(separator);
		wxString segment;
		if (pos == -1)
		{
			segment = subDir;
			subDir = _T("");
		}
		else
		{
			segment = subDir.Left(pos);
			subDir = subDir.Mid(pos + 1);
		}

		DisplayDir(parent, dirname, segment);

		wxTreeItemId item = GetSubdir(parent, segment);
		if (!item)
			return wxTreeItemId();

		parent = item;
		dirname += segment + separator;
	}
	
	DisplayDir(parent, dirname);
	return parent;
}

void CLocalTreeView::OnItemExpanding(wxTreeEvent& event)
{
	wxTreeItemId item = event.GetItem();

	wxTreeItemIdValue value;
	wxTreeItemId child = GetFirstChild(item, value);
	if (child && GetItemText(child) == _T(""))
		DisplayDir(item, GetDirFromItem(item));
}

wxString CLocalTreeView::GetDirFromItem(wxTreeItemId item)
{
	const wxString separator = wxFileName::GetPathSeparator();
	wxString dir;
	while (item)
	{
#ifdef __WXMSW__
		if (GetItemParent(item) == GetRootItem())
		{
			wxString text = GetItemText(item);
			int pos = text.Find(_T(" "));
			if (pos == -1)
				return text + separator + dir;
			else
				return text.Left(pos) + separator + dir;
		}
		else
#endif
		if (item == GetRootItem())
			return _T("/") + dir;

		dir = GetItemText(item) + separator + dir;

		item = GetItemParent(item);
	}

	return separator;
}

struct t_dir
{
	wxString dir;
	wxTreeItemId item;
};

static bool sortfunc(const wxString& a, const wxString& b)
{
#ifdef __WXMSW__
	return b.CmpNoCase(a) > 0;
#else
	return b.Cmp(a) > 0;
#endif
}

void CLocalTreeView::Refresh()
{
	wxLogNull nullLog;

	const wxString separator = wxFileName::GetPathSeparator();

	std::list<t_dir> dirsToCheck;

#ifdef __WXMSW__
	int prevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

	wxTreeItemIdValue tmp;
	wxTreeItemId child = GetFirstChild(GetRootItem(), tmp);
	while (child)
	{
		if (IsExpanded(child))
		{
			wxString drive = GetItemText(child);
			int pos = drive.Find(_T(" "));
			if (pos != -1)
				drive = drive.Left(pos);

			t_dir dir;
			dir.dir = drive + separator;
			dir.item = child;
			dirsToCheck.push_back(dir);
		}

		child = GetNextSibling(child);
	}
#else
	t_dir dir;
	dir.dir = separator;
	dir.item = GetRootItem();
	dirsToCheck.push_back(dir);
#endif

	CFilterDialog filter;

	while (!dirsToCheck.empty())
	{
		t_dir dir = dirsToCheck.front();
		dirsToCheck.pop_front();

		wxDir find(dir.dir);
		wxString file;
		bool found = find.GetFirst(&file, _T(""), wxDIR_DIRS | wxDIR_HIDDEN);
		std::list<wxString> dirs;
		while (found)
		{
			if (file == _T(""))
			{
				wxGetApp().DisplayEncodingWarning();
				found = find.GetNext(&file);
				continue;
			}
			wxFileName fn(dir.dir + file);
			const bool isDir = fn.DirExists();

			wxLongLong size;
			
			wxStructStat buf;
			if (!isDir && !wxStat(fn.GetFullPath(), &buf))
				size = buf.st_size;
			else
				size = -1;

			if (!filter.FilenameFiltered(file, isDir, size , true))
				dirs.push_back(file);
			found = find.GetNext(&file);
		}
		dirs.sort(sortfunc);

		bool inserted = false;

		wxTreeItemId child = GetLastChild(dir.item);
		std::list<wxString>::reverse_iterator iter = dirs.rbegin();
		while (child && iter != dirs.rend())
		{
#ifdef __WXMSW__
			int cmp = GetItemText(child).CmpNoCase(*iter);
#else
			int cmp = GetItemText(child).Cmp(*iter);
#endif
			if (!cmp)
			{
				if (!IsExpanded(child))
				{
					wxTreeItemId subchild = GetLastChild(child);
					if (subchild && GetItemText(subchild) == _T(""))
					{
						if (!HasSubdir(dir.dir + *iter + separator))
							Delete(subchild);
					}
					else if (!subchild)
					{
						if (HasSubdir(dir.dir + *iter + separator))
							AppendItem(child, _T(""));
					}
					else
					{
						t_dir subdir;
						subdir.dir = dir.dir + *iter + separator;
						subdir.item = child;
						dirsToCheck.push_front(subdir);
					}
				}
				else
				{
					t_dir subdir;
					subdir.dir = dir.dir + *iter + separator;
					subdir.item = child;
					dirsToCheck.push_front(subdir);
				}
				child = GetPrevSibling(child);
				iter++;
			}
			else if (cmp > 0)
			{
				wxTreeItemId prev = GetPrevSibling(child);
				Delete(child);
				child = prev;
			}
			else if (cmp < 0)
			{
				wxString fullname = dir.dir + *iter + separator;
				wxTreeItemId newItem = AppendItem(dir.item, *iter, GetIconIndex(::dir, fullname), GetIconIndex(opened_dir, fullname));
				if (HasSubdir(fullname))
					AppendItem(newItem, _T(""));
				iter++;
				inserted = true;
			}
		}
		while (child)
		{
			wxTreeItemId prev = GetPrevSibling(child);
			Delete(child);
			child = prev;
		}
		while (iter != dirs.rend())
		{
			wxString fullname = dir.dir + *iter + separator;
			wxTreeItemId newItem = AppendItem(dir.item, *iter, GetIconIndex(::dir, fullname), GetIconIndex(opened_dir, fullname));
			if (HasSubdir(fullname))
				AppendItem(newItem, _T(""));
			iter++;
			inserted = true;
		}
		if (inserted)
			SortChildren(dir.item);
	}

#ifdef __WXMSW__
	SetErrorMode(prevErrorMode);
#endif
}

void CLocalTreeView::OnSelectionChanged(wxTreeEvent& event)
{
	if (m_setSelection)
	{
		event.Skip();
		return;
	}

	wxTreeItemId item = event.GetItem();
	if (!item)
		return;

	wxString dir = GetDirFromItem(item);

	m_pState->SetLocalDir(dir);
}

int CLocalTreeView::OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2)
{
	wxString label1 = GetItemText(item1);
	wxString label2 = GetItemText(item2);
#ifdef __WXMSW__
	return label1.CmpNoCase(label2);
#else
	return label1.Cmp(label2);
#endif
}

void CLocalTreeView::OnStateChange(unsigned int event)
{
	if (event == STATECHANGE_LOCAL_DIR)
		SetDir(m_pState->GetLocalDir());
	else if (event == STATECHANGE_APPLYFILTER)
		Refresh();
}
