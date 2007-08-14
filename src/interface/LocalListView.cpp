#include "FileZilla.h"
#include "LocalListView.h"
#include "queue.h"
#include "filezillaapp.h"
#include "filter.h"
#include "inputdialog.h"
#include <algorithm>
#include <wx/dnd.h>
#include "dndobjects.h"
#include "Options.h"
#ifdef __WXMSW__
#include "lm.h"
#include <wx/msw/registry.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CLocalListViewDropTarget : public wxDropTarget
{
public:
	CLocalListViewDropTarget(CLocalListView* pLocalListView)
		: m_pLocalListView(pLocalListView), m_pFileDataObject(new wxFileDataObject()),
		m_pRemoteDataObject(new CRemoteDataObject())
	{
		m_pDataObject = new wxDataObjectComposite;
		m_pDataObject->Add(m_pRemoteDataObject, true);
		m_pDataObject->Add(m_pFileDataObject, false);
		SetDataObject(m_pDataObject);
	}

	void ClearDropHighlight()
	{
		const int dropTarget = m_pLocalListView->m_dropTarget;
		if (dropTarget != -1)
		{
			m_pLocalListView->SetItemState(dropTarget, 0, wxLIST_STATE_DROPHILITED);
			m_pLocalListView->m_dropTarget = -1;
		}
	}
	
	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
	{
		if (def == wxDragError ||
			def == wxDragNone ||
			def == wxDragCancel)
			return def;

		if (m_pLocalListView->m_fileData.empty())
			return wxDragError;

		if (def != wxDragCopy && def != wxDragMove)
			return wxDragError;

		wxString subdir;
		int flags;
		int hit = m_pLocalListView->HitTest(wxPoint(x, y), flags, 0);
		if (hit != -1 && (flags & wxLIST_HITTEST_ONITEM))
		{
			const CLocalListView::t_fileData* const data = m_pLocalListView->GetData(hit);
			if (data && data->dir)
				subdir = data->name;
		}

		wxString dir;
		if (subdir != _T(""))
		{
			dir = CState::Canonicalize(m_pLocalListView->m_dir, subdir);
			if (dir == _T(""))
				return wxDragError;
		}
		else
			dir = m_pLocalListView->m_dir;

		if (!CState::LocalDirIsWriteable(dir))
			return wxDragError;

		if (!GetData())
			return wxDragError;

		if (m_pDataObject->GetReceivedFormat() == m_pFileDataObject->GetFormat())
			m_pLocalListView->m_pState->HandleDroppedFiles(m_pFileDataObject, dir, def == wxDragCopy);
		else
		{
			if (m_pRemoteDataObject->GetProcessId() != (int)wxGetProcessId())
			{
				wxMessageBox(_("Drag&drop between different instances of FileZilla has not been implemented yet."));
				return wxDragNone;
			}

			if (!m_pLocalListView->m_pState->GetServer() || m_pRemoteDataObject->GetServer() != *m_pLocalListView->m_pState->GetServer())
			{
				wxMessageBox(_("Drag&drop between different servers has not been implemented yet."));
				return wxDragNone;
			}

			if (!m_pLocalListView->m_pState->DownloadDroppedFiles(m_pRemoteDataObject, dir))
				return wxDragNone;
		}

		return def;
	}

	virtual bool OnDrop(wxCoord x, wxCoord y)
	{
		ClearDropHighlight();

		if (m_pLocalListView->m_fileData.empty())
			return false;

		return true;
	}

	wxString DisplayDropHighlight(wxPoint point)
	{
		wxString subDir;

		int flags;
		int hit = m_pLocalListView->HitTest(point, flags, 0);
		if (!(flags & wxLIST_HITTEST_ONITEM))
			hit = -1;

		if (hit != -1)
		{
			const CLocalListView::t_fileData* const data = m_pLocalListView->GetData(hit);
			if (!data || !data->dir)
				hit = -1;
			else
				subDir = data->name;
		}
		if (hit != m_pLocalListView->m_dropTarget)
		{
			ClearDropHighlight();
			if (hit != -1)
			{
				m_pLocalListView->SetItemState(hit, wxLIST_STATE_DROPHILITED, wxLIST_STATE_DROPHILITED);
				m_pLocalListView->m_dropTarget = hit;
			}
		}

		return subDir;
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

		if (m_pLocalListView->m_fileData.empty())
		{
			ClearDropHighlight();
			return wxDragNone;
		}

		const wxString& subdir = DisplayDropHighlight(wxPoint(x, y));
		
		if (subdir == _T(""))
		{
			if (!CState::LocalDirIsWriteable(m_pLocalListView->m_dir))
				return wxDragNone;
		}
		else
		{
			wxString dir = CState::Canonicalize(m_pLocalListView->m_dir, subdir);
			if (dir == _T("") || !CState::LocalDirIsWriteable(dir))
				return wxDragNone;
		}

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
	CLocalListView *m_pLocalListView;
	wxFileDataObject* m_pFileDataObject;
	CRemoteDataObject* m_pRemoteDataObject;
	wxDataObjectComposite* m_pDataObject;
};

BEGIN_EVENT_TABLE(CLocalListView, wxListCtrl)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, CLocalListView::OnItemActivated)
	EVT_LIST_COL_CLICK(wxID_ANY, CLocalListView::OnColumnClicked)
	EVT_CONTEXT_MENU(CLocalListView::OnContextMenu)
	// Map both ID_UPLOAD and ID_ADDTOQUEUE to OnMenuUpload, code is identical
	EVT_MENU(XRCID("ID_UPLOAD"), CLocalListView::OnMenuUpload)
	EVT_MENU(XRCID("ID_ADDTOQUEUE"), CLocalListView::OnMenuUpload)
	EVT_MENU(XRCID("ID_MKDIR"), CLocalListView::OnMenuMkdir)
	EVT_MENU(XRCID("ID_DELETE"), CLocalListView::OnMenuDelete)
	EVT_MENU(XRCID("ID_RENAME"), CLocalListView::OnMenuRename)
	EVT_KEY_DOWN(CLocalListView::OnKeyDown)
	EVT_LIST_BEGIN_LABEL_EDIT(wxID_ANY, CLocalListView::OnBeginLabelEdit)
	EVT_LIST_END_LABEL_EDIT(wxID_ANY, CLocalListView::OnEndLabelEdit)
	EVT_LIST_BEGIN_DRAG(wxID_ANY, CLocalListView::OnBeginDrag)
END_EVENT_TABLE()

CLocalListView::CLocalListView(wxWindow* parent, wxWindowID id, CState *pState, CQueueView *pQueue)
	: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxLC_VIRTUAL | wxLC_REPORT | wxNO_BORDER | wxLC_EDIT_LABELS),
	CSystemImageList(16), CStateEventHandler(pState, STATECHANGE_LOCAL_DIR | STATECHANGE_APPLYFILTER | STATECHANGE_LOCAL_REFRESH_FILE)
{
	m_dropTarget = -1;

	m_hasParent = true;

	m_pQueue = pQueue;

	unsigned long widths[4] = { 120, 80, 100, 120 };

	if (!wxGetKeyState(WXK_SHIFT) || !wxGetKeyState(WXK_ALT) || !wxGetKeyState(WXK_CONTROL))
		COptions::Get()->ReadColumnWidths(OPTION_LOCALFILELIST_COLUMN_WIDTHS, 4, widths);

	InsertColumn(0, _("Filename"), wxLIST_FORMAT_LEFT, widths[0]);
	InsertColumn(1, _("Filesize"), wxLIST_FORMAT_RIGHT, widths[1]);
	InsertColumn(2, _("Filetype"), wxLIST_FORMAT_LEFT, widths[2]);
	InsertColumn(3, _("Last modified"), wxLIST_FORMAT_LEFT, widths[3]);

	m_sortColumn = 0;
	m_sortDirection = 0;

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

	TCHAR buffer[1000] = {0};
	HDITEM item;
	item.mask = HDI_TEXT;
	item.pszText = buffer;
	item.cchTextMax = 999;
	SendMessage(header, HDM_GETITEM, 0, (LPARAM)&item);

	SendMessage(header, HDM_SETIMAGELIST, 0, (LPARAM)m_pHeaderImageList->GetHandle());
#endif

	SetDropTarget(new CLocalListViewDropTarget(this));
}

CLocalListView::~CLocalListView()
{
#ifdef __WXMSW__
	delete m_pHeaderImageList;
#endif
}

bool CLocalListView::DisplayDir(wxString dirname)
{
	wxString focused;
	std::list<wxString> selectedNames;
	if (m_dir != dirname)
	{
		// Clear selection
		int item = -1;
		while (true)
		{
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
			if (item == -1)
				break;
			SetItemState(item, 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		}
		focused = _T("..");

		if (GetItemCount())
			EnsureVisible(0);
		m_dir = dirname;
	}
	else
	{
		// Remember which items were selected
		selectedNames = RememberSelectedItems(focused);
	}

	const int oldItemCount = m_indexMapping.size();

	m_fileData.clear();
	m_indexMapping.clear();

	m_hasParent = CState::LocalDirHasParent(dirname);

	if (m_hasParent)
	{
		t_fileData data;
		data.dir = true;
		data.icon = -2;
		data.name = _T("..");
		data.size = -1;
		data.hasTime = 0;
		m_fileData.push_back(data);
		m_indexMapping.push_back(0);
	}

#ifdef __WXMSW__
	if (dirname == _T("\\"))
	{
		DisplayDrives();
	}
	else if (dirname.Left(2) == _T("\\\\"))
	{
		int pos = dirname.Mid(2).Find('\\');
		if (pos != -1 && pos + 3 != (int)dirname.Len())
			goto regular_dir;

		// UNC path without shares
		DisplayShares(dirname);
	}
	else
#endif
	{
#ifdef __WXMSW__
regular_dir:
#endif
		CFilterDialog filter;

		wxDir dir(dirname);
		if (!dir.IsOpened())
		{
			SetItemCount(1);
			return false;
		}
		wxString file;
		bool found = dir.GetFirst(&file);
		int num = m_fileData.size();
		while (found)
		{
			if (file == _T(""))
			{
				wxGetApp().DisplayEncodingWarning();
				found = dir.GetNext(&file);
				continue;
			}
			t_fileData data;

			data.dir = wxFileName::DirExists(dirname + file);
			data.icon = -2;
			data.name = file;

			wxStructStat buf;
			int result;
			result = wxStat(dirname + file, &buf);

			if (!result)
			{
				data.hasTime = true;
				data.lastModified = wxDateTime(buf.st_mtime);
			}
			else
				data.hasTime = false;

			if (data.dir)
				data.size = -1;
			else
				data.size = result ? -1 : buf.st_size;

			m_fileData.push_back(data);
			if (!filter.FilenameFiltered(data.name, data.dir, data.size, true))
				m_indexMapping.push_back(num);
			num++;

			found = dir.GetNext(&file);
		}
	}

	if (m_dropTarget != -1)
	{
		t_fileData* data = GetData(m_dropTarget);
		if (!data || !data->dir)
		{
			SetItemState(m_dropTarget, 0, wxLIST_STATE_DROPHILITED);
			m_dropTarget = -1;
		}
	}

	const int count = m_indexMapping.size();
	if (oldItemCount != count)
		SetItemCount(count);

	SortList();

	ReselectItems(selectedNames, focused);

	Refresh();

	return true;
}

// Declared const due to design error in wxWidgets.
// Won't be fixed since a fix would break backwards compatibility
// Both functions use a const_cast<CLocalListView *>(this) and modify
// the instance.
wxString CLocalListView::OnGetItemText(long item, long column) const
{
	CLocalListView *pThis = const_cast<CLocalListView *>(this);
	t_fileData *data = pThis->GetData(item);
	if (!data)
		return _T("");

	if (!column)
		return data->name;
	else if (column == 1)
	{
		if (data->size < 0)
			return _T("");
		else
			return wxLongLong(data->size).ToString();
	}
	else if (column == 2)
	{
		if (!item && m_hasParent)
			return _T("");

		if (data->fileType == _T(""))
			data->fileType = pThis->GetType(data->name, data->dir);

		return data->fileType;
	}
	else if (column == 3)
	{
		if (!data->hasTime)
			return _T("");

		return data->lastModified.Format(_T("%c"));
	}
	return _T("");
}

// See comment to OnGetItemText
int CLocalListView::OnGetItemImage(long item) const
{
	CLocalListView *pThis = const_cast<CLocalListView *>(this);
	t_fileData *data = pThis->GetData(item);
	if (!data)
		return -1;
	int &icon = data->icon;

	if (icon == -2)
	{
		wxString path = _T("");
		if (data->name != _T(".."))
		{
#ifdef __WXMSW__
			if (m_dir == _T("\\"))
				path = data->name + _T("\\");
			else
#endif
				path = m_dir + data->name;
		}

		icon = pThis->GetIconIndex(data->dir ? dir : file, path);
	}
	return icon;
}

void CLocalListView::OnItemActivated(wxListEvent &event)
{
	int count = 0;
	bool back = false;

	int item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		count++;

		if (!item && m_hasParent)
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
		OnMenuUpload(cmdEvent);
		return;
	}

	item = event.GetIndex();

	t_fileData *data = GetData(item);
	if (!data)
		return;

	if (data->dir)
	{
		wxString error;
		if (!m_pState->SetLocalDir(data->name, &error))
		{
			if (error != _T(""))
				wxMessageBox(error, _("Failed to change directory"), wxICON_INFORMATION);
			else
				wxBell();
		}
		return;
	}

	const CServer* pServer = m_pState->GetServer();
	if (!pServer)
	{
		wxBell();
		return;
	}

	CServerPath path = m_pState->GetRemotePath();
	if (path.IsEmpty())
	{
		wxBell();
		return;
	}

	wxFileName fn(m_dir, data->name);

	m_pQueue->QueueFile(false, false, fn.GetFullPath(), data->name, path, *pServer, data->size);
}

#ifdef __WXMSW__
void CLocalListView::DisplayDrives()
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
		return;

	wxChar* drives = new wxChar[len + 1];

	if (!GetLogicalDriveStrings(len, drives))
	{
		delete [] drives;
		return;
	}

	const wxChar* pDrive = drives;

	int count = m_fileData.size();
	while(*pDrive)
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

		wxString path = pDrive;
		if (path.Right(1) == _T("\\"))
			path.Truncate(path.Length() - 1);

		t_fileData data;
		data.name = path;
		data.dir = true;
		data.icon = -2;
		data.size = -1;
		data.hasTime = false;

		m_fileData.push_back(data);
		m_indexMapping.push_back(count);
		pDrive += wxStrlen(pDrive) + 1;
		count++;
	}

	delete [] drives;
}

void CLocalListView::DisplayShares(wxString computer)
{
	SHARE_INFO_1* pShareInfo = 0;

	DWORD read, total;
	DWORD resume_handle = 0;

	if (computer.Last() == '\\')
		computer.RemoveLast();

	int j = m_fileData.size();
	int res = 0;
	do
	{
		res = NetShareEnum((TCHAR*)computer.c_str(), 1, (LPBYTE*)&pShareInfo, MAX_PREFERRED_LENGTH, &read, &total, &resume_handle);

		if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA)
			break;

		SHARE_INFO_1* p = pShareInfo;
		for (unsigned int i = 0; i < read; i++, p++)
		{
			if (p->shi1_type != STYPE_DISKTREE)
				continue;

			t_fileData data;
			data.name = p->shi1_netname;
			data.dir = true;
			data.icon = -2;
			data.size = -1;
			data.hasTime = false;

			m_fileData.push_back(data);
			m_indexMapping.push_back(j++);
		}

		NetApiBufferFree(pShareInfo);
	}
	while (res == ERROR_MORE_DATA);
}

#endif

wxString CLocalListView::GetType(wxString name, bool dir)
{
#ifdef __WXMSW__
	wxString ext = wxFileName(name).GetExt();
	ext.MakeLower();
	std::map<wxString, wxString>::iterator typeIter = m_fileTypeMap.find(ext);
	if (typeIter != m_fileTypeMap.end())
		return typeIter->second;

	wxString type;

	wxString path;
	if (m_dir == _T("\\"))
		path = name + _T("\\");
	else
		path = m_dir + name;

	SHFILEINFO shFinfo;
	memset(&shFinfo, 0, sizeof(SHFILEINFO));
	if (SHGetFileInfo(path,
		dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL,
		&shFinfo,
		sizeof(shFinfo),
		SHGFI_TYPENAME))
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
	return dir ? _("Folder") : _("File");
#endif
}

CLocalListView::t_fileData* CLocalListView::GetData(unsigned int item)
{
	if (!IsItemValid(item))
		return 0;

	return &m_fileData[m_indexMapping[item]];
}

bool CLocalListView::IsItemValid(unsigned int item) const
{
	if (item >= m_indexMapping.size())
		return false;

	unsigned int index = m_indexMapping[item];
	if (index >= m_fileData.size())
		return false;

	return true;
}

// Helper classes for fast sorting using std::sort
// -----------------------------------------------

class CLocalListViewSort //: public std::binary_function<int,int,bool>
{
public:
	enum DirSortMode
	{
		dirsort_ontop,
		dirsort_onbottom,
		dirsort_inline
	};

	CLocalListViewSort(std::vector<CLocalListView::t_fileData>& fileData, enum DirSortMode dirSortMode)
		: m_fileData(fileData), m_dirSortMode(dirSortMode)
	{
	}

	virtual ~CLocalListViewSort() { }

	virtual bool operator()(int a, int b) const = 0;

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

	inline int CmpDir(const CLocalListView::t_fileData &data1, const CLocalListView::t_fileData &data2) const
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

	inline int CmpName(const CLocalListView::t_fileData &data1, const CLocalListView::t_fileData &data2) const
	{
#ifdef __WXMSW__
		return data1.name.CmpNoCase(data2.name);
#else
		return data1.name.Cmp(data2.name);
#endif
	}

	inline int CmpSize(const CLocalListView::t_fileData &data1, const CLocalListView::t_fileData &data2) const
	{
		const wxLongLong diff = data1.size - data2.size;
		if (diff < 0)
			return -1;
		else if (diff > 0)
			return 1;
		else
			return 0;
	}

	inline int CmpType(const CLocalListView::t_fileData &data1, const CLocalListView::t_fileData &data2) const
	{
		return data1.fileType.CmpNoCase(data2.fileType);
	}

	inline int CmpTime(const CLocalListView::t_fileData &data1, const CLocalListView::t_fileData &data2) const
	{
		if (!data1.hasTime)
		{
			if (data2.hasTime)
				return -1;
			else
				return 0;
		}
		else
		{
			if (!data2.hasTime)
				return 1;

			if (data1.lastModified < data2.lastModified)
				return -1;
			else if (data1.lastModified > data2.lastModified)
				return 1;
			else
				return 0;
		}
	}

protected:
	std::vector<CLocalListView::t_fileData>& m_fileData;

	DirSortMode m_dirSortMode;
};

class CLocalListViewSortObject : public std::binary_function<int,int,bool>
{
public:
	CLocalListViewSortObject(CLocalListViewSort* pObject)
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
	CLocalListViewSort* m_pObject;
};

template<class T> class CReverseSort : public T
{
public:
	CReverseSort(std::vector<CLocalListView::t_fileData>& fileData, enum CLocalListViewSort::DirSortMode dirSortMode, CLocalListView* pListView)
		: T(fileData, dirSortMode, pListView)
	{
	}

	inline bool operator()(int a, int b) const
	{
		return T::operator()(b, a);
	}
};

class CLocalListViewSortName : public CLocalListViewSort
{
public:
	CLocalListViewSortName(std::vector<CLocalListView::t_fileData>& fileData, enum DirSortMode dirSortMode, CLocalListView* pListView)
		: CLocalListViewSort(fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CLocalListView::t_fileData &data1 = m_fileData[a];
		const CLocalListView::t_fileData &data2 = m_fileData[b];

		CMP(CmpDir, data1, data2);

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CLocalListViewSortName> CLocalListViewSortName_Reverse;

class CLocalListViewSortSize : public CLocalListViewSort
{
public:
	CLocalListViewSortSize(std::vector<CLocalListView::t_fileData>& fileData, enum DirSortMode dirSortMode, CLocalListView* pListView)
		: CLocalListViewSort(fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CLocalListView::t_fileData &data1 = m_fileData[a];
		const CLocalListView::t_fileData &data2 = m_fileData[b];

		CMP(CmpDir, data1, data2);

		if (!data1.dir)
			CMP(CmpSize, data1, data2)

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CLocalListViewSortSize> CLocalListViewSortSize_Reverse;

class CLocalListViewSortType : public CLocalListViewSort
{
public:
	CLocalListViewSortType(std::vector<CLocalListView::t_fileData>& fileData, enum DirSortMode dirSortMode, CLocalListView* pListView)
		: CLocalListViewSort(fileData, dirSortMode)
	{
		m_pListView = pListView;
	}

	bool operator()(int a, int b) const
	{
		CLocalListView::t_fileData &data1 = m_fileData[a];
		CLocalListView::t_fileData &data2 = m_fileData[b];

		if (data1.fileType == _T(""))
			data1.fileType = m_pListView->GetType(data1.name, data1.dir);
		if (data2.fileType == _T(""))
			data2.fileType = m_pListView->GetType(data2.name, data2.dir);

		CMP(CmpDir, data1, data2);

		CMP(CmpType, data1, data2)

		CMP_LESS(CmpName, data1, data2);
	}

protected:
	CLocalListView* m_pListView;
};
typedef CReverseSort<CLocalListViewSortType> CLocalListViewSortType_Reverse;

class CLocalListViewSortTime : public CLocalListViewSort
{
public:
	CLocalListViewSortTime(std::vector<CLocalListView::t_fileData>& fileData, enum DirSortMode dirSortMode, CLocalListView* pListView)
		: CLocalListViewSort(fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CLocalListView::t_fileData &data1 = m_fileData[a];
		const CLocalListView::t_fileData &data2 = m_fileData[b];

		CMP(CmpDir, data1, data2);

		CMP(CmpTime, data1, data2)

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CLocalListViewSortTime> CLocalListViewSortTime_Reverse;

CLocalListViewSortObject CLocalListView::GetComparisonObject()
{
	const int dirSortOption = COptions::Get()->GetOptionVal(OPTION_FILELIST_DIRSORT);

	CLocalListViewSort::DirSortMode dirSortMode;
	switch (dirSortOption)
	{
	case 0:
	default:
		dirSortMode = CLocalListViewSort::dirsort_ontop;
		break;
	case 1:
		if (m_sortDirection)
			dirSortMode = CLocalListViewSort::dirsort_onbottom;
		else
			dirSortMode = CLocalListViewSort::dirsort_ontop;
		break;
	case 2:
		dirSortMode = CLocalListViewSort::dirsort_inline;
		break;
	}

	if (!m_sortDirection)
	{
		if (m_sortColumn == 1)
			return CLocalListViewSortObject(new CLocalListViewSortSize(m_fileData, dirSortMode, this));
		else if (m_sortColumn == 2)
			return CLocalListViewSortObject(new CLocalListViewSortType(m_fileData, dirSortMode, this));
		else if (m_sortColumn == 3)
			return CLocalListViewSortObject(new CLocalListViewSortTime(m_fileData, dirSortMode, this));
		else
			return CLocalListViewSortObject(new CLocalListViewSortName(m_fileData, dirSortMode, this));
	}
	else
	{
		if (m_sortColumn == 1)
			return CLocalListViewSortObject(new CLocalListViewSortSize_Reverse(m_fileData, dirSortMode, this));
		else if (m_sortColumn == 2)
			return CLocalListViewSortObject(new CLocalListViewSortType_Reverse(m_fileData, dirSortMode, this));
		else if (m_sortColumn == 3)
			return CLocalListViewSortObject(new CLocalListViewSortTime_Reverse(m_fileData, dirSortMode, this));
		else
			return CLocalListViewSortObject(new CLocalListViewSortName_Reverse(m_fileData, dirSortMode, this));
	}
}

void CLocalListView::SortList(int column /*=-1*/, int direction /*=-1*/)
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
		m_sortColumn = column;
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
		std::vector<unsigned int>::iterator start = m_indexMapping.begin();
		if (m_hasParent)
			start++;
		std::reverse(start, m_indexMapping.end());

		SortList_UpdateSelections(selected, focused);
		delete [] selected;

		return;
	}

	m_sortDirection = direction;
	m_sortColumn = column;

	const unsigned int minsize = m_hasParent ? 3 : 2;
	if (m_indexMapping.size() < minsize)
	{
		delete [] selected;
		return;
	}

	std::vector<unsigned int>::iterator start = m_indexMapping.begin();
	if (m_hasParent)
		start++;
	CLocalListViewSortObject object = GetComparisonObject();
	std::sort(start, m_indexMapping.end(), object);
	object.Destroy();

	SortList_UpdateSelections(selected, focused);
	delete [] selected;
}

void CLocalListView::SortList_UpdateSelections(bool* selections, int focus)
{
	for (int i = m_hasParent ? 1 : 0; i < GetItemCount(); i++)
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

void CLocalListView::OnColumnClicked(wxListEvent &event)
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
	Refresh(false);
}

void CLocalListView::OnContextMenu(wxContextMenuEvent& event)
{
	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_LOCALFILELIST"));
	if (!pMenu)
		return;

	int index = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	int count = 0;
	while (index != -1)
	{
		count++;
		if (!index && m_hasParent)
		{
			pMenu->Enable(XRCID("ID_UPLOAD"), false);
			pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
			pMenu->Enable(XRCID("ID_DELETE"), false);
			pMenu->Enable(XRCID("ID_RENAME"), false);
		}
		index = GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
	if (!count)
	{
		pMenu->Enable(XRCID("ID_UPLOAD"), false);
		pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
		pMenu->Enable(XRCID("ID_DELETE"), false);
		pMenu->Enable(XRCID("ID_RENAME"), false);
	}
	else if (count > 1)
		pMenu->Enable(XRCID("ID_RENAME"), false);

	PopupMenu(pMenu);
	delete pMenu;
}

void CLocalListView::OnMenuUpload(wxCommandEvent& event)
{
	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item && m_hasParent)
			return;
	}

	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		t_fileData *data = GetData(item);
		if (!data)
			return;

		if (!item && m_hasParent)
			m_pState->SetLocalDir(data->name);
		else
		{
			const CServer* pServer = m_pState->GetServer();
			if (!pServer)
			{
				wxBell();
				return;
			}

			CServerPath path = m_pState->GetRemotePath();
			if (path.IsEmpty())
			{
				wxBell();
				return;
			}

			if (data->dir)
			{
				path.ChangePath(data->name);

				wxFileName fn(m_dir, _T(""));
				fn.AppendDir(data->name);
				m_pQueue->QueueFolder(event.GetId() == XRCID("ID_ADDTOQUEUE"), false, fn.GetPath(), path, *pServer);
			}
			else
			{
				wxFileName fn(m_dir, data->name);

				m_pQueue->QueueFile(event.GetId() == XRCID("ID_ADDTOQUEUE"), false, fn.GetFullPath(), data->name, path, *pServer, data->size);
			}
		}
	}
}

void CLocalListView::OnMenuMkdir(wxCommandEvent& event)
{
	CInputDialog dlg;
	if (!dlg.Create(this, _("Create directory"), _("Please enter the name of the directory which should be created:")))
		return;

	if (dlg.ShowModal() != wxID_OK)
		return;

	if (dlg.GetValue() == _T(""))
	{
		wxBell();
		return;
	}

	wxFileName fn(dlg.GetValue(), _T(""));
	fn.Normalize(wxPATH_NORM_ALL, m_dir);

	bool res;

	{
		wxLogNull log;
		res = fn.Mkdir(fn.GetPath(), 0777, wxPATH_MKDIR_FULL);
	}

	if (!res)
		wxBell();

	DisplayDir(m_dir);
}

void CLocalListView::OnMenuDelete(wxCommandEvent& event)
{
	// Under Windows use SHFileOperation to delete files and directories.
	// Under other systems, we have to recurse into subdirectories manually
	// to delete all contents.

#ifdef __WXMSW__
	// SHFileOperation accepts a list of null-terminated strings. Go through all
	// items to get the required buffer length

	wxString dir = m_dir;
	if (dir.Right(1) != _T("\\") && dir.Right(1) != _T("/"))
		dir += _T("\\");
	int dirLen = dir.Length();

	int len = 1; // String list terminated by empty string

	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item && m_hasParent)
			continue;

		t_fileData *data = GetData(item);
		if (!data)
			continue;

		len += dirLen + data->name.Length() + 1;
	}

	// Allocate memory
	wxChar* pBuffer = new wxChar[len];
	wxChar* p = pBuffer;

	// Loop through all selected items again and fill buffer
	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item && m_hasParent)
			continue;

		t_fileData *data = GetData(item);
		if (!data)
			continue;

		_tcscpy(p, dir);
		p += dirLen;
		_tcscpy(p, data->name);
		p += data->name.Length() + 1;
	}
	*p = 0;

	// Now we can delete the files in the buffer
	SHFILEOPSTRUCT op;
	memset(&op, 0, sizeof(op));
	op.hwnd = (HWND)GetHandle();
	op.wFunc = FO_DELETE;
	op.pFrom = pBuffer;

	// Move to trash if shift is not pressed, else delete
	op.fFlags = wxGetKeyState(WXK_SHIFT) ? 0 : FOF_ALLOWUNDO;

	SHFileOperation(&op);
	delete [] pBuffer;
#else
	if (wxMessageBox(_("Really delete all selected files and/or directories?"), _("Confirmation needed"), wxICON_QUESTION | wxYES_NO, this) != wxYES)
		return;

	// Remember the directories to delete and the directories to visit
	std::list<wxString> dirsToDelete;
	std::list<wxString> dirsToVisit;

	wxString dir = m_dir;
	if (dir.Right(1) != _T("\\") && dir.Right(1) != _T("/"))
		dir += _T("/");

	// First process selected items
	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item && m_hasParent)
			continue;

		t_fileData *data = GetData(item);
		if (!data)
			continue;

		if (data->dir)
		{
			wxString subDir = dir + data->name + _T("/");
			dirsToVisit.push_back(subDir);
			dirsToDelete.push_front(subDir);
		}
		else
			wxRemoveFile(dir + data->name);
	}

	// Process any subdirs which still have to be visited
	while (!dirsToVisit.empty())
	{
		wxString filename = dirsToVisit.front();
		dirsToVisit.pop_front();
		wxDir dir;
		if (!dir.Open(filename))
			continue;

		wxString file;
		for (bool found = dir.GetFirst(&file); found; found = dir.GetNext(&file))
		{
			if (file == _T(""))
			{
				wxGetApp().DisplayEncodingWarning();
				continue;
			}
			if (wxFileName::DirExists(filename + file))
			{
				wxString subDir = filename + file + _T("/");
				dirsToVisit.push_back(subDir);
				dirsToDelete.push_front(subDir);
			}
			else
				wxRemoveFile(filename + file);
		}
	}

	// Delete the now empty directories
	for (std::list<wxString>::const_iterator iter = dirsToDelete.begin(); iter != dirsToDelete.end(); iter++)
		wxRmdir(*iter);

#endif
	m_pState->SetLocalDir(m_dir);
}

void CLocalListView::OnMenuRename(wxCommandEvent& event)
{
	int item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item < 0 || (!item && m_hasParent))
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

void CLocalListView::OnChar(wxKeyEvent& event)
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
			wxString text = GetData(item)->name;
			if (text.Length() >= m_prefix.Length() && !m_prefix.CmpNoCase(text.Left(m_prefix.Length())))
				beep = true;
		}
		else if (m_prefix == _T(""))
			beep = true;

		int start = item;
		if (start < 0)
			start = 0;

		int newPos = FindItemWithPrefix(newPrefix, start);

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

void CLocalListView::OnKeyDown(wxKeyEvent& event)
{
	const int code = event.GetKeyCode();
	if (code == 'A' && event.GetModifiers() == wxMOD_CMD)
	{
		for (int i = m_hasParent ? 1 : 0; i < GetItemCount(); i++)
		{
			SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
		}
	}
	else
		OnChar(event);
		//event.Skip();
}

int CLocalListView::FindItemWithPrefix(const wxString& prefix, int start)
{
	const int itemCount = m_fileData.size();
	for (int i = start; i < (itemCount + start); i++)
	{
		int item = i % itemCount;
		wxString fn;

		t_fileData* data = GetData(item);
		if (!data)
			continue;
		fn = data->name.Left(prefix.Length());

		if (!fn.CmpNoCase(prefix))
			return i % itemCount;
	}
	return -1;
}

void CLocalListView::OnBeginLabelEdit(wxListEvent& event)
{
	if (!m_hasParent)
		return;

	if (event.GetIndex() == 0)
	{
		event.Veto();
		return;
	}
}

void CLocalListView::OnEndLabelEdit(wxListEvent& event)
{
	if (event.IsEditCancelled())
		return;

	int index = event.GetIndex();
	if (!index && m_hasParent)
	{
		event.Veto();
		return;
	}

	if (event.GetLabel() == _T(""))
	{
		event.Veto();
		return;
	}

	wxString newname = event.GetLabel();
#ifdef __WXMSW__
	newname = newname.Left(255);

	if ((newname.Find('/') != -1) ||
		(newname.Find('\\') != -1) ||
		(newname.Find(':') != -1) ||
		(newname.Find('*') != -1) ||
		(newname.Find('?') != -1) ||
		(newname.Find('"') != -1) ||
		(newname.Find('<') != -1) ||
		(newname.Find('>') != -1) ||
		(newname.Find('|') != -1))
	{
		wxMessageBox(_("Filenames may not contain any of the following characters: / \\ : * ? \" < > |"), _("Invalid filename"), wxICON_EXCLAMATION);
		event.Veto();
		return;
	}

	SHFILEOPSTRUCT op;
	memset(&op, 0, sizeof(op));

	wxString dir = m_dir;
	if (dir.Right(1) != _T("\\") && dir.Right(1) != _T("/"))
		dir += _T("\\");
	wxString from = dir + m_fileData[m_indexMapping[index]].name + _T(" ");
	from.SetChar(from.Length() - 1, '\0');
	op.pFrom = from;
	wxString to = dir + newname + _T(" ");
	to.SetChar(to.Length()-1, '\0');
	op.pTo = to;
	op.hwnd = (HWND)GetHandle();
	op.wFunc = FO_RENAME;
	op.fFlags = FOF_ALLOWUNDO;
	if (SHFileOperation(&op))
		event.Veto();
	else
	{
		m_fileData[m_indexMapping[index]].name = newname;
		return;
	}
#else
	if ((newname.Find('/') != -1) ||
		(newname.Find('*') != -1) ||
		(newname.Find('?') != -1) ||
		(newname.Find('<') != -1) ||
		(newname.Find('>') != -1) ||
		(newname.Find('|') != -1))
	{
		wxMessageBox(_("Filenames may not contain any of the following characters: / * ? < > |"), _("Invalid filename"), wxICON_EXCLAMATION);
		event.Veto();
		return;
	}

	wxString dir = m_dir;
	if (dir.Right(1) != _T("\\") && dir.Right(1) != _T("/"))
		dir += _T("\\");
	if (wxRename(dir + m_fileData[m_indexMapping[index]].name, dir + newname))
		m_fileData[m_indexMapping[index]].name = newname;
	else
		event.Veto();
#endif
}

void CLocalListView::ApplyCurrentFilter()
{
	unsigned int min = m_hasParent ? 1 : 0;
	if (m_fileData.size() <= min)
		return;

	wxString focused;
	const std::list<wxString>& selectedNames = RememberSelectedItems(focused);

	CFilterDialog filter;
	m_indexMapping.clear();
	m_indexMapping.push_back(0);
	for (unsigned int i = min; i < m_fileData.size(); i++)
	{
		const t_fileData& data = m_fileData[i];
		if (!filter.FilenameFiltered(data.name, data.dir, data.size, true))
			m_indexMapping.push_back(i);
	}
	SetItemCount(m_indexMapping.size());

	SortList();

	ReselectItems(selectedNames, focused);
}

std::list<wxString> CLocalListView::RememberSelectedItems(wxString& focused)
{
	std::list<wxString> selectedNames;
	// Remember which items were selected
	int item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;
		const t_fileData &data = m_fileData[m_indexMapping[item]];
		if (data.dir)
			selectedNames.push_back(_T("d") + data.name);
		else
			selectedNames.push_back(_T("-") + data.name);
		SetItemState(item, 0, wxLIST_STATE_SELECTED);
	}

	item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
	if (item != -1)
	{
		const t_fileData &data = m_fileData[m_indexMapping[item]];
		focused = data.name;

		SetItemState(item, 0, wxLIST_STATE_FOCUSED);
	}

	return selectedNames;
}

void CLocalListView::ReselectItems(const std::list<wxString>& selectedNames, wxString focused)
{
	// Reselect previous items if neccessary.
	// Sorting direction did not change. We just have to scan through items once

	if (selectedNames.empty())
	{
		if (focused == _T(""))
			return;
		for (unsigned int i = 0; i < m_indexMapping.size(); i++)
		{
			const t_fileData &data = m_fileData[m_indexMapping[i]];
			if (data.name == focused)
			{
				SetItemState(i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
				return;
			}
		}
		return;
	}

	int firstSelected = -1;

	unsigned i = 0;
	for (std::list<wxString>::const_iterator iter = selectedNames.begin(); iter != selectedNames.end(); iter++)
	{
		while (i < m_indexMapping.size())
		{
			const t_fileData &data = m_fileData[m_indexMapping[i]];
			if (data.name == focused)
			{
				SetItemState(i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
				focused = _T("");
			}
			if (data.dir && *iter == (_T("d") + data.name))
			{
				if (firstSelected == -1)
					firstSelected = i;
				SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
				i++;
				break;
			}
			else if (*iter == (_T("-") + data.name))
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

void CLocalListView::OnStateChange(unsigned int event, const wxString& data)
{
	if (event == STATECHANGE_LOCAL_DIR)
		DisplayDir(m_pState->GetLocalDir());
	else if (event == STATECHANGE_APPLYFILTER)
		ApplyCurrentFilter();
	else if (event == STATECHANGE_LOCAL_REFRESH_FILE)
		RefreshFile(data);
}

void CLocalListView::OnBeginDrag(wxListEvent& event)
{
	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		if (!item && m_hasParent)
			return;
	}

	wxFileDataObject obj;

	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		t_fileData *data = GetData(item);
		if (!data)
			return;

		obj.AddFile(m_dir + data->name);
	}

	wxDropSource source(this);
	source.SetData(obj);
	int res = source.DoDragDrop(wxDrag_AllowMove);
	if (res == wxDragCopy || res == wxDragMove)
		m_pState->RefreshLocal();
}

void CLocalListView::RefreshFile(const wxString& file)
{
	if (!wxFileName::FileExists(m_dir + file))
		return;

	t_fileData data;

	data.dir = wxFileName::DirExists(m_dir + file);
	data.icon = -2;
	data.name = file;

	wxStructStat buf;
	int result = wxStat(m_dir + file, &buf);

	if (!result)
	{
		data.hasTime = true;
		data.lastModified = wxDateTime(buf.st_mtime);
	}
	else
		data.hasTime = false;

	if (data.dir)
		data.size = -1;
	else
		data.size = result ? -1 : buf.st_size;

	// Look if file data already exists
	for (std::vector<t_fileData>::iterator iter = m_fileData.begin(); iter != m_fileData.end(); iter++)
	{
		const t_fileData& oldData = *iter;
		if (oldData.name != file)
			continue;

		// It exists, update entry
		data.fileType = oldData.fileType;

		*iter = data;
		if (m_sortColumn)
			SortList();
		Refresh(false);
		return;
	}

	// Insert new entry
	int index = m_fileData.size();
	m_fileData.push_back(data);
	
	// Find correct position in index mapping
	std::vector<unsigned int>::iterator start = m_indexMapping.begin();
	if (m_hasParent)
		start++;
	CLocalListViewSortObject compare = GetComparisonObject();
	std::vector<unsigned int>::iterator insertPos = std::lower_bound(start, m_indexMapping.end(), index, compare);
	compare.Destroy();

	const int item = insertPos - m_indexMapping.begin();
	m_indexMapping.insert(insertPos, index);
	SetItemCount(m_indexMapping.size());
	
	// Move selections
	int prevState = 0;
	for (unsigned int i = item; i < m_indexMapping.size(); i++)
	{
		int state = GetItemState(i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		if (state != prevState)
		{
			SetItemState(i, prevState, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
			prevState = state;
		}
	}

	Refresh(false);
}
