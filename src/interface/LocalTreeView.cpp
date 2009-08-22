#include "FileZilla.h"
#include "LocalTreeView.h"
#include "queue.h"
#include "filezillaapp.h"
#include "filter.h"
#include <wx/dnd.h>
#include "dndobjects.h"
#include "inputdialog.h"
#include "local_filesys.h"
#include "dragdropmanager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#include <shlobj.h>
#include <dbt.h>
#include "volume_enumerator.h"
#endif

class CTreeItemData : public wxTreeItemData
{
public:
	CTreeItemData(const wxString& known_subdir) : m_known_subdir(known_subdir) {}
	wxString m_known_subdir;
};

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

		const CLocalPath path(GetDirFromItem(hit));
		if (path.empty() || !path.IsWriteable())
			return wxDragNone;

		if (!GetData())
			return wxDragError;

		CDragDropManager* pDragDropManager = CDragDropManager::Get();
		if (pDragDropManager)
			pDragDropManager->pDropTarget = m_pLocalTreeView;

		if (m_pDataObject->GetReceivedFormat() == m_pFileDataObject->GetFormat())
			m_pLocalTreeView->m_pState->HandleDroppedFiles(m_pFileDataObject, path, def == wxDragCopy);
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

			if (!m_pLocalTreeView->m_pState->DownloadDroppedFiles(m_pRemoteDataObject, path))
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

		const wxString dir = GetDirFromItem(hit);
		if (dir == _T("") || !CLocalPath(dir).IsWriteable())
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
#ifdef __WXMSW__
EVT_TREE_SEL_CHANGING(wxID_ANY, CLocalTreeView::OnSelectionChanging)
#endif
EVT_TREE_SEL_CHANGED(wxID_ANY, CLocalTreeView::OnSelectionChanged)
EVT_TREE_BEGIN_DRAG(wxID_ANY, CLocalTreeView::OnBeginDrag)
#ifndef __WXMSW__
EVT_KEY_DOWN(CLocalTreeView::OnKeyDown)
#else
EVT_COMMAND(-1, fzEVT_VOLUMESENUMERATED, CLocalTreeView::OnVolumesEnumerated)
EVT_COMMAND(-1, fzEVT_VOLUMEENUMERATED, CLocalTreeView::OnVolumesEnumerated)
#endif //__WXMSW__
EVT_TREE_ITEM_MENU(wxID_ANY, CLocalTreeView::OnContextMenu)
EVT_MENU(XRCID("ID_UPLOAD"), CLocalTreeView::OnMenuUpload)
EVT_MENU(XRCID("ID_ADDTOQUEUE"), CLocalTreeView::OnMenuUpload)
EVT_MENU(XRCID("ID_DELETE"), CLocalTreeView::OnMenuDelete)
EVT_MENU(XRCID("ID_RENAME"), CLocalTreeView::OnMenuRename)
EVT_MENU(XRCID("ID_MKDIR"), CLocalTreeView::OnMenuMkdir)
EVT_TREE_BEGIN_LABEL_EDIT(wxID_ANY, CLocalTreeView::OnBeginLabelEdit)
EVT_TREE_END_LABEL_EDIT(wxID_ANY, CLocalTreeView::OnEndLabelEdit)
EVT_CHAR(CLocalTreeView::OnChar)
END_EVENT_TABLE()

CLocalTreeView::CLocalTreeView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueueView)
	: wxTreeCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxTR_EDIT_LABELS | wxTR_LINES_AT_ROOT | wxTR_HAS_BUTTONS | wxNO_BORDER),
	CSystemImageList(16),
	CStateEventHandler(pState),
	m_pQueueView(pQueueView)
{
#ifdef __WXMAC__
	SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
#endif

	pState->RegisterHandler(this, STATECHANGE_LOCAL_DIR);
	pState->RegisterHandler(this, STATECHANGE_APPLYFILTER);

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

		//Not needed, item stays unexpanded
		//if (child && GetItemText(child) == _T(""))
		//	DisplayDir(parent, localDir);

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
	{
		if (drive[1] == ':')
			return AddDrive(drive[0]);
		return wxTreeItemId();
	}
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
			int bit = 0;
			char letter = pDrive[0];
			if (letter >= 'A' && letter <= 'Z')
				bit = 1 << (letter - 'A');
			if (letter >= 'a' && letter <= 'z')
				bit = 1 << (letter - 'a');

			if (drivesToHide & bit)
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

void CLocalTreeView::DisplayDir(wxTreeItemId parent, const wxString& dirname, const wxString& knownSubdir /*=_T("")*/)
{
	CLocalFileSystem local_filesystem;

	{
		wxLogNull log;
		if (!local_filesystem.BeginFindFiles(dirname, true))
		{
			if (knownSubdir != _T(""))
			{
				wxTreeItemId item = GetSubdir(parent, knownSubdir);
				if (item != wxTreeItemId())
					return;

				const wxString fullName = dirname + knownSubdir;
				item = AppendItem(parent, knownSubdir, GetIconIndex(::dir, fullName),
#ifdef __WXMSW__
						-1
#else
						GetIconIndex(opened_dir, fullName)
#endif
					);
				CheckSubdirStatus(item, fullName);
			}
			else
			{
				m_setSelection = true;
				DeleteChildren(parent);
				m_setSelection = false;
			}
			return;
		}
	}

	wxASSERT(parent);
	m_setSelection = true;
	DeleteChildren(parent);
	m_setSelection = false;

	CFilterManager filter;

	bool matchedKnown = false;

	wxString file;
	bool wasLink;
	int attributes;
	bool is_dir;
	const wxLongLong size(-1);
	while (local_filesystem.GetNextFile(file, wasLink, is_dir, 0, 0, &attributes))
	{
		wxASSERT(is_dir);
		if (file == _T(""))
		{
			wxGetApp().DisplayEncodingWarning();
			continue;
		}

		wxString fullName = dirname + file;
#ifdef __WXMSW__
		if (file.CmpNoCase(knownSubdir))
#else
		if (file != knownSubdir)
#endif
		{
			if (filter.FilenameFiltered(file, dirname, true, size, true, attributes))
				continue;
		}
		else
			matchedKnown = true;

		wxTreeItemId item = AppendItem(parent, file, GetIconIndex(::dir, fullName),
#ifdef __WXMSW__
				-1
#else
				GetIconIndex(opened_dir, fullName)
#endif
			);

		CheckSubdirStatus(item, fullName);
	}

	if (!matchedKnown && knownSubdir != _T(""))
	{
		const wxString fullName = dirname + knownSubdir;
		wxTreeItemId item = AppendItem(parent, knownSubdir, GetIconIndex(::dir, fullName),
#ifdef __WXMSW__
				-1
#else
				GetIconIndex(opened_dir, fullName)
#endif
			);

		CheckSubdirStatus(item, fullName);
	}

	SortChildren(parent);
}

wxString CLocalTreeView::HasSubdir(const wxString& dirname)
{
	wxLogNull nullLog;

	CFilterManager filter;
	
	CLocalFileSystem local_filesystem;
	if (!local_filesystem.BeginFindFiles(dirname, true))
		return _T("");

	wxString file;
	bool wasLink;
	int attributes;
	bool is_dir;
	const wxLongLong size(-1);
	while (local_filesystem.GetNextFile(file, wasLink, is_dir, 0, 0, &attributes))
	{
		wxASSERT(is_dir);
		if (file == _T(""))
		{
			wxGetApp().DisplayEncodingWarning();
			continue;
		}

		if (filter.FilenameFiltered(file, dirname, true, size, true, attributes))
			continue;

		return file;
	}

	return _T("");
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

	// Not needed, stays unexpanded by default
	// DisplayDir(parent, dirname);
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

	CFilterManager filter;

	while (!dirsToCheck.empty())
	{
		t_dir dir = dirsToCheck.front();
		dirsToCheck.pop_front();

		CLocalFileSystem local_filesystem;
		if (!local_filesystem.BeginFindFiles(dir.dir, true))
		{
			// Dir does exist (listed in parent) but may not be accessible.
			// Recurse into children anyhow, they might be accessible again.
			wxTreeItemIdValue value;
			wxTreeItemId child = GetFirstChild(dir.item, value);
			while (child)
			{
				t_dir subdir;
				subdir.dir = dir.dir + GetItemText(child) + separator;
				subdir.item = child;
				dirsToCheck.push_back(subdir);

				child = GetNextSibling(child);
			}
			continue;
		}

		std::list<wxString> dirs;

		
		wxString file;
		const wxLongLong size(-1);
		bool was_link;
		bool is_dir;
		int attributes;
		while (local_filesystem.GetNextFile(file, was_link, is_dir, 0, 0, &attributes))
		{
			if (file == _T(""))
			{
				wxGetApp().DisplayEncodingWarning();
				continue;
			}

			if (filter.FilenameFiltered(file, dir.dir, true, size, true, attributes))
				continue;

			dirs.push_back(file);
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
					wxString path = dir.dir + *iter + separator;
					if (!CheckSubdirStatus(child, path))
					{
						t_dir subdir;
						subdir.dir = path;
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
				wxTreeItemId sel = GetSelection();
				while (sel && sel != child)
					sel = GetItemParent(sel);
				wxTreeItemId prev = GetPrevSibling(child);
				if (!sel)
					Delete(child);
				child = prev;
			}
			else if (cmp < 0)
			{
				wxString fullname = dir.dir + *iter + separator;
				wxTreeItemId newItem = AppendItem(dir.item, *iter, GetIconIndex(::dir, fullname),
#ifdef __WXMSW__
						-1
#else
						GetIconIndex(opened_dir, fullName)
#endif
					);

				CheckSubdirStatus(newItem, fullname);
				iter++;
				inserted = true;
			}
		}
		while (child)
		{
			wxTreeItemId sel = GetSelection();
			while (sel && sel != child)
				sel = GetItemParent(sel);
			wxTreeItemId prev = GetPrevSibling(child);
			if (!sel)
				Delete(child);
			child = prev;
		}
		while (iter != dirs.rend())
		{
			wxString fullname = dir.dir + *iter + separator;
			wxTreeItemId newItem = AppendItem(dir.item, *iter, GetIconIndex(::dir, fullname),
#ifdef __WXMSW__
					-1
#else
					GetIconIndex(opened_dir, fullName)
#endif
				);

			CheckSubdirStatus(newItem, fullname);
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

void CLocalTreeView::OnStateChange(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2)
{
	if (notification == STATECHANGE_LOCAL_DIR)
		SetDir(m_pState->GetLocalDir().GetPath());
	else
	{
		wxASSERT(notification == STATECHANGE_APPLYFILTER);
		Refresh();
	}
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

#ifdef __WXMSW__
	if (dir.Last() == '\\')
		dir.RemoveLast();
#endif
	if (dir.Last() == '/')
		dir.RemoveLast();

#ifdef __WXMSW__
	if (dir.Last() == ':')
		return;
#endif

	CDragDropManager* pDragDropManager = CDragDropManager::Init();
	pDragDropManager->pDragSource = this;

	wxFileDataObject obj;

	obj.AddFile(dir);

	wxDropSource source(this);
	source.SetData(obj);
	int res = source.DoDragDrop(wxDrag_AllowMove);

	bool handled_internally = pDragDropManager->pDropTarget != 0;

	pDragDropManager->Release();

	if (!handled_internally && (res == wxDragCopy || res == wxDragMove))
	{
		// We only need to refresh local side if the operation got handled
		// externally, the internal handlers do this for us already
		m_pState->RefreshLocal();
	}
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

	if (event.GetEventType() == fzEVT_VOLUMESENUMERATED)
	{
		delete m_pVolumeEnumeratorThread;
		m_pVolumeEnumeratorThread = 0;
	}

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

#endif

void CLocalTreeView::OnContextMenu(wxTreeEvent& event)
{
	m_contextMenuItem = event.GetItem();
	if (!m_contextMenuItem.IsOk())
		return;

	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_LOCALTREE"));
	if (!pMenu)
		return;

	const CLocalPath path(GetDirFromItem(m_contextMenuItem));

	const bool hasParent = path.HasParent();
	const bool writeable = path.IsWriteable();

	const bool remoteConnected = m_pState->IsRemoteConnected() && !m_pState->GetRemotePath().IsEmpty();

	pMenu->Enable(XRCID("ID_UPLOAD"), hasParent && remoteConnected);
	pMenu->Enable(XRCID("ID_ADDTOQUEUE"), hasParent && remoteConnected);
	pMenu->Enable(XRCID("ID_MKDIR"), writeable);
	pMenu->Enable(XRCID("ID_DELETE"), writeable && hasParent);
	pMenu->Enable(XRCID("ID_RENAME"), writeable && hasParent);

	PopupMenu(pMenu);
	delete pMenu;
}

void CLocalTreeView::OnMenuUpload(wxCommandEvent& event)
{
	if (!m_contextMenuItem.IsOk())
		return;

	wxString path = GetDirFromItem(m_contextMenuItem);

	if (!CLocalPath(path).HasParent())
		return;

	if (!m_pState->IsRemoteConnected())
		return;

	const CServer server = *m_pState->GetServer();
	CServerPath remotePath = m_pState->GetRemotePath();
	if (remotePath.IsEmpty())
		return;

	if (!remotePath.ChangePath(GetItemText(m_contextMenuItem)))
		return;

	if (path.Last() == wxFileName::GetPathSeparator())
		path.RemoveLast();
	m_pQueueView->QueueFolder(event.GetId() == XRCID("ID_ADDTOQUEUE"), false, path, remotePath, server);
}

void CLocalTreeView::OnMenuMkdir(wxCommandEvent& event)
{
	if (!m_contextMenuItem.IsOk())
		return;

	wxString path = GetDirFromItem(m_contextMenuItem);
	if (path.Last() != wxFileName::GetPathSeparator())
		path += wxFileName::GetPathSeparator();

	if (!CLocalPath(path).IsWriteable())
	{
		wxBell();
		return;
	}

	CInputDialog dlg;
	if (!dlg.Create(this, _("Create directory"), _("Please enter the name of the directory which should be created:")))
		return;

	wxString newName = _("New directory");
	dlg.SetValue(path + newName);
	dlg.SelectText(path.Len(), path.Len() + newName.Len());

	if (dlg.ShowModal() != wxID_OK)
		return;

	wxFileName fn(dlg.GetValue(), _T(""));
	if (!fn.Normalize(wxPATH_NORM_ALL, path))
	{
		wxBell();
		return;
	}

	bool res;
	{
		wxLogNull log;
		res = fn.Mkdir(fn.GetPath(), 0777, wxPATH_MKDIR_FULL);
	}

	if (!res)
		wxBell();

	Refresh();
	m_pState->RefreshLocal();
}

void CLocalTreeView::OnMenuRename(wxCommandEvent& event)
{
	if (!m_contextMenuItem.IsOk())
		return;

#ifdef __WXMSW__
	if (m_contextMenuItem == m_desktop || m_contextMenuItem == m_documents)
	{
		wxBell();
		return;
	}
#endif

	CLocalPath path(GetDirFromItem(m_contextMenuItem));
	if (!path.HasParent() || !path.IsWriteable())
	{
		wxBell();
		return;
	}

	EditLabel(m_contextMenuItem);
}

void CLocalTreeView::OnMenuDelete(wxCommandEvent& event)
{
	if (!m_contextMenuItem.IsOk())
		return;

	wxString path = GetDirFromItem(m_contextMenuItem);

	CLocalPath local_path(path);
	if (!local_path.HasParent() || !local_path.IsWriteable())
		return;

	if (!CLocalFileSystem::RecursiveDelete(path, this))
		wxGetApp().DisplayEncodingWarning();

	wxTreeItemId item = GetSelection();
	while (item && item != m_contextMenuItem)
		item = GetItemParent(item);

	if (!item)
	{
		if (GetItemParent(m_contextMenuItem) == GetSelection())
			m_pState->RefreshLocal();
		else
			Refresh();
		return;
	}

	if (path.Last() == wxFileName::GetPathSeparator())
		path.RemoveLast();
	int pos = path.Find(wxFileName::GetPathSeparator(), true);
	if (pos < 1)
		path = _T("/");
	else
		path = path.Left(pos);

	m_pState->SetLocalDir(path);
	Refresh();
}

void CLocalTreeView::OnBeginLabelEdit(wxTreeEvent& event)
{
	wxTreeItemId item = event.GetItem();

#ifdef __WXMSW__
	if (item == m_desktop || item == m_documents)
	{
		wxBell();
		event.Veto();
		return;
	}
#endif

	CLocalPath path(GetDirFromItem(item));

	if (!path.HasParent() || !path.IsWriteable())
	{
		wxBell();
		event.Veto();
		return;
	}
}

// Defined in LocalListView.cpp
extern bool Rename(wxWindow* pWnd, wxString dir, wxString from, wxString to);

void CLocalTreeView::OnEndLabelEdit(wxTreeEvent& event)
{
	if (event.IsEditCancelled())
	{
		event.Veto();
		return;
	}

	wxTreeItemId item = event.GetItem();

#ifdef __WXMSW__
	if (item == m_desktop || item == m_documents)
	{
		wxBell();
		event.Veto();
		return;
	}
#endif

	wxString path = GetDirFromItem(item);

	CLocalPath local_path(path);
	if (!local_path.HasParent() || !local_path.IsWriteable())
	{
		wxBell();
		event.Veto();
		return;
	}

	if (path.Last() == wxFileName::GetPathSeparator())
		path.RemoveLast();

	int pos = path.Find(wxFileName::GetPathSeparator(), true);
	wxASSERT(pos != -1);

	wxString parent = path.Left(pos + 1);

	const wxString& oldName = GetItemText(item);
	const wxString& newName = event.GetLabel();
	if (newName == _T(""))
	{
		wxBell();
		event.Veto();
		return;
	}

	wxASSERT(parent + oldName == path);

	if (oldName == newName)
		return;

	if (!Rename(this, parent, oldName, newName))
	{
		event.Veto();
		return;
	}

	// We may call SetLocalDir, item might be deleted by it, so
	// if we don't rename item now and veto the event, wxWidgets
	// might access deleted item.
	event.Veto();
	SetItemText(item, newName);

	wxTreeItemId currentSel = GetSelection();
	if (currentSel == wxTreeItemId())
	{
		Refresh();
		return;
	}

	if (item == currentSel)
	{
		m_pState->SetLocalDir(parent + newName);
		return;
	}

	wxString sub;

	wxTreeItemId tmp = currentSel;
	while (tmp != GetRootItem() && tmp != item)
	{
		sub = wxFileName::GetPathSeparator() + GetItemText(tmp) + sub;
		tmp = GetItemParent(tmp);
	}

	if (tmp == GetRootItem())
	{
		// Rename unrelated to current selection
		return;
	}

	// Current selection below renamed item
	m_pState->SetLocalDir(parent + newName + sub);
}

void CLocalTreeView::OnChar(wxKeyEvent& event)
{
	m_contextMenuItem = GetSelection();

	wxCommandEvent cmdEvt;
	if (event.GetKeyCode() == WXK_F2)
		OnMenuRename(cmdEvt);
	else if (event.GetKeyCode() == WXK_DELETE)
		OnMenuDelete(cmdEvt);
	else
		event.Skip();
}

bool CLocalTreeView::CheckSubdirStatus(wxTreeItemId& item, const wxString& path)
{
	wxTreeItemIdValue value;
	wxTreeItemId child = GetFirstChild(item, value);

	static const wxLongLong size(-1);

#ifdef __WXMAC__
	// By default, OS X has a list of servers mounted into /net,
	// listing that directory is slow.
	if (GetItemParent(item) == GetRootItem() && (path == _T("/net") || path == _T("/net/")))
	{
			CFilterManager filter;

			const int attributes = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
			if (!filter.FilenameFiltered(_T("localhost"), path, true, size, true, attributes))
			{
				if (!child)
					AppendItem(item, _T(""));
				return true;
			}
	}
#endif

	if (child)
	{
		if (GetItemText(child) != _T(""))
			return false;

		CTreeItemData* pData = (CTreeItemData*)GetItemData(child);
		if (pData)
		{
			bool wasLink;
			int attributes;
			enum CLocalFileSystem::local_fileType type;
			if (path.Last() == CLocalFileSystem::path_separator)
				type = CLocalFileSystem::GetFileInfo(path + pData->m_known_subdir, wasLink, 0, 0, &attributes);
			else
				type = CLocalFileSystem::GetFileInfo(path + CLocalFileSystem::path_separator + pData->m_known_subdir, wasLink, 0, 0, &attributes);
			if (type == CLocalFileSystem::dir)
			{
				CFilterManager filter;
				if (!filter.FilenameFiltered(pData->m_known_subdir, path, true, size, true, attributes))
					return true;
			}
		}
	}

	wxString sub = HasSubdir(path);
	if (sub != _T(""))
	{
		wxTreeItemId subItem = AppendItem(item, _T(""));
		SetItemData(subItem, new CTreeItemData(sub));
	}
	else if (child)
		Delete(child);

	return true;
}

#ifdef __WXMSW__
void CLocalTreeView::OnDevicechange(WPARAM wParam, LPARAM lParam)
{
	if (!m_drives)
		return;

	if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE)
	{
		DEV_BROADCAST_HDR* pHdr = (DEV_BROADCAST_HDR*)lParam;
		if (pHdr->dbch_devicetype != DBT_DEVTYP_VOLUME)
			return;

		// Added or removed volume
		DEV_BROADCAST_VOLUME* pVolume = (DEV_BROADCAST_VOLUME*)lParam;

		wxChar drive = 'A';
		int mask = 1;
		while (drive <= 'Z')
		{
			if (pVolume->dbcv_unitmask & mask)
			{
				if (wParam == DBT_DEVICEARRIVAL)
					AddDrive(drive);
				else
				{
					RemoveDrive(drive);

					if (pVolume->dbcv_flags & DBTF_MEDIA)
					{
						// E.g. disk removed from CD-ROM drive, need to keep the drive letter
						AddDrive(drive);
					}
				}
			}
			drive++;
			mask <<= 1;
		}

		if (GetSelection() == m_drives && m_pState)
			m_pState->RefreshLocal();
	}
}

wxTreeItemId CLocalTreeView::AddDrive(wxChar drive)
{
	// Adhere to the NODRIVES group policy
	long drivesToHide = 0;
	wxRegKey key(_T("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"));
	if (key.Exists())
	{
		if (!key.HasValue(_T("NoDrives")) || !key.QueryValue(_T("NoDrives"), &drivesToHide))
			drivesToHide = 0;
	}

	int bit = 0;
	if (drive >= 'A' && drive <= 'Z')
		bit = 1 << (drive - 'A');
	else if (drive >= 'a' && drive <= 'z')
		bit = 1 << (drive - 'a');
	if (drivesToHide & bit)
		return wxTreeItemId();

	wxString driveName = drive;
	driveName += _T(":");

	// Get the label of the drive
	wxChar volumeName[501];
	int oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
	BOOL res = GetVolumeInformation(driveName + _T("\\"), volumeName, 500, 0, 0, 0, 0, 0);
	SetErrorMode(oldErrorMode);

	wxString itemLabel = driveName;
	if (res && volumeName[0])
	{
		itemLabel += _T(" (");
		itemLabel += volumeName;
		itemLabel += _T(")");
	}

	wxTreeItemIdValue value;
	wxTreeItemId driveItem = GetFirstChild(m_drives, value);
	while (driveItem)
	{
		if (!GetItemText(driveItem).Left(2).CmpNoCase(driveName))
			break;

		driveItem = GetNextSibling(driveItem);
	}
	if (driveItem)
	{
		SetItemText(driveItem, itemLabel);
		int icon = GetIconIndex(dir, driveName + _T("\\"));
		SetItemImage(driveItem, icon, wxTreeItemIcon_Normal);
		SetItemImage(driveItem, icon, wxTreeItemIcon_Selected);
		SetItemImage(driveItem, icon, wxTreeItemIcon_Expanded);
		SetItemImage(driveItem, icon, wxTreeItemIcon_SelectedExpanded);

		return driveItem;
	}

	wxTreeItemId item = AppendItem(m_drives, itemLabel, GetIconIndex(dir, driveName + _T("\\")));
	AppendItem(item, _T(""));
	SortChildren(m_drives);

	return item;
}

void CLocalTreeView::RemoveDrive(wxChar drive)
{
	wxString driveName = drive;
	driveName += _T(":");
	wxTreeItemIdValue value;
	wxTreeItemId driveItem = GetFirstChild(m_drives, value);
	while (driveItem)
	{
		if (!GetItemText(driveItem).Left(2).CmpNoCase(driveName))
			break;

		driveItem = GetNextSibling(driveItem);
	}
	if (!driveItem)
		return;

	Delete(driveItem);
}

void CLocalTreeView::OnSelectionChanging(wxTreeEvent& event)
{
	// On-demand open icon for selected items
	wxTreeItemId item = event.GetItem();
	if (GetItemImage(item, wxTreeItemIcon_Selected) == -1)
	{
		int icon = GetIconIndex(opened_dir, GetDirFromItem(item));
		SetItemImage(item, icon, wxTreeItemIcon_Selected);
		SetItemImage(item, icon, wxTreeItemIcon_SelectedExpanded);
	}
}

#endif
