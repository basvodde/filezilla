#define FILELISTCTRL_INCLUDE_TEMPLATE_DEFINITION

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
#else
#include <langinfo.h>
#endif
#include "edithandler.h"
#include "dragdropmanager.h"
#include "local_filesys.h"

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

		wxString subdir;
		int flags;
		int hit = m_pLocalListView->HitTest(wxPoint(x, y), flags, 0);
		if (hit != -1 && (flags & wxLIST_HITTEST_ONITEM))
		{
			const CLocalFileData* const data = m_pLocalListView->GetData(hit);
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

		if (subdir == _T(""))
		{
			const CDragDropManager* pDragDropManager = CDragDropManager::Get();
			if (pDragDropManager && pDragDropManager->localParent == m_pLocalListView->m_dir)
				return wxDragNone;

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
END_EVENT_TABLE()

CLocalListView::CLocalListView(wxWindow* pParent, CState *pState, CQueueView *pQueue)
	: CFileListCtrl<CLocalFileData>(pParent, pState, pQueue),
	CSystemImageList(16), CStateEventHandler(pState)
{
	m_pState->RegisterHandler(this, STATECHANGE_LOCAL_DIR);
	m_pState->RegisterHandler(this, STATECHANGE_APPLYFILTER);
	m_pState->RegisterHandler(this, STATECHANGE_LOCAL_REFRESH_FILE);

	m_dropTarget = -1;

	const unsigned long widths[4] = { 120, 80, 100, 120 };

	AddColumn(_("Filename"), wxLIST_FORMAT_LEFT, widths[0]);
	AddColumn(_("Filesize"), wxLIST_FORMAT_RIGHT, widths[1]);
	AddColumn(_("Filetype"), wxLIST_FORMAT_LEFT, widths[2]);
	AddColumn(_("Last modified"), wxLIST_FORMAT_LEFT, widths[3]);
	LoadColumnSettings(OPTION_LOCALFILELIST_COLUMN_WIDTHS, OPTION_LOCALFILELIST_COLUMN_SHOWN, OPTION_LOCALFILELIST_COLUMN_ORDER);

	InitSort(OPTION_LOCALFILELIST_SORTORDER);

	SetImageList(GetSystemImageList(), wxIMAGE_LIST_SMALL);

#ifdef __WXMSW__
	InitHeaderImageList();
#endif

	SetDropTarget(new CLocalListViewDropTarget(this));

	InitDateFormat();

	EnablePrefixSearch(true);
}

CLocalListView::~CLocalListView()
{
	wxString str = wxString::Format(_T("%d %d"), m_sortDirection, m_sortColumn);
	COptions::Get()->SetOption(OPTION_LOCALFILELIST_SORTORDER, str);
}

bool CLocalListView::DisplayDir(wxString dirname)
{
	wxString focused;
	std::list<wxString> selectedNames;
	if (m_dir != dirname)
	{
		ResetSearchPrefix();

		if (IsComparing())
			ExitComparisonMode();

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
		CLocalFileData data;
		data.flags = normal;
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
		CFilterManager filter;
		CLocalFileSystem local_filesystem;

		if (!local_filesystem.BeginFindFiles(dirname, false))
		{
			SetItemCount(1);
			return false;
		}

		int num = m_fileData.size();
		CLocalFileData data;
		data.flags = normal;
		data.icon = -2;
		bool wasLink;
		while (local_filesystem.GetNextFile(data.name, wasLink, data.dir, &data.size, &data.lastModified, &data.attributes))
		{
			if (data.name == _T(""))
			{
				wxGetApp().DisplayEncodingWarning();
				continue;
			}
			data.hasTime = data.lastModified.IsValid();

			m_fileData.push_back(data);
			if (!filter.FilenameFiltered(data.name, data.dir, data.size, true, data.attributes))
				m_indexMapping.push_back(num);
			num++;
		}
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

	Refresh();

	return true;
}

wxString FormatSize(const wxLongLong& size)
{
	COptions* const pOptions = COptions::Get();
	const int format = pOptions->GetOptionVal(OPTION_SIZE_FORMAT);

	if (!format)
	{
		if (!pOptions->GetOptionVal(OPTION_SIZE_USETHOUSANDSEP))
			return size.ToString();

#ifdef __WXMSW__
		wxChar sep[5];
		int count = ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, sep, 5);
		if (!count)
			return size.ToString();
#else
		char* chr = nl_langinfo(THOUSEP);
		if (!chr || !*chr)
			return size.ToString();
		wxChar sep = chr[0];
#endif

		wxString tmp = size.ToString();
		const int len = tmp.Len();
		if (len <= 3)
			return tmp;

		wxString result;
		int i = (len - 1) % 3 + 1;
		result = tmp.Left(i);
		while (i < len)
		{
			result += sep + tmp.Mid(i, 3);
			i += 3;
		}

		return result;
	}

	int divider;
	if (format == 3)
		divider = 1000;
	else
		divider = 1024;

	// We always round up. Set to true if there's a reminder
	bool r2 = false;

	// Exponent (2^(10p) or 10^(3p) depending on option
	int p = 0;

	wxLongLong r = size;
	while (r > divider && p < 6)
	{
		const wxLongLong rr = r / divider;
		if (rr * divider != r)
			r2 = true;
		r = rr;
		p++;
	}
	if (r2)
		r++;

	wxString result = r.ToString();
	result += ' ';
	if (!p)
		return result + _T("B");

	// We stop at Exa. If someone has files bigger than that, he can afford to
	// make a donation to have this changed ;)
	wxChar prefix[] = { ' ', 'K', 'M', 'G', 'T', 'P', 'E' };

	result += prefix[p];
	if (format == 1)
		result += 'i';
	result += 'B';

	return result;
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
		if (IsComparing())
			ExitComparisonMode();

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

	if (data->flags == fill)
	{
		wxBell();
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

		CLocalFileData data;
		data.flags = normal;
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
			data.dir = true;
			data.icon = -2;
			data.size = -1;
			data.hasTime = false;

			m_fileData.push_back(data);
			m_indexMapping.push_back(j++);
		}

		NetApiBufferFree(si.pShareInfo);
	}
	while (res == ERROR_MORE_DATA);
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

		if (data1.fileType == _T(""))
			data1.fileType = m_pListView->GetType(data1.name, data1.dir, m_pListView->m_dir);
		if (data2.fileType == _T(""))
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
		pMenu->Enable(XRCID("ID_EDIT"), false);
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
		pMenu->Enable(XRCID("ID_UPLOAD"), false);
		pMenu->Enable(XRCID("ID_ADDTOQUEUE"), false);
		pMenu->Enable(XRCID("ID_DELETE"), false);
		pMenu->Enable(XRCID("ID_RENAME"), false);
		pMenu->Enable(XRCID("ID_OPEN"), false);
		pMenu->Enable(XRCID("ID_EDIT"), false);
	}
	else if (count > 1)
	{
		pMenu->Enable(XRCID("ID_RENAME"), false);
		pMenu->Enable(XRCID("ID_OPEN"), false);
		pMenu->Enable(XRCID("ID_EDIT"), false);
	}
	else if (selectedDir)
	{
		pMenu->Enable(XRCID("ID_OPEN"), false);
		pMenu->Enable(XRCID("ID_EDIT"), false);
	}

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

		CLocalFileData *data = GetData(item);
		if (!data)
			return;

		if (data->flags == fill)
			continue;

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

void CLocalListView::OnChar(wxKeyEvent& event)
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

		if (IsComparing())
			ExitComparisonMode();

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

void CLocalListView::OnKeyDown(wxKeyEvent& event)
{
	const int code = event.GetKeyCode();
	const int mods = event.GetModifiers();
	if (code == 'A' && (mods == wxMOD_CMD || mods == (wxMOD_CONTROL | wxMOD_META)))
	{
		for (unsigned int i = m_hasParent ? 1 : 0; i < m_indexMapping.size(); i++)
		{
			CLocalFileData *data = GetData(i);
			if (data && data->flags != fill)
				SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
			else
				SetItemState(i, 0, wxLIST_STATE_SELECTED);
		}
	}
	else
		OnChar(event);
}

void CLocalListView::OnBeginLabelEdit(wxListEvent& event)
{
	if (!m_pState->LocalDirIsWriteable(m_dir))
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

	if (!m_pState->LocalDirIsWriteable(m_dir))
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

	m_indexMapping.clear();
	if (m_hasParent)
		m_indexMapping.push_back(0);
	for (unsigned int i = min; i < m_fileData.size(); i++)
	{
		const CLocalFileData& data = m_fileData[i];
		if (data.flags == fill)
			continue;
		if (!filter.FilenameFiltered(data.name, data.dir, data.size, true, data.attributes))
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
		const CLocalFileData &data = m_fileData[m_indexMapping[item]];
		if (data.flags != fill)
		{
			if (data.dir)
				selectedNames.push_back(_T("d") + data.name);
			else
				selectedNames.push_back(_T("-") + data.name);
		}
		SetItemState(item, 0, wxLIST_STATE_SELECTED);
	}

	item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
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
				SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
				break;
			}
			else if (*iter == (_T("-") + data.name))
			{
				if (firstSelected == -1)
					firstSelected = i;
				SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
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

void CLocalListView::OnStateChange(enum t_statechange_notifications notification, const wxString& data)
{
	if (notification == STATECHANGE_LOCAL_DIR)
		DisplayDir(m_pState->GetLocalDir());
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

	pDragDropManager->Release();

	if (res == wxDragCopy || res == wxDragMove)
		m_pState->RefreshLocal();
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
	data.dir = type == CLocalFileSystem::dir;
	data.hasTime = data.lastModified.IsValid();

	// Look if file data already exists
	for (std::vector<CLocalFileData>::iterator iter = m_fileData.begin(); iter != m_fileData.end(); iter++)
	{
		const CLocalFileData& oldData = *iter;
		if (oldData.name != file)
			continue;

		// It exists, update entry
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
			Refresh(false);
		}
		return;
	}

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
				SetItemState(i, prevState, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
				prevState = state;
			}
		}
		Refresh();
	}
	else
	{
		RefreshComparison();
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

	Refresh();

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
		return data->name;
	else if (column == 1)
	{
		if (data->size < 0)
			return _T("");
		else
			return FormatSize(data->size);
	}
	else if (column == 2)
	{
		if (!item && m_hasParent)
			return _T("");

		if (data->flags == fill)
			return _T("");

		if (data->fileType == _T(""))
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
	const CServer* const pServer = m_pState->GetServer();
	if (!pServer)
	{
		wxBell();
		return;
	}

	const CServerPath& path = m_pState->GetRemotePath();
	if (path.IsEmpty())
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

	if (!item && m_hasParent)
	{
		wxBell();
		return;
	}

	CEditHandler* pEditHandler = CEditHandler::Get();

	CLocalFileData *data = GetData(item);
	if (!data)
	{
		wxBell();
		return;
	}

	if (data->dir || data->flags == fill)
	{
		wxBell();
		return;
	}

	wxFileName fn(m_dir, data->name);

	bool dangerous = false;
	if (!pEditHandler->CanOpen(CEditHandler::local, fn.GetFullPath(), dangerous) || dangerous)
	{
		wxMessageBox(wxString::Format(_("The file '%s' could not be opened:\nNo program has been associated on your system with this file type."), fn.GetFullPath().c_str()), _("Opening failed"), wxICON_EXCLAMATION);
		return;
	}

	CEditHandler::fileState state = pEditHandler->GetFileState(CEditHandler::local, fn.GetFullPath());
	switch (state)
	{
	case CEditHandler::upload:
	case CEditHandler::upload_and_remove:
		wxMessageBox(_("A file with that name is already being transferred."), _("Cannot view / edit selected file"), wxICON_EXCLAMATION);
		return;
	case CEditHandler::edit:
		{
			int res = wxMessageBox(_("A file with that name is already being edited. Discard old file and download the new file?"), _("Selected file already being edited"), wxICON_QUESTION | wxYES_NO);
			if (res != wxYES)
			{
				wxBell();
				return;
			}
			pEditHandler->StartEditing(CEditHandler::local, fn.GetFullPath());
			return;
		}
	default:
		break;
	}

	if (!pEditHandler->AddFile(CEditHandler::local, fn.GetFullPath(), path, *pServer))
	{
		wxMessageBox(wxString::Format(_("The file '%s' could not be opened:\nThe associated command failed"), fn.GetFullPath().c_str()), _("Opening failed"), wxICON_EXCLAMATION);
		return;
	}
}

void CLocalListView::OnMenuOpen(wxCommandEvent& event)
{
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

	if (!item && m_hasParent)
	{
		wxBell();
		return;
	}

	CEditHandler* pEditHandler = CEditHandler::Get();

	CLocalFileData *data = GetData(item);
	if (!data)
	{
		wxBell();
		return;
	}

	if (data->dir || data->flags == fill)
	{
		wxBell();
		return;
	}

	wxFileName fn(m_dir, data->name);

	wxString cmd = pEditHandler->GetSystemOpenCommand(fn.GetFullPath());
	if (cmd == _T(""))
	{
		int pos = data->name.Find('.') == -1;
		if (pos == -1 || (pos == 0 && data->name.Mid(1).Find('.') == -1))
			cmd = pEditHandler->GetOpenCommand(fn.GetFullPath());
	}
	if (cmd == _T(""))
	{
		wxMessageBox(wxString::Format(_("The file '%s' could not be opened:\nNo program has been associated on your system with this file type."), fn.GetFullPath().c_str()), _("Opening failed"), wxICON_EXCLAMATION);
		return;
	}

	if (wxExecute(cmd))
		return;

	wxMessageBox(wxString::Format(_("The file '%s' could not be opened:\nThe associated command failed"), fn.GetFullPath().c_str()), _("Opening failed"), wxICON_EXCLAMATION);
}
