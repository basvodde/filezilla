#define FILELISTCTRL_INCLUDE_TEMPLATE_DEFINITION

#include "FileZilla.h"
#include "search.h"
#include "filelistctrl.h"
#include "recursive_operation.h"
#include "commandqueue.h"
#include "Options.h"
#include "window_state_manager.h"
#include "queue.h"

class CSearchFileData : public CGenericFileData
{
public:
	CServerPath path;

	CDirentry entry;
};

class CSearchDialogFileList : public CFileListCtrl<CSearchFileData>, public CSystemImageList
{
	friend class CSearchDialog;
	friend class CSearchSortType;
public:
	CSearchDialogFileList(CSearchDialog* pParent, CState* pState, CQueueView* pQueue);

protected:
	virtual bool ItemIsDir(int index) const;

	virtual wxLongLong ItemGetSize(int index) const;

	CFileListCtrl<CSearchFileData>::CSortComparisonObject GetSortComparisonObject();

	CSearchDialog *m_searchDialog;

	virtual wxString GetItemText(int item, unsigned int column);
	virtual int OnGetItemImage(long item) const;

	void InitDateFormat();

private:
	virtual bool CanStartComparison(wxString* pError) { return false; }
	virtual void StartComparison() {}
	virtual bool GetNextFile(wxString& name, bool &dir, wxLongLong &size, wxDateTime& date, bool &hasTime) { return false; }
	virtual void CompareAddFile(CComparableListing::t_fileEntryFlags flags) {}
	virtual void FinishComparison() {}
	virtual void ScrollTopItem(int item) {}
	virtual void OnExitComparisonMode() {}

	int m_dirIcon;

	wxString m_timeFormat;
	wxString m_dateFormat;
};

// Helper classes for fast sorting using std::sort
// -----------------------------------------------

class CSearchDialogFileList;
class CSearchSort : public CListViewSort
{
public:
	CSearchSort(CSearchDialogFileList* pListCtrl, std::vector<CSearchFileData> &fileData, enum DirSortMode dirSortMode)
		: m_pListCtrl(pListCtrl), m_fileData(fileData), m_dirSortMode(dirSortMode)
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
		if (data1.hasTimestamp == CDirentry::timestamp_none)
		{
			if (data2.hasTimestamp != CDirentry::timestamp_none)
				return -1;
			else
				return 0;
		}
		else
		{
			if (data2.hasTimestamp == CDirentry::timestamp_none)
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
	std::vector<CSearchFileData> &m_fileData;

	const enum DirSortMode m_dirSortMode;

	CSearchDialogFileList* m_pListCtrl;
};

template<class T> class CReverseSort : public T
{
public:
	CReverseSort(CSearchDialogFileList* pListCtrl, std::vector<CSearchFileData>& fileData, enum CSearchSort::DirSortMode dirSortMode)
		: T(pListCtrl, fileData, dirSortMode)
	{
	}

	inline bool operator()(int a, int b) const
	{
		return T::operator()(b, a);
	}
};

class CSearchSortName : public CSearchSort
{
public:
	CSearchSortName(CSearchDialogFileList* pListCtrl, std::vector<CSearchFileData>& fileData, enum CSearchSort::DirSortMode dirSortMode)
		: CSearchSort(pListCtrl, fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = m_fileData[a].entry;
		const CDirentry& data2 = m_fileData[b].entry;

		CMP(CmpDir, data1, data2);

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CSearchSortName> CSearchSortName_Reverse;

class CSearchSortPath : public CSearchSort
{
public:
	CSearchSortPath(CSearchDialogFileList* pListCtrl, std::vector<CSearchFileData>& fileData, enum CSearchSort::DirSortMode dirSortMode)
		: CSearchSort(pListCtrl, fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = m_fileData[a].entry;
		const CDirentry& data2 = m_fileData[b].entry;

		if (m_fileData[a].path < m_fileData[b].path)
			return true;
		if (m_fileData[a].path != m_fileData[b].path)
			return false;

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CSearchSortPath> CSearchSortPath_Reverse;

class CSearchSortSize : public CSearchSort
{
public:
	CSearchSortSize(CSearchDialogFileList* pListCtrl, std::vector<CSearchFileData>& fileData, enum CSearchSort::DirSortMode dirSortMode)
		: CSearchSort(pListCtrl, fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = m_fileData[a].entry;
		const CDirentry& data2 = m_fileData[b].entry;

		CMP(CmpDir, data1, data2);

		CMP(CmpSize, data1, data2);

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CSearchSortSize> CSearchSortSize_Reverse;

class CSearchSortType : public CSearchSort
{
public:
	CSearchSortType(CSearchDialogFileList* pListCtrl, std::vector<CSearchFileData>& fileData, enum CSearchSort::DirSortMode dirSortMode)
		: CSearchSort(pListCtrl, fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		CSearchFileData &data1 = m_fileData[a];
		CSearchFileData &data2 = m_fileData[b];

		CMP(CmpDir, data1.entry, data2.entry);

		if (data1.fileType.IsEmpty())
			data1.fileType = m_pListCtrl->GetType(data1.entry.name, data1.entry.dir);
		if (data2.fileType.IsEmpty())
			data2.fileType = m_pListCtrl->GetType(data2.entry.name, data2.entry.dir);

		CMP(CmpStringNoCase, data1.fileType, data2.fileType);

		CMP_LESS(CmpName, data1.entry, data2.entry);
	}
};
typedef CReverseSort<CSearchSortType> CSearchSortType_Reverse;

class CSearchSortTime : public CSearchSort
{
public:
	CSearchSortTime(CSearchDialogFileList* pListCtrl, std::vector<CSearchFileData>& fileData, enum CSearchSort::DirSortMode dirSortMode)
		: CSearchSort(pListCtrl, fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = m_fileData[a].entry;
		const CDirentry& data2 = m_fileData[b].entry;

		CMP(CmpDir, data1, data2);

		CMP(CmpTime, data1, data2);

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CSearchSortTime> CSearchSortTime_Reverse;

class CSearchSortPermissions : public CSearchSort
{
public:
	CSearchSortPermissions(CSearchDialogFileList* pListCtrl, std::vector<CSearchFileData>& fileData, enum CSearchSort::DirSortMode dirSortMode)
		: CSearchSort(pListCtrl, fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = m_fileData[a].entry;
		const CDirentry& data2 = m_fileData[b].entry;

		CMP(CmpDir, data1, data2);

		CMP(CmpStringNoCase, data1.permissions, data2.permissions);

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CSearchSortPermissions> CSearchSortPermissions_Reverse;

class CSearchSortOwnerGroup : public CSearchSort
{
public:
	CSearchSortOwnerGroup(CSearchDialogFileList* pListCtrl, std::vector<CSearchFileData>& fileData, enum CSearchSort::DirSortMode dirSortMode)
		: CSearchSort(pListCtrl, fileData, dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		const CDirentry& data1 = m_fileData[a].entry;
		const CDirentry& data2 = m_fileData[b].entry;

		CMP(CmpDir, data1, data2);

		CMP(CmpStringNoCase, data1.ownerGroup, data2.ownerGroup);

		CMP_LESS(CmpName, data1, data2);
	}
};
typedef CReverseSort<CSearchSortOwnerGroup> CSearchSortOwnerGroup_Reverse;

// Search dialog file list
// -----------------------

// Defined in LocalListView.cpp
extern wxString FormatSize(const wxLongLong& size, bool add_bytes_suffix = false);

// Defined in RemoteListView.cpp
extern wxString StripVMSRevision(const wxString& name);

CSearchDialogFileList::CSearchDialogFileList(CSearchDialog* pParent, CState* pState, CQueueView* pQueue)
	: CFileListCtrl<CSearchFileData>(pParent, pState, pQueue),
	CSystemImageList(16), m_searchDialog(pParent)
{
	m_hasParent = false;

	SetImageList(GetSystemImageList(), wxIMAGE_LIST_SMALL);

	m_dirIcon = GetIconIndex(dir);

	InitDateFormat();

	InitSort(OPTION_SEARCH_SORTORDER);

#ifdef __WXMSW__
	InitHeaderImageList();
#endif
	const unsigned long widths[7] = { 130, 130, 75, 80, 120, 80, 80 };

	AddColumn(_("Filename"), wxLIST_FORMAT_LEFT, widths[0]);
	AddColumn(_("Path"), wxLIST_FORMAT_LEFT, widths[1]);
	AddColumn(_("Filesize"), wxLIST_FORMAT_RIGHT, widths[2]);
	AddColumn(_("Filetype"), wxLIST_FORMAT_LEFT, widths[3]);
	AddColumn(_("Last modified"), wxLIST_FORMAT_LEFT, widths[4]);
	AddColumn(_("Permissions"), wxLIST_FORMAT_LEFT, widths[5]);
	AddColumn(_("Owner/Group"), wxLIST_FORMAT_LEFT, widths[6]);
	LoadColumnSettings(OPTION_SEARCH_COLUMN_WIDTHS, OPTION_SEARCH_COLUMN_SHOWN, OPTION_SEARCH_COLUMN_ORDER);
}

bool CSearchDialogFileList::ItemIsDir(int index) const
{
	return m_fileData[index].entry.dir;
}

wxLongLong CSearchDialogFileList::ItemGetSize(int index) const
{
	return m_fileData[index].entry.size;
}

CFileListCtrl<CSearchFileData>::CSortComparisonObject CSearchDialogFileList::GetSortComparisonObject()
{
	CSearchSort::DirSortMode dirSortMode = GetDirSortMode();

	if (!m_sortDirection)
	{
		if (m_sortColumn == 1)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortPath(this, m_fileData, dirSortMode));
		else if (m_sortColumn == 2)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortSize(this, m_fileData, dirSortMode));
		else if (m_sortColumn == 3)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortType(this, m_fileData, dirSortMode));
		else if (m_sortColumn == 4)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortTime(this, m_fileData, dirSortMode));
		else if (m_sortColumn == 5)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortPermissions(this, m_fileData, dirSortMode));
		else if (m_sortColumn == 6)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortOwnerGroup(this, m_fileData, dirSortMode));
		else
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortName(this, m_fileData, dirSortMode));
	}
	else
	{
		if (m_sortColumn == 1)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortPath_Reverse(this, m_fileData, dirSortMode));
		else if (m_sortColumn == 2)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortSize_Reverse(this, m_fileData, dirSortMode));
		else if (m_sortColumn == 3)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortType_Reverse(this, m_fileData, dirSortMode));
		else if (m_sortColumn == 4)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortTime_Reverse(this, m_fileData, dirSortMode));
		else if (m_sortColumn == 5)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortPermissions_Reverse(this, m_fileData, dirSortMode));
		else if (m_sortColumn == 6)
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortOwnerGroup_Reverse(this, m_fileData, dirSortMode));
		else
			return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSortName_Reverse(this, m_fileData, dirSortMode));
	}
}

wxString CSearchDialogFileList::GetItemText(int item, unsigned int column)
{
	if (item < 0 || item >= (int)m_indexMapping.size())
		return _T("");
	int index = m_indexMapping[item];

	const CDirentry& entry = m_fileData[index].entry;
	if (!column)
		return entry.name;
	else if (column == 1)
		return m_fileData[index].path.GetPath();
	else if (column == 2)
	{
		if (entry.dir || entry.size < 0)
			return _T("");
		else
			return FormatSize(entry.size);
	}
	else if (column == 3)
	{
		CSearchFileData& data = m_fileData[index];
		if (data.fileType.IsEmpty())
		{
			if (data.path.GetType() == VMS)
				data.fileType = GetType(StripVMSRevision(entry.name), entry.dir);
			else
				data.fileType = GetType(entry.name, entry.dir);
		}

		return data.fileType;
	}
	else if (column == 4)
	{
		if (entry.hasTimestamp == CDirentry::timestamp_none)
			return _T("");

		if (entry.hasTimestamp >= CDirentry::timestamp_time)
			return entry.time.Format(m_timeFormat);
		else
			return entry.time.Format(m_dateFormat);
	}
	else if (column == 5)
		return entry.permissions;
	else if (column == 6)
		return entry.ownerGroup;
	return _T("");
}

int CSearchDialogFileList::OnGetItemImage(long item) const
{
	CSearchDialogFileList *pThis = const_cast<CSearchDialogFileList *>(this);
	if (item < 0 || item >= (int)m_indexMapping.size())
		return -1;
	int index = m_indexMapping[item];

	int &icon = pThis->m_fileData[index].icon;

	if (icon != -2)
		return icon;

	icon = pThis->GetIconIndex(file, pThis->m_fileData[index].entry.name, false);
	return icon;
}

void CSearchDialogFileList::InitDateFormat()
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

// Search dialog
// -------------

BEGIN_EVENT_TABLE(CSearchDialog, CFilterConditionsDialog)
EVT_BUTTON(XRCID("ID_START"), CSearchDialog::OnSearch)
EVT_BUTTON(XRCID("ID_STOP"), CSearchDialog::OnStop)
EVT_CONTEXT_MENU(CSearchDialog::OnContextMenu)
EVT_MENU(XRCID("ID_MENU_SEARCH_DOWNLOAD"), CSearchDialog::OnDownload)
EVT_MENU(XRCID("ID_MENU_SEARCH_DELETE"), CSearchDialog::OnDelete)
END_EVENT_TABLE()

CSearchDialog::CSearchDialog(wxWindow* parent, CState* pState, CQueueView* pQueue)
	: CStateEventHandler(pState)
{
	m_pQueue = pQueue;
	m_parent = parent;
	m_pWindowStateManager = 0;
	m_searching = false;
}

CSearchDialog::~CSearchDialog()
{
	if (m_pWindowStateManager)
	{
		m_pWindowStateManager->Remember(OPTION_SEARCH_SIZE);
		delete m_pWindowStateManager;
	}
}

bool CSearchDialog::Load()
{
	if (!wxDialogEx::Load(m_parent, _T("ID_SEARCH")))
		return false;

	if (!CreateListControl(filter_name | filter_size | filter_path))
		return false;

	CFilter filter;
	CFilterCondition cond;
	cond.condition = 0;
	cond.type = filter_name;
	filter.filters.push_back(cond);
	Layout();
	EditFilter(filter);
	XRCCTRL(*this, "ID_CASE", wxCheckBox)->SetValue(filter.matchCase);

	m_results = new CSearchDialogFileList(this, m_pState, 0);
	ReplaceControl(XRCCTRL(*this, "ID_RESULTS", wxWindow), m_results);

	const CServerPath path = m_pState->GetRemotePath();
	if (!path.IsEmpty())
		XRCCTRL(*this, "ID_PATH", wxTextCtrl)->ChangeValue(path.GetPath());

	SetCtrlState();

	m_pWindowStateManager = new CWindowStateManager(this);
	m_pWindowStateManager->Restore(OPTION_SEARCH_SIZE, wxSize(750, 500));

	return true;
}

void CSearchDialog::Run()
{
	m_original_dir = m_pState->GetRemotePath();
	m_local_target = m_pState->GetLocalDir();

	m_pState->BlockHandlers(STATECHANGE_REMOTE_DIR);
	m_pState->BlockHandlers(STATECHANGE_REMOTE_DIR_MODIFIED);
	m_pState->RegisterHandler(this, STATECHANGE_REMOTE_DIR);
	m_pState->RegisterHandler(this, STATECHANGE_REMOTE_IDLE);

	ShowModal();

	m_pState->UnregisterHandler(this, STATECHANGE_REMOTE_IDLE);
	m_pState->UnregisterHandler(this, STATECHANGE_REMOTE_DIR);
	m_pState->UnblockHandlers(STATECHANGE_REMOTE_DIR);
	m_pState->UnblockHandlers(STATECHANGE_REMOTE_DIR_MODIFIED);

	if (m_searching)
	{
		if (!m_pState->IsRemoteIdle())
		{
			m_pState->m_pCommandQueue->Cancel();
			m_pState->GetRecursiveOperationHandler()->StopRecursiveOperation();
		}
		if (!m_original_dir.IsEmpty())
			m_pState->ChangeRemoteDir(m_original_dir);
	}
	else
	{
		if (m_pState->IsRemoteIdle() && !m_original_dir.IsEmpty())
			m_pState->ChangeRemoteDir(m_original_dir);
	}

}

void CSearchDialog::OnStateChange(enum t_statechange_notifications notification, const wxString& data)
{
	if (notification == STATECHANGE_REMOTE_DIR)
		ProcessDirectoryListing();
	else if (notification == STATECHANGE_REMOTE_IDLE)
	{
		if (m_pState->IsRemoteIdle())
			m_searching = false;
		SetCtrlState();
	}
}

void CSearchDialog::ProcessDirectoryListing()
{
	CSharedPointer<const CDirectoryListing> listing = m_pState->GetRemoteDir();

	if (!listing || listing->m_failed)
		return;

	// Do not process same directory multiple times
	if (!m_visited.insert(listing->path).second)
		return;
	
	int old_count = m_results->m_fileData.size();
	int added = 0;
	for (unsigned int i = 0; i < listing->GetCount(); i++)
	{
		const CDirentry& entry = (*listing)[i];

		if (m_search_filter.filters.size() && !CFilterManager::FilenameFilteredByFilter(m_search_filter, entry.name, listing->path.GetPath(), entry.dir, entry.size, 0))
			continue;

		CSearchFileData data;
		data.flags = CComparableListing::normal;
		data.entry = entry;
		data.path = listing->path;
		data.icon = entry.dir ? m_results->m_dirIcon : -2;
		m_results->m_fileData.push_back(data);
		m_results->m_indexMapping.push_back(old_count + added++);
	}

	m_results->SetItemCount(old_count + added);

	m_results->SortList(-1, -1, true);

	m_results->RefreshListOnly(false);
}

void CSearchDialog::OnSearch(wxCommandEvent& event)
{
	if (!m_pState->IsRemoteIdle())
	{
		wxBell();
		return;
	}

	CServerPath path;

	const CServer* pServer = m_pState->GetServer();
	if (!pServer)
	{
		wxMessageBox(_("Connection to server lost."), _("Remote file search"), wxICON_EXCLAMATION);
		return;
	}
	path.SetType(pServer->GetType());
	if (!path.SetPath(XRCCTRL(*this, "ID_PATH", wxTextCtrl)->GetValue()) || path.IsEmpty())
	{
		wxMessageBox(_("Need to enter valid remote path"), _("Remote file search"), wxICON_EXCLAMATION);
		return;
	}

	m_search_root = path;

	// Prepare filter
	wxString error;
	if (!ValidateFilter(error, true))
	{
		wxMessageBox(wxString::Format(_("Invalid filter: %s"), error.c_str()), _("Remote file search"), wxICON_EXCLAMATION);
		return;
	}
	m_search_filter = GetFilter();
	m_search_filter.matchCase = XRCCTRL(*this, "ID_CASE", wxCheckBox)->GetValue();

	// Delete old results
	m_results->m_indexMapping.clear();
	m_results->m_fileData.clear();
	m_results->SetItemCount(0);
	m_visited.clear();
	m_results->RefreshListOnly(true);

	// Start
	m_searching = true;
	m_pState->GetRecursiveOperationHandler()->AddDirectoryToVisitRestricted(path, _T(""), true);
	std::list<CFilter> filters; // Empty, recurse into everything
	m_pState->GetRecursiveOperationHandler()->StartRecursiveOperation(CRecursiveOperation::recursive_list, path, filters, true);
}

void CSearchDialog::OnStop(wxCommandEvent& event)
{
	if (!m_pState->IsRemoteIdle())
	{
		m_pState->m_pCommandQueue->Cancel();
		m_pState->GetRecursiveOperationHandler()->StopRecursiveOperation();
	}
}

void CSearchDialog::SetCtrlState()
{
	bool idle = m_pState->IsRemoteIdle();
	XRCCTRL(*this, "ID_START", wxButton)->Enable(idle);
	XRCCTRL(*this, "ID_STOP", wxButton)->Enable(!idle);
}

void CSearchDialog::OnContextMenu(wxContextMenuEvent& event)
{
	if (event.GetEventObject() != m_results)
	{
		event.Skip();
		return;
	}

	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_SEARCH"));
	if (!pMenu)
		return;

	if (!m_pState->IsRemoteIdle())
	{
		pMenu->Enable(XRCID("ID_MENU_SEARCH_DOWNLOAD"), false);
		pMenu->Enable(XRCID("ID_MENU_SEARCH_DELETE"), false);
	}

	PopupMenu(pMenu);
	delete pMenu;
}



class CSearchDownloadDialog : public wxDialogEx
{
public:
	bool Run(wxWindow* parent, const wxString& m_local_dir, int count_files, int count_dirs)
	{
		if (!Load(parent, _T("ID_SEARCH_DOWNLOAD")))
			return false;

		wxString desc;
		if (!count_dirs)
			desc.Printf(wxPLURAL("Selected %d file for transfer.", "Selected %d files for transfer.", count_files), count_files);
		else if (!count_files)
			desc.Printf(wxPLURAL("Selected %d directory with its contents for transfer.", "Selected %d directories with their contents for transfer.", count_dirs), count_dirs);
		else
		{
			wxString files = wxString::Format(wxPLURAL("%d file", "%d files", count_files), count_files);
			wxString dirs = wxString::Format(wxPLURAL("%d directory with its contents", "%d directories with their contents", count_dirs), count_dirs);
			desc.Printf(_("Selected %s and %s for transfer."), files.c_str(), dirs.c_str());
		}
		XRCCTRL(*this, "ID_DESC", wxStaticText)->SetLabel(desc);

		XRCCTRL(*this, "ID_LOCALPATH", wxTextCtrl)->ChangeValue(m_local_dir);

		if (ShowModal() != wxID_OK)
			return false;

		return true;
	}

protected:

	DECLARE_EVENT_TABLE();
	void OnBrowse(wxCommandEvent& event);
	void OnOK(wxCommandEvent& event);
};

BEGIN_EVENT_TABLE(CSearchDownloadDialog, wxDialogEx)
EVT_BUTTON(XRCID("ID_BROWSE"), CSearchDownloadDialog::OnBrowse)
EVT_BUTTON(XRCID("wxID_OK"), CSearchDownloadDialog::OnOK)
END_EVENT_TABLE()

void CSearchDownloadDialog::OnBrowse(wxCommandEvent& event)
{
	wxTextCtrl *pText = XRCCTRL(*this, "ID_LOCALPATH", wxTextCtrl);

	wxDirDialog dlg(this, _("Select target download directory"), pText->GetValue(), wxDD_NEW_DIR_BUTTON);
	if (dlg.ShowModal() == wxID_OK)
		pText->ChangeValue(dlg.GetPath());
}

void CSearchDownloadDialog::OnOK(wxCommandEvent& event)
{
	wxTextCtrl *pText = XRCCTRL(*this, "ID_LOCALPATH", wxTextCtrl);

	CLocalPath path(pText->GetValue());
	if (path.empty())
	{
		wxMessageBox(_("You have to enter a local directory."), _("Download search results"), wxICON_EXCLAMATION);
		return;
	}

	if (!path.IsWriteable())
	{
		wxMessageBox(_("You have to enter a writable local directory."), _("Download search results"), wxICON_EXCLAMATION);
		return;
	}

	EndDialog(wxID_OK);
}

void CSearchDialog::ProcessSelection(std::list<int> &selected_files, std::list<CServerPath> &selected_dirs)
{
	int sel = -1;
	while ((sel = m_results->GetNextItem(sel, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
	{
		if (sel > (int)m_results->m_indexMapping.size())
			continue;
		int index = m_results->m_indexMapping[sel];

		if (m_results->m_fileData[index].entry.dir)
		{
			CServerPath path = m_results->m_fileData[index].path;
			path.ChangePath(m_results->m_fileData[index].entry.name);
			if (path.IsEmpty())
				continue;

			bool replaced = false;
			std::list<CServerPath>::iterator iter = selected_dirs.begin();
			std::list<CServerPath>::iterator prev;

			// Make sure that selected_dirs does not contain
			// any directories that are in a parent-child relationship
			// Resolve by only keeping topmost parents
			while (iter != selected_dirs.end())
			{
				if (*iter == path)
				{
					replaced = true;
					break;
				}

				if (iter->IsParentOf(path, false))
				{
					replaced = true;
					break;
				}

				if (iter->IsSubdirOf(path, false))
				{
					if (!replaced)
					{
						*iter = path;
						replaced = true;
					}
					else
					{
						prev = iter++;
						selected_dirs.erase(prev);
						continue;
					}
				}
				iter++;
			}
			if (!replaced)
				selected_dirs.push_back(path);
		}
		else
			selected_files.push_back(index);
	}

	// Now in a second phase filter out all files that are also in a directory
	std::list<int> selected_files_new;
	for (std::list<int>::const_iterator iter = selected_files.begin(); iter != selected_files.end(); iter++)
	{
		CServerPath path = m_results->m_fileData[*iter].path;
		std::list<CServerPath>::const_iterator path_iter;
		for (path_iter = selected_dirs.begin(); path_iter != selected_dirs.end(); path_iter++)
		{
			if (*path_iter == path || path_iter->IsParentOf(path, false))
				break;
		}
		if (path_iter == selected_dirs.end())
			selected_files_new.push_back(*iter);
	}
	selected_files.swap(selected_files_new);
	
	// At this point selected_dirs contains uncomparable
	// paths and selected_files contains only files not
	// covered by any of those directories.
}

void CSearchDialog::OnDownload(wxCommandEvent& event)
{
	if (!m_pState->IsRemoteIdle())
		return;

	// Find all selected files and directories
	std::list<CServerPath> selected_dirs;
	std::list<int> selected_files;
	ProcessSelection(selected_files, selected_dirs);

	if (selected_files.empty() && selected_dirs.empty())
		return;

	if (selected_dirs.size() > 1)
	{
		wxMessageBox(_("Downloading multiple unrelated directories is not yet supported"), _("Downloading search results"), wxICON_EXCLAMATION);
		return;
	}

	CSearchDownloadDialog dlg;
	if (!dlg.Run(this, m_local_target.GetPath(), selected_files.size(), selected_dirs.size()))
		return;

	wxTextCtrl *pText = XRCCTRL(dlg, "ID_LOCALPATH", wxTextCtrl);

	CLocalPath path(pText->GetValue());
	if (path.empty() || !path.IsWriteable())
	{
		wxBell();
		return;
	}
	m_local_target = path;

	const CServer* pServer = m_pState->GetServer();
	if (!pServer)
	{
		wxBell();
		return;
	}

	bool start = XRCCTRL(dlg, "ID_QUEUE_START", wxRadioButton)->GetValue();
	bool flatten = XRCCTRL(dlg, "ID_PATHS_FLATTEN", wxRadioButton)->GetValue();

	for (std::list<int>::const_iterator iter = selected_files.begin(); iter != selected_files.end(); iter++)
	{
		const CDirentry& entry = m_results->m_fileData[*iter].entry;

		CLocalPath target_path = path;
		if (!flatten)
		{
			// Append relative path to search root to local target path
			CServerPath remote_path = m_results->m_fileData[*iter].path;
			std::list<wxString> segments;
			while (m_search_root.IsParentOf(remote_path, false) && remote_path.HasParent())
			{
				segments.push_front(remote_path.GetLastSegment());
				remote_path = remote_path.GetParent();
			}
			for (std::list<wxString>::const_iterator segment_iter = segments.begin(); segment_iter != segments.end(); segment_iter++)
				target_path.AddSegment(*segment_iter);
		}

		CServerPath remote_path = m_results->m_fileData[*iter].path;
		m_pQueue->QueueFile(!start, true, target_path.GetPath() + entry.name, entry.name, remote_path, *pServer, entry.size);
	}
	m_pQueue->QueueFile_Finish(start);

	enum CRecursiveOperation::OperationMode mode;
	if (flatten)
		mode = start ? CRecursiveOperation::recursive_download_flatten : CRecursiveOperation::recursive_addtoqueue_flatten;
	else
		mode = start ? CRecursiveOperation::recursive_download : CRecursiveOperation::recursive_addtoqueue;

	for (std::list<CServerPath>::const_iterator iter = selected_dirs.begin(); iter != selected_dirs.end(); iter++)
	{
		CLocalPath target_path = path;
		if (!flatten && iter->HasParent())
			target_path.AddSegment(iter->GetLastSegment());

		m_pState->GetRecursiveOperationHandler()->AddDirectoryToVisit(*iter, _T(""), target_path.GetPath(), false);
		std::list<CFilter> filters; // Empty, recurse into everything
		m_pState->GetRecursiveOperationHandler()->StartRecursiveOperation(mode, *iter, filters, true, m_original_dir);
	}
}

void CSearchDialog::OnDelete(wxCommandEvent& event)
{
	if (!m_pState->IsRemoteIdle())
		return;

	// Find all selected files and directories
	std::list<CServerPath> selected_dirs;
	std::list<int> selected_files;
	ProcessSelection(selected_files, selected_dirs);

	if (selected_files.empty() && selected_dirs.empty())
		return;

	if (selected_dirs.size() > 1)
	{
		wxMessageBox(_("Deleting multiple unrelated directories is not yet supported"), _("Deleting search results"), wxICON_EXCLAMATION);
		return;
	}

	wxString question;
	if (selected_dirs.empty())
		question.Printf(wxPLURAL("Really delete %d file?", "Really delete %d files?", selected_files.size()), selected_files.size());
	else if (selected_files.empty())
		question.Printf(wxPLURAL("Really delete %d directory with its contents?", "Really delete %d directories with their contents?", selected_dirs.size()), selected_dirs.size());
	else
	{
		wxString files = wxString::Format(wxPLURAL("%d file", "%d files", selected_files.size()), selected_files.size());
		wxString dirs = wxString::Format(wxPLURAL("%d directory with its contents", "%d directories with their contents", selected_dirs.size()), selected_dirs.size());
		question.Printf(_("Really delete %s and %s?"), files.c_str(), dirs.c_str());
	}

	if (wxMessageBox(question, _("Deleting search results"), wxICON_QUESTION | wxYES_NO) != wxYES)
		return;

	for (std::list<int>::const_iterator iter = selected_files.begin(); iter != selected_files.end(); iter++)
	{
		const CDirentry& entry = m_results->m_fileData[*iter].entry;
		std::list<wxString> files_to_delete;
		files_to_delete.push_back(entry.name);
		m_pState->m_pCommandQueue->ProcessCommand(new CDeleteCommand(m_results->m_fileData[*iter].path, files_to_delete));
	}

	for (std::list<CServerPath>::const_iterator iter = selected_dirs.begin(); iter != selected_dirs.end(); iter++)
	{
		CServerPath path = *iter;
		if (!path.HasParent())
			m_pState->GetRecursiveOperationHandler()->AddDirectoryToVisit(path, _T(""));
		else
		{
			m_pState->GetRecursiveOperationHandler()->AddDirectoryToVisit(path.GetParent(), path.GetLastSegment());
			path = path.GetParent();
		}

		std::list<CFilter> filters; // Empty, recurse into everything
		m_pState->GetRecursiveOperationHandler()->StartRecursiveOperation(CRecursiveOperation::recursive_delete, path, filters, !path.HasParent(), m_original_dir);
	}
}
