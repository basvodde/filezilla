#define FILELISTCTRL_INCLUDE_TEMPLATE_DEFINITION

#include "FileZilla.h"
#include "search.h"
#include "filelistctrl.h"
#include "recursive_operation.h"
#include "commandqueue.h"
#include "Options.h"
#include "window_state_manager.h"

class CSearchFileData : public CGenericFileData
{
public:
	CServerPath path;

	CDirentry entry;
};

// Helper classes for fast sorting using std::sort
// -----------------------------------------------

class CSearchSort : public CListViewSort
{
public:
	CSearchSort(const std::vector<CSearchFileData> &fileData, enum DirSortMode dirSortMode)
		: m_fileData(fileData), m_dirSortMode(dirSortMode)
	{
	}

	bool operator()(int a, int b) const
	{
		return m_fileData[a].entry.name < m_fileData[b].entry.name;
	}

protected:
	const std::vector<CSearchFileData> &m_fileData;

	const enum DirSortMode m_dirSortMode;
};

// Search dialog file list
// -----------------------

// Defined in LocalListView.cpp
extern wxString FormatSize(const wxLongLong& size, bool add_bytes_suffix = false);

// Defined in RemoteListView.cpp
extern wxString StripVMSRevision(const wxString& name);

class CSearchDialogFileList : public CFileListCtrl<CSearchFileData>, public CSystemImageList
{
	friend class CSearchDialog;
public:
	CSearchDialogFileList(CSearchDialog* pParent, CState* pState, CQueueView* pQueue)
		: CFileListCtrl<CSearchFileData>(pParent, pState, pQueue),
		CSystemImageList(16), m_searchDialog(pParent)
	{
		m_hasParent = false;

		SetImageList(GetSystemImageList(), wxIMAGE_LIST_SMALL);

		m_dirIcon = GetIconIndex(dir);

		InitDateFormat();

		const unsigned long widths[7] = { 130, 130, 75, 80, 100, 80, 80 };

		AddColumn(_("Filename"), wxLIST_FORMAT_LEFT, widths[0]);
		AddColumn(_("Path"), wxLIST_FORMAT_LEFT, widths[1]);
		AddColumn(_("Filesize"), wxLIST_FORMAT_RIGHT, widths[2]);
		AddColumn(_("Filetype"), wxLIST_FORMAT_LEFT, widths[3]);
		AddColumn(_("Last modified"), wxLIST_FORMAT_LEFT, widths[4]);
		AddColumn(_("Permissions"), wxLIST_FORMAT_LEFT, widths[5]);
		AddColumn(_("Owner/Group"), wxLIST_FORMAT_LEFT, widths[6]);
		LoadColumnSettings(OPTION_SEARCH_COLUMN_WIDTHS, OPTION_SEARCH_COLUMN_SHOWN, OPTION_SEARCH_COLUMN_ORDER);
	}

protected:
	virtual bool ItemIsDir(int index) const
	{
		return m_fileData[index].entry.dir;
	}

	virtual wxLongLong ItemGetSize(int index) const
	{
		return m_fileData[index].entry.size;
	}

	CFileListCtrl<CSearchFileData>::CSortComparisonObject GetSortComparisonObject()
	{
		CSearchSort::DirSortMode dirSortMode = GetDirSortMode();
		return CFileListCtrl<CSearchFileData>::CSortComparisonObject(new CSearchSort(m_fileData, dirSortMode));
	}

	CSearchDialog *m_searchDialog;

	virtual wxString GetItemText(int item, unsigned int column)
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

	// See comment to OnGetItemText
	int OnGetItemImage(long item) const
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

	void InitDateFormat()
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

// Search dialog
// -------------

BEGIN_EVENT_TABLE(CSearchDialog, CFilterConditionsDialog)
EVT_BUTTON(XRCID("ID_START"), CSearchDialog::OnSearch)
EVT_BUTTON(XRCID("ID_STOP"), CSearchDialog::OnStop)
END_EVENT_TABLE()

CSearchDialog::CSearchDialog(wxWindow* parent, CState* pState)
	: CStateEventHandler(pState)
{
	m_parent = parent;
	m_pWindowStateManager = 0;
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

	EditFilter(CFilter());

	m_results = new CSearchDialogFileList(this, m_pState, 0);
	ReplaceControl(XRCCTRL(*this, "ID_RESULTS", wxWindow), m_results);

	const CServerPath path = m_pState->GetRemotePath();
	if (!path.IsEmpty())
		XRCCTRL(*this, "ID_PATH", wxTextCtrl)->ChangeValue(path.GetPath());

	SetCtrlState();

	m_pWindowStateManager = new CWindowStateManager(this);
	m_pWindowStateManager->Restore(OPTION_SEARCH_SIZE, wxSize(700, 450));

	return true;
}

void CSearchDialog::Run()
{
	const CServerPath original_dir = m_pState->GetRemotePath();

	m_pState->BlockHandlers(STATECHANGE_REMOTE_DIR);
	m_pState->BlockHandlers(STATECHANGE_REMOTE_DIR_MODIFIED);
	m_pState->RegisterHandler(this, STATECHANGE_REMOTE_DIR);
	m_pState->RegisterHandler(this, STATECHANGE_REMOTE_IDLE);

	ShowModal();

	m_pState->UnregisterHandler(this, STATECHANGE_REMOTE_IDLE);
	m_pState->UnregisterHandler(this, STATECHANGE_REMOTE_DIR);
	m_pState->UnblockHandlers(STATECHANGE_REMOTE_DIR);
	m_pState->UnblockHandlers(STATECHANGE_REMOTE_DIR_MODIFIED);

	if (!m_pState->m_pCommandQueue->Idle())
	{
		m_pState->m_pCommandQueue->Cancel();
		m_pState->GetRecursiveOperationHandler()->StopRecursiveOperation();
	}
	if (!original_dir.IsEmpty())
		m_pState->ChangeRemoteDir(original_dir);
}

void CSearchDialog::OnStateChange(enum t_statechange_notifications notification, const wxString& data)
{
	if (notification == STATECHANGE_REMOTE_DIR)
		ProcessDirectoryListing();
	else if (notification == STATECHANGE_REMOTE_IDLE)
		SetCtrlState();
}

void CSearchDialog::ProcessDirectoryListing()
{
	CSharedPointer<const CDirectoryListing> listing = m_pState->GetRemoteDir();

	if (!listing || listing->m_failed)
		return;

	int old_count = m_results->m_fileData.size();
	bool added = 0;
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

	m_results->SortList(-1, -1, false);
}

void CSearchDialog::OnSearch(wxCommandEvent& event)
{
	if (!m_pState->m_pCommandQueue->Idle())
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

	// Prepare filter
	wxString error;
	if (!ValidateFilter(error, true))
	{
		wxMessageBox(wxString::Format(_("Invalid filter: %s"), error.c_str()), _("Remote file search"), wxICON_EXCLAMATION);
		return;
	}
	m_search_filter = GetFilter();

	// Delete old results
	m_results->m_indexMapping.clear();
	m_results->m_fileData.clear();
	m_results->SetItemCount(0);

	// Start
	m_pState->GetRecursiveOperationHandler()->AddDirectoryToVisitRestricted(path, _T(""), true);
	std::list<CFilter> filters; // Empty, recurse into everything
	m_pState->GetRecursiveOperationHandler()->StartRecursiveOperation(CRecursiveOperation::recursive_list, path, filters, true);
}

void CSearchDialog::OnStop(wxCommandEvent& event)
{
	if (!m_pState->m_pCommandQueue->Idle())
	{
		m_pState->m_pCommandQueue->Cancel();
		m_pState->GetRecursiveOperationHandler()->StopRecursiveOperation();
	}
}

void CSearchDialog::SetCtrlState()
{
	bool idle = m_pState->m_pCommandQueue->Idle();
	XRCCTRL(*this, "ID_START", wxButton)->Enable(idle);
	XRCCTRL(*this, "ID_STOP", wxButton)->Enable(!idle);
}
