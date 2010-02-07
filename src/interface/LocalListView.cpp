#define FILELISTCTRL_INCLUDE_TEMPLATE_DEFINITION

#include "filezilla.h"
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
#include "volume_enumerator.h"
#endif
#include "edithandler.h"
#include "dragdropmanager.h"
#include "local_filesys.h"
#include "filelist_statusbar.h"
#include "sizeformatting.h"

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
			m_pLocalListView->m_dropTarget = -1;
#ifdef __WXMSW__
			m_pLocalListView->SetItemState(dropTarget, 0, wxLIST_STATE_DROPHILITED);
#else
			m_pLocalListView->RefreshItem(dropTarget);
#endif
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

		CDragDropManager* pDragDropManager = CDragDropManager::Get();
		if (pDragDropManager)
			pDragDropManager->pDropTarget = m_pLocalListView;

		wxString subdir;
		int flags;
		int hit = m_pLocalListView->HitTest(wxPoint(x, y), flags, 0);
		if (hit != -1 && (flags & wxLIST_HITTEST_ONITEM))
		{
			const CLocalFileData* const data = m_pLocalListView->GetData(hit);
			if (data && data->dir)
				subdir = data->name;
		}

		CLocalPath dir = m_pLocalListView->m_pState->GetLocalDir();
		if (subdir != _T(""))
		{
			if (!dir.ChangePath(subdir))
				return wxDragError;
		}

		if (!dir.IsWriteable())
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
			const CLocalFileData* const data = m_pLocalListView->GetData(hit);
			if (!data || !data->dir)
				hit = -1;
			else
			{
				const CDragDropManager* pDragDropManager = CDragDropManager::Get();
				if (pDragDropManager && pDragDropManager->pDragSource == m_pLocalListView)
				{
					if (m_pLocalListView->GetItemState(hit, wxLIST_STATE_SELECTED))
						hit = -1;
					else
						subDir = data->name;
				}
				else
					subDir = data->name;
			}
		}
		if (hit != m_pLocalListView->m_dropTarget)
		{
			ClearDropHighlight();
			if (hit != -1)
			{
				m_pLocalListView->m_dropTarget = hit;
#ifdef __WXMSW__
				m_pLocalListView->SetItemState(hit, wxLIST_STATE_DROPHILITED, wxLIST_STATE_DROPHILITED);
#else
				m_pLocalListView->RefreshItem(hit);
#endif
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

		CLocalPath dir = m_pLocalListView->m_pState->GetLocalDir();
		if (subdir == _T(""))
		{
			const CDragDropManager* pDragDropManager = CDragDropManager::Get();
			if (pDragDropManager && pDragDropManager->localParent == m_pLocalListView->m_dir)
				return wxDragNone;
		}
		else
		{
			if (!dir.ChangePath(subdir))
				return wxDragNone;
		}

		if (!dir.IsWriteable())
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
	CLocalListView *m_pLocalListView;
	wxFileDataObject* m_pFileDataObject;
	CRemoteDataObject* m_pRemoteDataObject;
	wxDataObjectComposite* m_pDataObject;
};

BEGIN_EVENT_TABLE(CLocalListView, CFileListCtrl<CLocalFileData>)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, CLocalListView::OnItemActivated)
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
	EVT_MENU(XRCID("ID_OPEN"), CLocalListView::OnMenuOpen)
	EVT_MENU(XRCID("ID_EDIT"), CLocalListView::OnMenuEdit)
	EVT_MENU(XRCID("ID_ENTER"), CLocalListView::OnMenuEnter)
#ifdef __WXMSW__
	EVT_COMMAND(-1, fzEVT_VOLUMESENUMERATED, CLocalListView::OnVolumesEnumerated)
	EVT_COMMAND(-1, fzEVT_VOLUMEENUMERATED, CLocalListView::OnVolumesEnumerated)
#endif
	EVT_MENU(XRCID("ID_CONTEXT_REFRESH"), CLocalListView::OnMenuRefresh)
END_EVENT_TABLE()

CLocalListView::CLocalListView(wxWindow* pParent, CState *pState, CQueueView *pQueue)
	: CFileListCtrl<CLocalFileData>(pParent, pState, pQueue),
	CStateEventHandler(pState)
{
	m_pState->RegisterHandler(this, STATECHANGE_LOCAL_DIR);
	m_pState->RegisterHandler(this, STATECHANGE_APPLYFILTER);
	m_pState->RegisterHandler(this, STATECHANGE_LOCAL_REFRESH_FILE);

	m_dropTarget = -1;

	const unsigned long widths[4] = { 120, 80, 100, 120 };

	AddColumn(_("Filename"), wxLIST_FORMAT_LEFT, widths[0], true);
	AddColumn(_("Filesize"), wxLIST_FORMAT_RIGHT, widths[1]);
	AddColumn(_("Filetype"), wxLIST_FORMAT_LEFT, widths[2]);
	AddColumn(_("Last modified"), wxLIST_FORMAT_LEFT, widths[3]);
	LoadColumnSettings(OPTION_LOCALFILELIST_COLUMN_WIDTHS, OPTION_LOCALFILELIST_COLUMN_SHOWN, OPTION_LOCALFILELIST_COLUMN_ORDER);

	InitSort(OPTION_LOCALFILELIST_SORTORDER);

	SetImageList(GetSystemImageList(), wxIMAGE_LIST_SMALL);

#ifdef __WXMSW__
	m_pVolumeEnumeratorThread = 0;
#endif

	InitHeaderImageList();

	SetDropTarget(new CLocalListViewDropTarget(this));

	InitDateFormat();

	EnablePrefixSearch(true);
}

CLocalListView::~CLocalListView()
{
	wxString str = wxString::Format(_T("%d %d"), m_sortDirection, m_sortColumn);
	COptions::Get()->SetOption(OPTION_LOCALFILELIST_SORTORDER, str);

#ifdef __WXMSW__
	delete m_pVolumeEnumeratorThread;
#endif
}

bool CLocalListView::DisplayDir(wxString dirname)
{
#ifdef __WXMSW__
	if (GetEditControl())
		ListView_CancelEditLabel((HWND)GetHandle());
#endif

	wxString focused;
	std::list<wxString> selectedNames;
	if (m_dir != dirname)
	{
		ResetSearchPrefix();

		if (IsComparing())
			ExitComparisonMode();

		ClearSelection();
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

	if (m_pFilelistStatusBar)
		m_pFilelistStatusBar->UnselectAll();

	const int oldItemCount = m_indexMapping.size();

	m_fileData.clear();
	m_indexMapping.clear();

	m_hasParent = CLocalPath(dirname).HasLogicalParent();

	if (m_hasParent)
	{
		CLocalFileData data;
		data.flags = normal;
		data.dir = true;
		data.icon = -2;
		data.name = _T("..");
#ifdef __WXMSW__
		data.label = _T("..");
#endif
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
		CFilterManager filter;
		CLocalFileSystem local_filesystem;

		if (!local_filesystem.BeginFindFiles(dirname, false))
		{
			SetItemCount(1);
			return false;
		}

		wxLongLong totalSize;
		int unknown_sizes = 0;
		int totalFileCount = 0;
		int totalDirCount = 0;
		int hidden = 0;

		int num = m_fileData.size();
		CLocalFileData data;
		data.flags = normal;
		data.icon = -2;
		bool wasLink;
		while (local_filesystem.GetNextFile(data.name, wasLink, data.dir, &data.size, &data.lastModified, &data.attributes))
		{
			if (data.name.IsEmpty())
			{
				wxGetApp().DisplayEncodingWarning();
				continue;
			}
#ifdef __WXMSW__
			data.label = data.name;
#endif
			data.hasTime = data.lastModified.IsValid();

			m_fileData.push_back(data);
			if (!filter.FilenameFiltered(data.name, dirname, data.dir, data.size, true, data.attributes))
			{
				if (data.dir)
					totalDirCount++;
				else
				{
					if (data.size != -1)
						totalSize += data.size;
					else
						unknown_sizes++;
					totalFileCount++;
				}
				m_indexMapping.push_back(num);
			}
			else
				hidden++;
			num++;
		}

		if (m_pFilelistStatusBar)
			m_pFilelistStatusBar->SetDirectoryContents(totalFileCount, totalDirCount, totalSize, unknown_sizes, hidden);
	}

	if (m_dropTarget != -1)
	{
		CLocalFileData* data = GetData(m_dropTarget);
		if (!data || !data->dir)
		{
			SetItemState(m_dropTarget, 0, wxLIST_STATE_DROPHILITED);
			m_dropTarget = -1;
		}
	}

	const int count = m_indexMapping.size();
	if (oldItemCount != count)
		SetItemCount(count);

	SortList(-1, -1, false);

	if (IsComparing())
	{
		m_originalIndexMapping.clear();
		RefreshComparison();
	}

	ReselectItems(selectedNames, focused);

	RefreshListOnly();

	return true;
}

// See comment to OnGetItemText
int CLocalListView::OnGetItemImage(long item) const
{
	CLocalListView *pThis = const_cast<CLocalListView *>(this);
	CLocalFileData *data = pThis->GetData(item);
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

	CLocalFileData *data = GetData(item);
	if (!data)
		return;

	if (data->dir)
	{
		const int action = COptions::Get()->GetOptionVal(OPTION_DOUBLECLICK_ACTION_DIRECTORY);
		if (action == 3)
		{
			// No action
			wxBell();
			return;
		}

		if (!action || data->name == _T(".."))
		{
			// Enter action

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

		wxCommandEvent evt(0, action == 1 ? XRCID("ID_UPLOAD") : XRCID("ID_ADDTOQUEUE"));
		OnMenuUpload(evt);
		return;
	}

	if (data->flags == fill)
	{
		wxBell();
		return;
	}

	const int action = COptions::Get()->GetOptionVal(OPTION_DOUBLECLICK_ACTION_FILE);
	if (action == 3)
	{
		// No action
		wxBell();
		return;
	}

	if (action == 2)
	{
		// View / Edit action
		wxCommandEvent evt;
		OnMenuEdit(evt);
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

	const bool queue_only = action == 1;

	m_pQueue->QueueFile(queue_only, false, fn.GetFullPath(), data->name, path, *pServer, data->size);
	m_pQueue->QueueFile_Finish(true);
}

void CLocalListView::OnMenuEnter(wxCommandEvent &event)
{
	int item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item == -1)
	{
		wxBell();
		return;
	}

	if (GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) != -1)
	{
		wxBell();
		return;
	}

	CLocalFileData *data = GetData(item);
	if (!data || !data->dir)
	{
		wxBell();
		return;
	}

	wxString error;
	if (!m_pState->SetLocalDir(data->name, &error))
	{
		if (error != _T(""))
			wxMessageBox(error, _("Failed to change directory"), wxICON_INFORMATION);
		else
			wxBell();
	}
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
	int drive_count = 0;
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

		drive_count++;

		wxString path = pDrive;
		if (path.Right(1) == _T("\\"))
			path.Truncate(path.Length() - 1);

		CLocalFileData data;
		data.flags = normal;
		data.name = path;
		data.label = data.name;
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

	if (m_pFilelistStatusBar)
		m_pFilelistStatusBar->SetDirectoryContents(0, drive_count, 0, false, 0);

	if (!m_pVolumeEnumeratorThread)
	{
		m_pVolumeEnumeratorThread = new CVolumeDescriptionEnumeratorThread(this);
		if (m_pVolumeEnumeratorThread->Failed())
		{
			delete m_pVolumeEnumeratorThread;
			m_pVolumeEnumeratorThread = 0;
		}
	}
}

void CLocalListView::DisplayShares(wxString computer)
{
	// Cast through a union to avoid warning about breaking strict aliasing rule
	union
	{
		SHARE_INFO_1* pShareInfo;
		LPBYTE pShareInfoBlob;
	} si;

	DWORD read, total;
	DWORD resume_handle = 0;

	if (computer.Last() == '\\')
		computer.RemoveLast();

	int j = m_fileData.size();
	int share_count = 0;
	int res = 0;
	do
	{
		const wxWX2WCbuf buf = computer.wc_str(wxConvLocal);
		res = NetShareEnum((wchar_t*)(const wchar_t*)buf, 1, &si.pShareInfoBlob, MAX_PREFERRED_LENGTH, &read, &total, &resume_handle);

		if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA)
			break;

		SHARE_INFO_1* p = si.pShareInfo;
		for (unsigned int i = 0; i < read; i++, p++)
		{
			if (p->shi1_type != STYPE_DISKTREE)
				continue;

			CLocalFileData data;
			data.flags = normal;
			data.name = p->shi1_netname;
#ifdef __WXMSW__
			data.label = data.name;
#endif
			data.dir = true;
			data.icon = -2;
			data.size = -1;
			data.hasTime = false;

			m_fileData.push_back(data);
			m_indexMapping.push_back(j++);

			share_count++;
		}

		NetApiBufferFree(si.pShareInfo);
	}
	while (res == ERROR_MORE_DATA);

	if (m_pFilelistStatusBar)
		m_pFilelistStatusBar->SetDirectoryContents(0, share_count, 0, false, 0);
}

#endif //__WXMSW__

CLocalFileData* CLocalListView::GetData(unsigned int item)
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

class CLocalListViewSort : public CListViewSort
{
public:
	CLocalListViewSort(std::vector<CLocalFileData>& fileData, enum DirSortMode dirSortMode)
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

	inline int CmpDir(const CLocalFileData &data1, const CLocalFileData &data2) const
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

	inline int CmpName(const CLocalFileData &data1, const CLocalFileData &data2) const
	{
#ifdef __WXMSW__
		return data1.name.CmpNoCase(data2.name);
#else
		return data1.name.Cmp(data2.name);
#endif
	}

	inline int CmpSize(const CLocalFileData &data1, const CLocalFileData &data2) const
	{
		const wxLongLong diff = data1.size - data2.size;
		if (diff < 0)
			return -1;
		else if (diff > 0)
			return 1;
		else
			return 0;
	}

	inline int CmpType(const CLocalFileData &data1, const CLocalFileData &data2) const
	{
		return data1.fileType.CmpNoCase(data2.fileType);
	}

	inline int CmpTime(const CLocalFileData &data1, const CLocalFileData &data2) const
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
	std::vector<CLocalFileData>& m_fileData;

	DirSortMode m_dirSortMode;
};

template<class T> class CReverseSort : public T
{
public:
	CReverseSort(std::vector<CLocalFileData>& fileData, enum CLocalListViewSort::DirSortMode dirSortMode, CLocalListView* pListView)
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
	CLocalListViewSortName(std::vector<CLocalFileData>& fileData, enum DirSortMode dirSortMode, CLocalListView* pListView)
		: CLocalListViewSort(fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CLocalFileData &data1 = m_fileData[a];
		const CLocalFileData &data2 = m_fileData[b];

		CMP(CmpDir, data1, data2);

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CLocalListViewSortName> CLocalListViewSortName_Reverse;

class CLocalListViewSortSize : public CLocalListViewSort
{
public:
	CLocalListViewSortSize(std::vector<CLocalFileData>& fileData, enum DirSortMode dirSortMode, CLocalListView* pListView)
		: CLocalListViewSort(fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CLocalFileData &data1 = m_fileData[a];
		const CLocalFileData &data2 = m_fileData[b];

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
	CLocalListViewSortType(std::vector<CLocalFileData>& fileData, enum DirSortMode dirSortMode, CLocalListView* pListView)
		: CLocalListViewSort(fileData, dirSortMode)
	{
		m_pListView = pListView;
	}

	bool operator()(int a, int b) const
	{
		CLocalFileData &data1 = m_fileData[a];
		CLocalFileData &data2 = m_fileData[b];

		if (data1.fileType.IsEmpty())
			data1.fileType = m_pListView->GetType(data1.name, data1.dir, m_pListView->m_dir);
		if (data2.fileType.IsEmpty())
			data2.fileType = m_pListView->GetType(data2.name, data2.dir, m_pListView->m_dir);

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
	CLocalListViewSortTime(std::vector<CLocalFileData>& fileData, enum DirSortMode dirSortMode, CLocalListView* pListView)
		: CLocalListViewSort(fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CLocalFileData &data1 = m_fileData[a];
		const CLocalFileData &data2 = m_fileData[b];

		CMP(CmpDir, data1, data2);

		CMP(CmpTime, data1, data2)

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CLocalListViewSortTime> CLocalListViewSortTime_Reverse;

CFileListCtrl<CLocalFileData>::CSortComparisonObject CLocalListView::GetSortComparisonObject()
{
	CLocalListViewSort::DirSortMode dirSortMode = GetDirSortMode();

	if (!m_sortDirection)
	{
		if (m_sortColumn == 1)
			return CFileListCtrl<CLocalFileData>::CSortComparisonObject(new CLocalListViewSortSize(m_fileData, dirSortMode, this));
		else if (m_sortColumn == 2)
			return CFileListCtrl<CLocalFileData>::CSortComparisonObject(new CLocalListViewSortType(m_fileData, dirSortMode, this));
		else if (m_sortColumn == 3)
			return CFileListCtrl<CLocalFileData>::CSortComparisonObject(new CLocalListViewSortTime(m_fileData, dirSortMode, this));
		else
			return CFileListCtrl<CLocalFileData>::CSortComparisonObject(new CLocalListViewSortName(m_fileData, dirSortMode, this));
	}
	else
	{
		if (m_sortColumn == 1)
			return CFileListCtrl<CLocalFileData>::CSortComparisonObject(new CLocalListViewSortSize_Reverse(m_fileData, dirSortMode, this));
		else if (m_sortColumn == 2)
			return CFileListCtrl<CLocalFileData>::CSortComparisonObject(new CLocalListViewSortType_Reverse(m_fileData, dirSortMode, this));
		else if (m_sortColumn == 3)
			return CFileListCtrl<CLocalFileData>::CSortComparisonObject(new CLocalListViewSortTime_Reverse(m_fileData, dirSortMode, this));
		else
			return CFileListCtrl<CLocalFileData>::CSortComparisonObject(new CLocalListViewSortName_Reverse(m_fileData, dirSortMode, this));
	}
}

void CLocalListView::OnContextMenu(wxContextMenuEvent& event)
{
	if (GetEditControl())
	{
		event.Skip();
		return;
	}

	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_LOCALFILELIST"));
	if (!pMenu)
		return;

	const bool connected = m_pState->IsRemoteConnected();
	if (!connected)
	{
		pMenu->Enable(XRCID("ID_EDIT"), COptions::Get()->GetOptionVal(OPTION_EDIT_TRACK_LOCAL) == 0);
		pMenu->Enable(XRCID("ID_UPLOAD"), false);
		pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
	}

	int index = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	int count = 0;
	int fillCount = 0;
	bool selectedDir = false;
	while (index != -1)
	{
		count++;
		const CLocalFileData* const data = GetData(index);
		if (!data || (!index && m_hasParent))
		{
			pMenu->Enable(XRCID("ID_UPLOAD"), false);
			pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
			pMenu->Enable(XRCID("ID_DELETE"), false);
			pMenu->Enable(XRCID("ID_RENAME"), false);
			pMenu->Enable(XRCID("ID_OPEN"), false);
			pMenu->Enable(XRCID("ID_EDIT"), false);
		}
		if (data->flags == fill)
			fillCount++;
		if (data->dir)
			selectedDir = true;
		index = GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
	if (!count || fillCount == count)
	{
		pMenu->Delete(XRCID("ID_ENTER"));
		pMenu->Enable(XRCID("ID_UPLOAD"), false);
		pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
		pMenu->Enable(XRCID("ID_DELETE"), false);
		pMenu->Enable(XRCID("ID_RENAME"), false);
		pMenu->Enable(XRCID("ID_EDIT"), false);
	}
	else if (count > 1)
	{
		pMenu->Delete(XRCID("ID_ENTER"));
		pMenu->Enable(XRCID("ID_RENAME"), false);
	}
	else
	{
		// Exactly one item selected
		if (!selectedDir)
			pMenu->Delete(XRCID("ID_ENTER"));
	}
	if (selectedDir)
		pMenu->Enable(XRCID("ID_EDIT"), false);

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

	const CServer* pServer = m_pState->GetServer();
	if (!pServer)
	{
		wxBell();
		return;
	}

	bool added = false;

	bool queue_only = event.GetId() == XRCID("ID_ADDTOQUEUE");

	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		const CLocalFileData *data = GetData(item);
		if (!data)
			break;

		if (data->flags == fill)
			continue;

		CServerPath path = m_pState->GetRemotePath();
		if (path.IsEmpty())
		{
			wxBell();
			break;
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

			m_pQueue->QueueFile(queue_only, false, fn.GetFullPath(), data->name, path, *pServer, data->size);
			added = true;
		}
	}
	if (added)
		m_pQueue->QueueFile_Finish(!queue_only);
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
	wxString dir = m_dir;
	if (dir.Right(1) != _T("\\") && dir.Right(1) != _T("/"))
		dir += _T("\\");

	std::list<wxString> pathsToDelete;
	long item = -1;
	while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
	{
		if (!item && m_hasParent)
			continue;

		CLocalFileData *data = GetData(item);
		if (!data)
			continue;

		if (data->flags == fill)
			continue;

		pathsToDelete.push_back(dir + data->name);
	}
	if (!CLocalFileSystem::RecursiveDelete(pathsToDelete, this))
		wxGetApp().DisplayEncodingWarning();

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

	CLocalFileData *data = GetData(item);
	if (!data || data->flags == fill)
	{
		wxBell();
		return;
	}

	EditLabel(item);
}

void CLocalListView::OnKeyDown(wxKeyEvent& event)
{
	const int code = event.GetKeyCode();
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
	else if (code == WXK_BACK)
	{
		if (!m_hasParent)
		{
			wxBell();
			return;
		}

		wxString error;
		if (!m_pState->SetLocalDir(_T(".."), &error))
		{
			if (error != _T(""))
				wxMessageBox(error, _("Failed to change directory"), wxICON_INFORMATION);
			else
				wxBell();
		}
	}
	else
		event.Skip();
}

void CLocalListView::OnBeginLabelEdit(wxListEvent& event)
{
	if (!m_pState->GetLocalDir().IsWriteable())
	{
		event.Veto();
		return;
	}

	if (event.GetIndex() == 0 && m_hasParent)
	{
		event.Veto();
		return;
	}

	const CLocalFileData * const data = GetData(event.GetIndex());
	if (!data || data->flags == fill)
	{
		event.Veto();
		return;
	}
}

bool Rename(wxWindow* parent, wxString dir, wxString from, wxString to)
{
	if (dir.Right(1) != _T("\\") && dir.Right(1) != _T("/"))
		dir += wxFileName::GetPathSeparator();

#ifdef __WXMSW__
	to = to.Left(255);

	if ((to.Find('/') != -1) ||
		(to.Find('\\') != -1) ||
		(to.Find(':') != -1) ||
		(to.Find('*') != -1) ||
		(to.Find('?') != -1) ||
		(to.Find('"') != -1) ||
		(to.Find('<') != -1) ||
		(to.Find('>') != -1) ||
		(to.Find('|') != -1))
	{
		wxMessageBox(_("Filenames may not contain any of the following characters: / \\ : * ? \" < > |"), _("Invalid filename"), wxICON_EXCLAMATION);
		return false;
	}

	SHFILEOPSTRUCT op;
	memset(&op, 0, sizeof(op));

	from = dir + from + _T(" ");
	from.SetChar(from.Length() - 1, '\0');
	op.pFrom = from;
	to = dir + to + _T(" ");
	to.SetChar(to.Length()-1, '\0');
	op.pTo = to;
	op.hwnd = (HWND)parent->GetHandle();
	op.wFunc = FO_RENAME;
	op.fFlags = FOF_ALLOWUNDO;
	return SHFileOperation(&op) == 0;
#else
	if ((to.Find('/') != -1) ||
		(to.Find('*') != -1) ||
		(to.Find('?') != -1) ||
		(to.Find('<') != -1) ||
		(to.Find('>') != -1) ||
		(to.Find('|') != -1))
	{
		wxMessageBox(_("Filenames may not contain any of the following characters: / * ? < > |"), _("Invalid filename"), wxICON_EXCLAMATION);
		return false;
	}

	return wxRename(dir + from, dir + to) == 0;
#endif
}

void CLocalListView::OnEndLabelEdit(wxListEvent& event)
{
#ifdef __WXMAC__
	int item = event.GetIndex();
	if (item != -1)
	{
		int from = item;
		int to = item;
		if (from)
			from--;
		if (to < GetItemCount() - 1)
			to++;
		RefreshItems(from, to);
	}
#endif

	if (event.IsEditCancelled())
		return;

	const int index = event.GetIndex();
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

	if (!m_pState->GetLocalDir().IsWriteable())
	{
		event.Veto();
		return;
	}

	CLocalFileData *const data = GetData(event.GetIndex());
	if (!data || data->flags == fill)
	{
		event.Veto();
		return;
	}

	wxString newname = event.GetLabel();
#ifdef __WXMSW__
	newname = newname.Left(255);
#endif

	if (newname == data->name)
		return;

	if (!Rename(this, m_dir, data->name, newname))
		event.Veto();
	else
	{
		data->name = newname;
#ifdef __WXMSW__
		data->label = data->name;
#endif
		m_pState->RefreshLocal();
	}
}

void CLocalListView::ApplyCurrentFilter()
{
	CFilterManager filter;

	if (!filter.HasSameLocalAndRemoteFilters() && IsComparing())
		ExitComparisonMode();

	unsigned int min = m_hasParent ? 1 : 0;
	if (m_fileData.size() <= min)
		return;

	wxString focused;
	const std::list<wxString>& selectedNames = RememberSelectedItems(focused);

	if (m_pFilelistStatusBar)
		m_pFilelistStatusBar->UnselectAll();

	wxLongLong totalSize;
	int unknown_sizes = 0;
	int totalFileCount = 0;
	int totalDirCount = 0;
	int hidden = 0;

	m_indexMapping.clear();
	if (m_hasParent)
		m_indexMapping.push_back(0);
	for (unsigned int i = min; i < m_fileData.size(); i++)
	{
		const CLocalFileData& data = m_fileData[i];
		if (data.flags == fill)
			continue;
		if (filter.FilenameFiltered(data.name, m_dir, data.dir, data.size, true, data.attributes))
		{
			hidden++;
			continue;
		}

		if (data.dir)
			totalDirCount++;
		else
		{
			if (data.size != -1)
				totalSize += data.size;
			else
				unknown_sizes++;
			totalFileCount++;
		}

		m_indexMapping.push_back(i);
	}
	SetItemCount(m_indexMapping.size());

	if (m_pFilelistStatusBar)
		m_pFilelistStatusBar->SetDirectoryContents(totalFileCount, totalDirCount, totalSize, unknown_sizes, hidden);

	SortList(-1, -1, false);

	if (IsComparing())
	{
		m_originalIndexMapping.clear();
		RefreshComparison();
	}

	ReselectItems(selectedNames, focused);

	if (!IsComparing())
		RefreshListOnly();
}

std::list<wxString> CLocalListView::RememberSelectedItems(wxString& focused)
{
	std::list<wxString> selectedNames;
	// Remember which items were selected
#ifndef __WXMSW__
	// GetNextItem is O(n) if nothing is selected, GetSelectedItemCount() is O(1)
	if (GetSelectedItemCount())
#endif
	{
		int item = -1;
		while (true)
		{
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			if (item == -1)
				break;
			const CLocalFileData &data = m_fileData[m_indexMapping[item]];
			if (data.flags != fill)
			{
				if (data.dir)
					selectedNames.push_back(_T("d") + data.name);
				else
					selectedNames.push_back(_T("-") + data.name);
			}
			SetSelection(item, false);
		}
	}

	int item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
	if (item != -1)
	{
		const CLocalFileData &data = m_fileData[m_indexMapping[item]];
		if (data.flags != fill)
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
			const CLocalFileData &data = m_fileData[m_indexMapping[i]];
			if (data.name == focused)
			{
				SetItemState(i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
				return;
			}
		}
		return;
	}

	int firstSelected = -1;

	int i = -1;
	for (std::list<wxString>::const_iterator iter = selectedNames.begin(); iter != selectedNames.end(); iter++)
	{
		while (++i < (int)m_indexMapping.size())
		{
			const CLocalFileData &data = m_fileData[m_indexMapping[i]];
			if (data.name == focused)
			{
				SetItemState(i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
				focused = _T("");
			}
			if (data.dir && *iter == (_T("d") + data.name))
			{
				if (firstSelected == -1)
					firstSelected = i;
				if (m_pFilelistStatusBar)
					m_pFilelistStatusBar->SelectDirectory();
				SetSelection(i, true);
				break;
			}
			else if (*iter == (_T("-") + data.name))
			{
				if (firstSelected == -1)
					firstSelected = i;
				if (m_pFilelistStatusBar)
					m_pFilelistStatusBar->SelectFile(data.size);
				SetSelection(i, true);
				break;
			}
		}
		if (i == (int)m_indexMapping.size())
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

void CLocalListView::OnStateChange(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2)
{
	if (notification == STATECHANGE_LOCAL_DIR)
		DisplayDir(m_pState->GetLocalDir().GetPath());
	else if (notification == STATECHANGE_APPLYFILTER)
		ApplyCurrentFilter();
	else
	{
		wxASSERT(notification == STATECHANGE_LOCAL_REFRESH_FILE);
		RefreshFile(data);
	}
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

	CDragDropManager* pDragDropManager = CDragDropManager::Init();
	pDragDropManager->pDragSource = this;
	pDragDropManager->localParent = m_dir;

	item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		CLocalFileData *data = GetData(item);
		if (!data)
			continue;

		if (data->flags == fill)
			continue;

		const wxString name = m_dir + data->name;
		pDragDropManager->m_localFiles.push_back(name);
		obj.AddFile(name);
	}

	if (obj.GetFilenames().IsEmpty())
	{
		pDragDropManager->Release();
		return;
	}

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

void CLocalListView::RefreshFile(const wxString& file)
{
	CLocalFileData data;

	bool wasLink;
	enum CLocalFileSystem::local_fileType type = CLocalFileSystem::GetFileInfo(m_dir + file, wasLink, &data.size, &data.lastModified, &data.attributes);
	if (type == CLocalFileSystem::unknown)
		return;

	data.flags = normal;
	data.icon = -2;
	data.name = file;
#ifdef __WXMSW__
	data.label = file;
#endif
	data.dir = type == CLocalFileSystem::dir;
	data.hasTime = data.lastModified.IsValid();

	CFilterManager filter;
	if (filter.FilenameFiltered(data.name, m_dir, data.dir, data.size, true, data.attributes))
		return;

	// Look if file data already exists
	unsigned int i = 0;
	for (std::vector<CLocalFileData>::iterator iter = m_fileData.begin(); iter != m_fileData.end(); iter++, i++)
	{
		const CLocalFileData& oldData = *iter;
		if (oldData.name != file)
			continue;

		// Update file list status bar
		if (m_pFilelistStatusBar)
		{
#ifndef __WXMSW__
			// GetNextItem is O(n) if nothing is selected, GetSelectedItemCount() is O(1)
			if (GetSelectedItemCount())
#endif
			{
				int item = -1;
				while (true)
				{
					item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
					if (item == -1)
						break;
					if (m_indexMapping[item] != i)
						continue;

					if (oldData.dir)
						m_pFilelistStatusBar->UnselectDirectory();
					else
						m_pFilelistStatusBar->UnselectFile(oldData.size);
					if (data.dir)
						m_pFilelistStatusBar->SelectDirectory();
					else
						m_pFilelistStatusBar->SelectFile(data.size);
					break;
				}
			}

			if (oldData.dir)
				m_pFilelistStatusBar->RemoveDirectory();
			else
				m_pFilelistStatusBar->RemoveFile(oldData.size);
			if (data.dir)
				m_pFilelistStatusBar->AddDirectory();
			else
				m_pFilelistStatusBar->AddFile(data.size);
		}

		// Update the data
		data.fileType = oldData.fileType;

		*iter = data;
		if (IsComparing())
		{
			// Sort order doesn't change
			RefreshComparison();
		}
		else
		{
			if (m_sortColumn)
				SortList();
			RefreshListOnly(false);
		}
		return;
	}

	if (data.dir)
		m_pFilelistStatusBar->AddDirectory();
	else
		m_pFilelistStatusBar->AddFile(data.size);

	wxString focused;
	std::list<wxString> selectedNames;
	if (IsComparing())
	{
		wxASSERT(!m_originalIndexMapping.empty());
		selectedNames = RememberSelectedItems(focused);
		m_indexMapping.clear();
		m_originalIndexMapping.swap(m_indexMapping);
	}

	// Insert new entry
	int index = m_fileData.size();
	m_fileData.push_back(data);

	// Find correct position in index mapping
	std::vector<unsigned int>::iterator start = m_indexMapping.begin();
	if (m_hasParent)
		start++;
	CFileListCtrl<CLocalFileData>::CSortComparisonObject compare = GetSortComparisonObject();
	std::vector<unsigned int>::iterator insertPos = std::lower_bound(start, m_indexMapping.end(), index, compare);
	compare.Destroy();

	const int item = insertPos - m_indexMapping.begin();
	m_indexMapping.insert(insertPos, index);

	if (!IsComparing())
	{
		SetItemCount(m_indexMapping.size());

		// Move selections
		int prevState = 0;
		for (unsigned int i = item; i < m_indexMapping.size(); i++)
		{
			int state = GetItemState(i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
			if (state != prevState)
			{
				SetItemState(i, prevState, wxLIST_STATE_FOCUSED);
				SetSelection(i, (prevState & wxLIST_STATE_SELECTED) != 0);
				prevState = state;
			}
		}
		RefreshListOnly();
	}
	else
	{
		RefreshComparison();
		if (m_pFilelistStatusBar)
			m_pFilelistStatusBar->UnselectAll();
		ReselectItems(selectedNames, focused);
	}
}

void CLocalListView::InitDateFormat()
{
	const wxString& dateFormat = COptions::Get()->GetOption(OPTION_DATE_FORMAT);
	const wxString& timeFormat = COptions::Get()->GetOption(OPTION_TIME_FORMAT);

	if (dateFormat == _T("1"))
		m_dateFormat = _T("%Y-%m-%d");
	else if (dateFormat[0] == '2')
		m_dateFormat = dateFormat.Mid(1);
	else
		m_dateFormat = _T("%x");

	m_dateFormat += ' ';

	if (timeFormat == _T("1"))
		m_dateFormat += _T("%H:%M");
	else if (timeFormat[0] == '2')
		m_dateFormat += timeFormat.Mid(1);
	else
		m_dateFormat += _T("%X");
}

wxListItemAttr* CLocalListView::OnGetItemAttr(long item) const
{
	CLocalListView *pThis = const_cast<CLocalListView *>(this);
	const CLocalFileData* const data = pThis->GetData((unsigned int)item);

	if (!data)
		return 0;

#ifndef __WXMSW__
	if (item == m_dropTarget)
		return &pThis->m_dropHighlightAttribute;
#endif

	switch (data->flags)
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

void CLocalListView::StartComparison()
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

	const CLocalFileData& last = m_fileData[m_fileData.size() - 1];
	if (last.flags != fill)
	{
		CLocalFileData data;
		data.dir = false;
		data.icon = -1;
		data.hasTime = false;
		data.size = -1;
		data.flags = fill;
		m_fileData.push_back(data);
	}
}

bool CLocalListView::GetNextFile(wxString& name, bool& dir, wxLongLong& size, wxDateTime& date, bool &hasTime)
{
	if (++m_comparisonIndex >= (int)m_originalIndexMapping.size())
		return false;

	const unsigned int index = m_originalIndexMapping[m_comparisonIndex];
	if (index >= m_fileData.size())
		return false;

	const CLocalFileData& data = m_fileData[index];

	name = data.name;
	dir = data.dir;
	size = data.size;
	date = data.lastModified;
	hasTime = true;

	return true;
}

void CLocalListView::FinishComparison()
{
	SetItemCount(m_indexMapping.size());

	ComparisonRestoreSelections();

	RefreshListOnly();

	CComparableListing* pOther = GetOther();
	if (!pOther)
		return;

	pOther->ScrollTopItem(GetTopItem());
}

bool CLocalListView::CanStartComparison(wxString* pError)
{
	return true;
}

wxString CLocalListView::GetItemText(int item, unsigned int column)
{
	CLocalFileData *data = GetData(item);
	if (!data)
		return _T("");

	if (!column)
	{
#ifdef __WXMSW__
		return data->label;
#else
		return data->name;
#endif
	}
	else if (column == 1)
	{
		if (data->size < 0)
			return _T("");
		else
			return CSizeFormat::Format(data->size);
	}
	else if (column == 2)
	{
		if (!item && m_hasParent)
			return _T("");

		if (data->flags == fill)
			return _T("");

		if (data->fileType.IsEmpty())
			data->fileType = GetType(data->name, data->dir, m_dir);

		return data->fileType;
	}
	else if (column == 3)
	{
		if (!data->hasTime)
			return _T("");

		return data->lastModified.Format(m_dateFormat);
	}
	return _T("");
}

void CLocalListView::OnMenuEdit(wxCommandEvent& event)
{
	CServer server;
	CServerPath path;

	if (!m_pState->GetServer())
	{
		if (COptions::Get()->GetOptionVal(OPTION_EDIT_TRACK_LOCAL))
		{
			wxMessageBox(_("Cannot edit file, not connected to any server."), _("Editing failed"), wxICON_EXCLAMATION);
			return;
		}
	}
	else
	{
		server = *m_pState->GetServer();

		path = m_pState->GetRemotePath();
		if (path.IsEmpty())
		{
			wxMessageBox(_("Cannot edit file, remote path unknown."), _("Editing failed"), wxICON_EXCLAMATION);
			return;
		}
	}

	std::list<CLocalFileData> selected_item_list;

	long item = -1;
	while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
	{
		if (!item && m_hasParent)
		{
			wxBell();
			return;
		}

		const CLocalFileData *data = GetData(item);
		if (!data)
			continue;

		if (data->dir)
		{
			wxBell();
			return;
		}

		if (data->flags == fill)
			continue;

		selected_item_list.push_back(*data);
	}

	CEditHandler* pEditHandler = CEditHandler::Get();

	if (selected_item_list.empty())
	{
		wxBell();
		return;
	}

	if (selected_item_list.size() > 10)
	{

		CConditionalDialog dlg(this, CConditionalDialog::many_selected_for_edit, CConditionalDialog::yesno);
		dlg.SetTitle(_("Confirmation needed"));
		dlg.AddText(_("You have selected more than 10 files for editing, do you really want to continue?"));

		if (!dlg.Run())
			return;
	}

	for (std::list<CLocalFileData>::const_iterator data = selected_item_list.begin(); data != selected_item_list.end(); data++)
	{
		wxFileName fn(m_dir, data->name);

		bool dangerous = false;
		bool program_exists = false;
		wxString cmd = pEditHandler->CanOpen(CEditHandler::local, fn.GetFullPath(), dangerous, program_exists);
		if (cmd.empty())
		{
			CNewAssociationDialog dlg(this);
			if (!dlg.Show(fn.GetFullName()))
				continue;
			cmd = pEditHandler->CanOpen(CEditHandler::local, fn.GetFullPath(), dangerous, program_exists);
			if (cmd.empty())
			{
				wxMessageBox(wxString::Format(_("The file '%s' could not be opened:\nNo program has been associated on your system with this file type."), fn.GetFullPath().c_str()), _("Opening failed"), wxICON_EXCLAMATION);
				continue;
			}
		}
		if (!program_exists)
		{
			wxString msg = wxString::Format(_("The file '%s' cannot be opened:\nThe associated program (%s) could not be found.\nPlease check your filetype associations."), fn.GetFullPath().c_str(), cmd.c_str());
			wxMessageBox(msg, _("Cannot edit file"), wxICON_EXCLAMATION);
			continue;
		}
		if (dangerous)
		{
			int res = wxMessageBox(_("The selected file would be executed directly.\nThis can be dangerous and damage your system.\nDo you really want to continue?"), _("Dangerous filetype"), wxICON_EXCLAMATION | wxYES_NO);
			if (res != wxYES)
			{
				wxBell();
				continue;
			}
		}

		CEditHandler::fileState state = pEditHandler->GetFileState(fn.GetFullPath());
		switch (state)
		{
		case CEditHandler::upload:
		case CEditHandler::upload_and_remove:
			wxMessageBox(_("A file with that name is already being transferred."), _("Cannot view/edit selected file"), wxICON_EXCLAMATION);
			continue;
		case CEditHandler::edit:
			{
				int res = wxMessageBox(wxString::Format(_("A file with that name is already being edited. Do you want to reopen '%s'?"), fn.GetFullPath().c_str()), _("Selected file already being edited"), wxICON_QUESTION | wxYES_NO);
				if (res != wxYES)
				{
					wxBell();
					continue;
				}
				pEditHandler->StartEditing(fn.GetFullPath());
				continue;
			}
		default:
			break;
		}

		wxString file = fn.GetFullPath();
		if (!pEditHandler->AddFile(CEditHandler::local, file, path, server))
		{
			wxMessageBox(wxString::Format(_("The file '%s' could not be opened:\nThe associated command failed"), fn.GetFullPath().c_str()), _("Opening failed"), wxICON_EXCLAMATION);
			continue;
		}
	}
}

void CLocalListView::OnMenuOpen(wxCommandEvent& event)
{
	long item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item == -1)
	{
		wxLaunchDefaultBrowser(CState::GetAsURL(m_dir));
		return;
	}

	std::list<CLocalFileData> selected_item_list;

	item = -1;
	while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
	{
		if (!item && m_hasParent)
		{
			wxBell();
			return;
		}

		const CLocalFileData *data = GetData(item);
		if (!data)
			continue;

		if (data->flags == fill)
			continue;

		selected_item_list.push_back(*data);
	}

	CEditHandler* pEditHandler = CEditHandler::Get();
	if (!pEditHandler)
	{
		wxBell();
		return;
	}

	if (selected_item_list.empty())
	{
		wxBell();
		return;
	}

	if (selected_item_list.size() > 10)
	{

		CConditionalDialog dlg(this, CConditionalDialog::many_selected_for_edit, CConditionalDialog::yesno);
		dlg.SetTitle(_("Confirmation needed"));
		dlg.AddText(_("You have selected more than 10 files or directories to open, do you really want to continue?"));

		if (!dlg.Run())
			return;
	}

	for (std::list<CLocalFileData>::const_iterator data = selected_item_list.begin(); data != selected_item_list.end(); data++)
	{
		if (data->dir)
		{
			CLocalPath path(m_dir);
			if (!path.ChangePath(data->name))
			{
				wxBell();
				continue;
			}

			wxLaunchDefaultBrowser(CState::GetAsURL(path.GetPath()));
			continue;
		}

		wxFileName fn(m_dir, data->name);

		bool program_exists = false;
		wxString cmd = pEditHandler->GetSystemOpenCommand(fn.GetFullPath(), program_exists);
		if (cmd == _T(""))
		{
			int pos = data->name.Find('.') == -1;
			if (pos == -1 || (pos == 0 && data->name.Mid(1).Find('.') == -1))
				cmd = pEditHandler->GetOpenCommand(fn.GetFullPath(), program_exists);
		}
		if (cmd == _T(""))
		{
			wxMessageBox(wxString::Format(_("The file '%s' could not be opened:\nNo program has been associated on your system with this file type."), fn.GetFullPath().c_str()), _("Opening failed"), wxICON_EXCLAMATION);
			continue;
		}
		if (!program_exists)
		{
			wxString msg = wxString::Format(_("The file '%s' cannot be opened:\nThe associated program (%s) could not be found.\nPlease check your filetype associations."), fn.GetFullPath().c_str(), cmd.c_str());
			wxMessageBox(msg, _("Cannot edit file"), wxICON_EXCLAMATION);
			continue;
		}

		if (wxExecute(cmd))
			continue;

		wxMessageBox(wxString::Format(_("The file '%s' could not be opened:\nThe associated command failed"), fn.GetFullPath().c_str()), _("Opening failed"), wxICON_EXCLAMATION);
	}
}

bool CLocalListView::ItemIsDir(int index) const
{
	return m_fileData[index].dir;
}

wxLongLong CLocalListView::ItemGetSize(int index) const
{
	return m_fileData[index].size;
}

#ifdef __WXMSW__

void CLocalListView::OnVolumesEnumerated(wxCommandEvent& event)
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

	if (m_dir != _T("\\"))
		return;

	for (std::list<CVolumeDescriptionEnumeratorThread::t_VolumeInfo>::const_iterator iter = volumeInfo.begin(); iter != volumeInfo.end(); iter++)
	{
		wxString drive = iter->volume;

		unsigned int item, index;
		for (item = 1; item < m_indexMapping.size(); item++)
		{
			index = m_indexMapping[item];
			if (m_fileData[index].name == drive || m_fileData[index].name.Left(drive.Len() + 1) == drive + _T(" "))
				break;
		}
		if (item == m_indexMapping.size())
			continue;

		m_fileData[index].label = drive + _T(" (") + iter->volumeName + _T(")");
		
		RefreshItem(item);
	}
}

#endif

void CLocalListView::OnMenuRefresh(wxCommandEvent& event)
{
	m_pState->RefreshLocal();
}
