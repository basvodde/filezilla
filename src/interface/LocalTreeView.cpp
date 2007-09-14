#include "FileZilla.h"
#include "LocalTreeView.h"
#include "queue.h"
#include "filezillaapp.h"
#include "filter.h"
#include <wx/dnd.h>
#include "dndobjects.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#include <shlobj.h>
#endif

class CLocalTreeViewDropTarget : public wxDropTarget
{
public:
	CLocalTreeViewDropTarget(CLocalTreeView* pLocalTreeView)
		: m_pLocalTreeView(pLocalTreeView), m_pFileDataObject(new wxFileDataObject()),
		m_pRemoteDataObject(new CRemoteDataObject())
	{
		m_pDataObject = new wxDataObjectComposite;
		m_pDataObject->Add(m_pRemoteDataObject, true);
		m_pDataObject->Add(m_pFileDataObject, false);
		SetDataObject(m_pDataObject);
	}

	void ClearDropHighlight()
	{
		const wxTreeItemId dropHighlight = m_pLocalTreeView->m_dropHighlight;
		if (dropHighlight != wxTreeItemId())
		{
			m_pLocalTreeView->SetItemDropHighlight(dropHighlight, false);
			m_pLocalTreeView->m_dropHighlight = wxTreeItemId();
		}
	}

	wxString GetDirFromItem(const wxTreeItemId& item)
	{
		const wxString& dir = m_pLocalTreeView->GetDirFromItem(item);

#ifdef __WXMSW__
		if (dir == _T("/"))
			return _T("");
#endif

		return dir;
	}

	wxTreeItemId GetHit(const wxPoint& point)
	{
		int flags = 0;
		wxTreeItemId hit = m_pLocalTreeView->HitTest(point, flags);

		if (flags & (wxTREE_HITTEST_ABOVE | wxTREE_HITTEST_BELOW | wxTREE_HITTEST_NOWHERE | wxTREE_HITTEST_TOLEFT | wxTREE_HITTEST_TORIGHT))
			return wxTreeItemId();

		return hit;
	}

	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
	{
		if (def == wxDragError ||
			def == wxDragNone ||
			def == wxDragCancel)
			return def;

		wxTreeItemId hit = GetHit(wxPoint(x, y));
		if (!hit)
			return wxDragNone;

		wxString dir = GetDirFromItem(hit);
		if (dir == _T("") || !CState::LocalDirIsWriteable(dir))
			return wxDragNone;

		if (!GetData())
			return wxDragError;

		if (m_pDataObject->GetReceivedFormat() == m_pFileDataObject->GetFormat())
			m_pLocalTreeView->m_pState->HandleDroppedFiles(m_pFileDataObject, dir, def == wxDragCopy);
		else
		{
			if (m_pRemoteDataObject->GetProcessId() != (int)wxGetProcessId())
			{
				wxMessageBox(_("Drag&drop between different instances of FileZilla has not been implemented yet."));
				return wxDragNone;
			}

			if (!m_pLocalTreeView->m_pState->GetServer() || m_pRemoteDataObject->GetServer() != *m_pLocalTreeView->m_pState->GetServer())
			{
				wxMessageBox(_("Drag&drop between different servers has not been implemented yet."));
				return wxDragNone;
			}

			if (!m_pLocalTreeView->m_pState->DownloadDroppedFiles(m_pRemoteDataObject, dir))
				return wxDragNone;
		}

		return def;
	}

	virtual bool OnDrop(wxCoord x, wxCoord y)
	{
		ClearDropHighlight();

		wxTreeItemId hit = GetHit(wxPoint(x, y));
		if (!hit)
			return false;

		wxString dir = GetDirFromItem(hit);
		if (dir == _T("") || !CState::LocalDirIsWriteable(dir))
			return false;

		return true;
	}

	wxString DisplayDropHighlight(wxPoint point)
	{
		wxTreeItemId hit = GetHit(point);
		if (!hit)
		{
			ClearDropHighlight();
			return _T("");
		}

		wxString dir = GetDirFromItem(hit);

		if (dir == _T(""))
		{
			ClearDropHighlight();
			return _T("");
		}

		const wxTreeItemId dropHighlight = m_pLocalTreeView->m_dropHighlight;
		if (dropHighlight != wxTreeItemId())
			m_pLocalTreeView->SetItemDropHighlight(dropHighlight, false);

		m_pLocalTreeView->SetItemDropHighlight(hit, true);
		m_pLocalTreeView->m_dropHighlight = hit;


		return dir;
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

		const wxString& dir = DisplayDropHighlight(wxPoint(x, y));
		if (dir == _T(""))
			return wxDragNone;

		if (def == wxDragLink)
			def = wxDragCopy;

		return def;
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
	CLocalTreeView *m_pLocalTreeView;
	wxFileDataObject* m_pFileDataObject;
	CRemoteDataObject* m_pRemoteDataObject;
	wxDataObjectComposite* m_pDataObject;
};

IMPLEMENT_CLASS(CLocalTreeView, wxTreeCtrl)

BEGIN_EVENT_TABLE(CLocalTreeView, wxTreeCtrl)
EVT_TREE_ITEM_EXPANDING(wxID_ANY, CLocalTreeView::OnItemExpanding)
EVT_TREE_SEL_CHANGED(wxID_ANY, CLocalTreeView::OnSelectionChanged)
EVT_TREE_BEGIN_DRAG(wxID_ANY, CLocalTreeView::OnBeginDrag)
#ifndef __WXMSW__
EVT_KEY_DOWN(CLocalTreeView::OnKeyDown)
#else
EVT_COMMAND(-1, fzEVT_VOLUMESENUMERATED, CLocalTreeView::OnVolumesEnumerated)
#endif //__WXMSW__
END_EVENT_TABLE()

CLocalTreeView::CLocalTreeView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueueView)
	: wxTreeCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxTR_EDIT_LABELS | wxTR_LINES_AT_ROOT | wxTR_HAS_BUTTONS | wxNO_BORDER),
	CSystemImageList(16),
	CStateEventHandler(pState, STATECHANGE_LOCAL_DIR | STATECHANGE_APPLYFILTER),
	m_pQueueView(pQueueView)
{
	m_setSelection = false;

	SetImageList(GetSystemImageList());

#ifdef __WXMSW__
	m_pVolumeEnumeratorThread = 0;

	CreateRoot();
#else
	AddRoot(_T("/"), GetIconIndex(dir), GetIconIndex(opened_dir));
	SetDir(_T("/"));
#endif

	SetDropTarget(new CLocalTreeViewDropTarget(this));
}

CLocalTreeView::~CLocalTreeView()
{
#ifdef __WXMSW__
	delete m_pVolumeEnumeratorThread;
#endif
}

void CLocalTreeView::SetDir(wxString localDir)
{
	if (m_currentDir == localDir)
	{
		Refresh();
		return;
	}

	if (localDir.Left(2) == _T("\\\\"))
	{
		// TODO: UNC path, don't display it yet
		m_currentDir = _T("");
		m_setSelection = true;
		SelectItem(wxTreeItemId());
		m_setSelection = false;
		return;
	}
	m_currentDir = localDir;

#ifdef __WXMSW__
	if (localDir == _T("\\"))
	{
		m_setSelection = true;
		SelectItem(m_drives);
		m_setSelection = false;
		return;
	}
#endif

	wxString subDirs = localDir;
	wxTreeItemId parent = GetNearestParent(subDirs);
	if (!parent)
	{
		m_setSelection = true;
		SelectItem(wxTreeItemId());
		m_setSelection = false;
		return;
	}

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
	wxTreeItemId root = GetFirstChild(m_drives, value);
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

bool CLocalTreeView::DisplayDrives(wxTreeItemId parent)
{
	long drivesToHide = 0;
	// Adhere to the NODRIVES group policy
	wxRegKey key(_T("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"));
	if (key.Exists())
	{
		if (!key.HasValue(_T("NoDrives")) || !key.QueryValue(_T("NoDrives"), &drivesToHide))
			drivesToHide = 0;
	}

	int len = GetLogicalDriveStrings(0, 0);
	if (!len)
		return false;

	wxChar* drives = new wxChar[len + 1];

	if (!GetLogicalDriveStrings(len, drives))
	{
		delete [] drives;
		return false;
	}

	m_pVolumeEnumeratorThread = new CVolumeDescriptionEnumeratorThread(this);
	if (m_pVolumeEnumeratorThread->Failed())
	{
		delete m_pVolumeEnumeratorThread;
		m_pVolumeEnumeratorThread = 0;
	}

	const wxChar* pDrive = drives;
	while (*pDrive)
	{
		// Check if drive should be hidden by default
		if (pDrive[0] != 0 && pDrive[1] == ':')
		{
			int bit = -1;
			char letter = pDrive[0];
			if (letter >= 'A' && letter <= 'Z')
				bit = 1 << (letter - 'A');
			if (letter >= 'a' && letter <= 'z')
				bit = 1 << (letter - 'a');

			if (bit != -1 && drivesToHide & bit)
			{
				pDrive += wxStrlen(pDrive) + 1;
				continue;
			}
		}

		wxString drive = pDrive;
		if (drive.Right(1) == _T("\\"))
			drive = drive.RemoveLast();

		wxTreeItemId item = AppendItem(parent, drive, GetIconIndex(dir, pDrive));
		AppendItem(item, _T(""));
		SortChildren(parent);

		pDrive += wxStrlen(pDrive) + 1;
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
		if (item == m_desktop)
		{
			wxChar path[MAX_PATH + 1];
			if (SHGetFolderPath(0, CSIDL_DESKTOPDIRECTORY, 0, SHGFP_TYPE_CURRENT, path) != S_OK)
			{
				if (SHGetFolderPath(0, CSIDL_DESKTOP, 0, SHGFP_TYPE_CURRENT, path) != S_OK)
				{
					wxMessageBox(_("Failed to get desktop path"));
					return _T("/");
				}
			}
			dir = path;
			if (dir.Last() != separator)
				dir += separator;
			return dir;
		}
		else if (item == m_documents)
		{
			wxChar path[MAX_PATH + 1];
			if (SHGetFolderPath(0, CSIDL_PERSONAL, 0, SHGFP_TYPE_CURRENT, path) != S_OK)
			{
				wxMessageBox(_("Failed to get 'My Documents' path"));
				return _T("/");
			}
			dir = path;
			if (dir.Last() != separator)
				dir += separator;
			return dir;
		}
		else if (item == m_drives)
			return _T("/");
		else if (GetItemParent(item) == m_drives)
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
	wxTreeItemId child = GetFirstChild(m_drives, tmp);
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

	wxString error;
	if (!m_pState->SetLocalDir(dir, &error))
	{
		if (error != _T(""))
			wxMessageBox(error, _("Failed to change directory"), wxICON_INFORMATION);
		else
			wxBell();
		m_setSelection = true;
		SelectItem(event.GetOldItem());
		m_setSelection = false;
	}
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

void CLocalTreeView::OnStateChange(unsigned int event, const wxString& data)
{
	if (event == STATECHANGE_LOCAL_DIR)
		SetDir(m_pState->GetLocalDir());
	else if (event == STATECHANGE_APPLYFILTER)
		Refresh();
}

void CLocalTreeView::OnBeginDrag(wxTreeEvent& event)
{
	wxTreeItemId item = event.GetItem();
	if (!item)
		return;

#ifdef __WXMSW__
	if (item == m_drives || item == m_desktop || item == m_documents)
		return;
#endif

	wxString dir = GetDirFromItem(item);
	if (dir == _T("/"))
		return;

	if (dir.Last() == '\\')
		dir.RemoveLast();

#ifdef __WXMSW__
	if (dir.Last() == ':')
		return;
#endif

	wxFileDataObject obj;

	obj.AddFile(dir);

	wxDropSource source(this);
	source.SetData(obj);
	int res = source.DoDragDrop(wxDrag_AllowMove);
	if (res == wxDragCopy || res == wxDragMove)
		m_pState->RefreshLocal();
}

#ifndef __WXMSW__
void CLocalTreeView::OnKeyDown(wxKeyEvent& event)
{
	if (event.GetKeyCode() != WXK_TAB)
	{
		event.Skip();
		return;
	}

	wxNavigationKeyEvent navEvent;
	navEvent.SetEventObject(this);
	navEvent.SetDirection(!event.ShiftDown());
	navEvent.SetFromTab(true);
	navEvent.ResumePropagation(1);
	ProcessEvent(navEvent);
}
#endif

#ifdef __WXMSW__

wxString CLocalTreeView::GetSpecialFolder(int folder, int &iconIndex, int &openIconIndex)
{
	LPITEMIDLIST list;
	if (SHGetSpecialFolderLocation((HWND)GetHandle(), folder, &list) != S_OK)
		return _T("");

	SHFILEINFO shFinfo;
	if (!SHGetFileInfo((LPCTSTR)list, 0, &shFinfo, sizeof(shFinfo), SHGFI_PIDL | SHGFI_ICON | SHGFI_SMALLICON))
		return _T("");

	DestroyIcon(shFinfo.hIcon);
	iconIndex = shFinfo.iIcon;

	if (!SHGetFileInfo((LPCTSTR)list, 0, &shFinfo, sizeof(shFinfo), SHGFI_PIDL | SHGFI_ICON | SHGFI_SMALLICON | SHGFI_OPENICON | SHGFI_DISPLAYNAME))
		return _T("");

	DestroyIcon(shFinfo.hIcon);
	openIconIndex = shFinfo.iIcon;

	wxString name = shFinfo.szDisplayName;

	LPMALLOC pMalloc;
    SHGetMalloc(&pMalloc);

	if (pMalloc)
	{
		pMalloc->Free(list);
		pMalloc->Release();
	}
	else
		wxLogLastError(wxT("SHGetMalloc"));

	return name;
}

bool CLocalTreeView::CreateRoot()
{
	int iconIndex, openIconIndex;
	wxString name = GetSpecialFolder(CSIDL_DESKTOP, iconIndex, openIconIndex);
	if (name == _T(""))
	{
		name = _("Desktop");
		iconIndex = openIconIndex = -1;
	}
	
	m_desktop = AddRoot(name, iconIndex, openIconIndex);

	name = GetSpecialFolder(CSIDL_PERSONAL, iconIndex, openIconIndex);
	if (name == _T(""))
	{
		name = _("My Documents");
		iconIndex = openIconIndex = -1;
	}
	
	m_documents = AppendItem(m_desktop, name, iconIndex, openIconIndex);


	name = GetSpecialFolder(CSIDL_DRIVES, iconIndex, openIconIndex);
	if (name == _T(""))
	{
		name = _("My Computer");
		iconIndex = openIconIndex = -1;
	}
	
	m_drives = AppendItem(m_desktop, name, iconIndex, openIconIndex);

	DisplayDrives(m_drives);
	Expand(m_desktop);
	Expand(m_drives);

	return true;
}

void CLocalTreeView::OnVolumesEnumerated(wxCommandEvent& event)
{
	if (!m_pVolumeEnumeratorThread)
		return;

	std::list<CVolumeDescriptionEnumeratorThread::t_VolumeInfo> volumeInfo;
	volumeInfo = m_pVolumeEnumeratorThread->GetVolumes();

	delete m_pVolumeEnumeratorThread;
	m_pVolumeEnumeratorThread = 0;

	for (std::list<CVolumeDescriptionEnumeratorThread::t_VolumeInfo>::const_iterator iter = volumeInfo.begin(); iter != volumeInfo.end(); iter++)
	{
		wxString drive = iter->volume;

		wxTreeItemIdValue tmp;
		wxTreeItemId item = GetFirstChild(m_drives, tmp);
		while (item)
		{
			wxString name = GetItemText(item);
			if (name == drive || name.Left(drive.Len() + 1) == drive + _T(" "))
			{
				SetItemText(item, drive + _T(" (") + iter->volumeName + _T(")"));
				break;
			}
			item = GetNextChild(m_drives, tmp);
		}
	}
}

DEFINE_EVENT_TYPE(fzEVT_VOLUMESENUMERATED)

CVolumeDescriptionEnumeratorThread::CVolumeDescriptionEnumeratorThread(wxEvtHandler* pEvtHandler)
	: wxThreadEx(wxTHREAD_JOINABLE),
	  m_pEvtHandler(pEvtHandler)
{
	m_failure = false;
	m_stop = false;
	m_running = true;

	if (Create() != wxTHREAD_NO_ERROR)
	{
		m_running = false;
		m_failure = true;
	}
	Run();
}

CVolumeDescriptionEnumeratorThread::~CVolumeDescriptionEnumeratorThread()
{
	m_stop = true;
	Wait();

	for (std::list<t_VolumeInfoInternal>::const_iterator iter = m_volumeInfo.begin(); iter != m_volumeInfo.end(); iter++)
	{
		delete [] iter->pVolume;
		delete [] iter->pVolumeName;
	}
	m_volumeInfo.clear();
}

wxThread::ExitCode CVolumeDescriptionEnumeratorThread::Entry()
{
	if (!GetDrives())
		m_failure = true;

	m_running = false;

	wxCommandEvent evt(fzEVT_VOLUMESENUMERATED);
	m_pEvtHandler->AddPendingEvent(evt);

	return 0;
}

bool CVolumeDescriptionEnumeratorThread::GetDrives()
{
	long drivesToHide = 0;
	// Adhere to the NODRIVES group policy
	wxRegKey key(_T("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"));
	if (key.Exists())
	{
		if (!key.HasValue(_T("NoDrives")) || !key.QueryValue(_T("NoDrives"), &drivesToHide))
			drivesToHide = 0;
	}

	int len = GetLogicalDriveStrings(0, 0);
	if (!len)
		return false;

	wxChar* drives = new wxChar[len + 1];

	if (!GetLogicalDriveStrings(len, drives))
	{
		delete [] drives;
		return false;
	}

	const wxChar* pDrive = drives;
	while (*pDrive)
	{
		if (m_stop)
		{
			delete [] drives;
			return false;
		}

		// Check if drive should be hidden by default
		if (pDrive[0] != 0 && pDrive[1] == ':')
		{
			int bit = -1;
			char letter = pDrive[0];
			if (letter >= 'A' && letter <= 'Z')
				bit = 1 << (letter - 'A');
			if (letter >= 'a' && letter <= 'z')
				bit = 1 << (letter - 'a');

			if (bit != -1 && drivesToHide & bit)
			{
				pDrive += wxStrlen(pDrive) + 1;
				continue;
			}
		}

		int len = wxStrlen(pDrive);
		wxChar* pVolume = new wxChar[len + 1];
		wxStrcpy(pVolume, pDrive);
		if (pVolume[len - 1] == '\\')
			pVolume[len - 1] = 0;
		if (!*pVolume)
		{
			delete [] pVolume;
			pDrive += len + 1;
			continue;
		}

		// Get the label of the drive
		wxChar* pVolumeName = new wxChar[501];
		int oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
		BOOL res = GetVolumeInformation(pDrive, pVolumeName, 500, 0, 0, 0, 0, 0);
		SetErrorMode(oldErrorMode);
		if (res && pVolumeName[0])
		{
			t_VolumeInfoInternal volumeInfo;
			volumeInfo.pVolume = pVolume;
			volumeInfo.pVolumeName = pVolumeName;
			m_volumeInfo.push_back(volumeInfo);
		}
		else
		{
			delete [] pVolumeName;
			delete [] pVolume;
		}

		pDrive += wxStrlen(pDrive) + 1;
	}

	delete [] drives;

	return true;
}

std::list<CVolumeDescriptionEnumeratorThread::t_VolumeInfo> CVolumeDescriptionEnumeratorThread::GetVolumes()
{
	std::list<t_VolumeInfo> volumeInfo;

	if (m_running)
		return volumeInfo;

	for (std::list<t_VolumeInfoInternal>::const_iterator iter = m_volumeInfo.begin(); iter != m_volumeInfo.end(); iter++)
	{
		t_VolumeInfo info;
		info.volume = iter->pVolume;
		delete [] iter->pVolume;
		info.volumeName = iter->pVolumeName;
		delete [] iter->pVolumeName;
		volumeInfo.push_back(info);
	}
	m_volumeInfo.clear();

	return volumeInfo;
}

#endif
