#include "FileZilla.h"
#include "edithandler.h"
#include "dialogex.h"
#include "filezillaapp.h"
#include "queue.h"
#include "Options.h"

// Defined in optionspage_edit.cpp
bool UnquoteCommand(wxString& command, wxString& arguments);
bool ProgramExists(const wxString& editor);

class CChangedFileDialog : public wxDialogEx
{
	DECLARE_EVENT_TABLE();
	void OnYes(wxCommandEvent& event);
	void OnNo(wxCommandEvent& event);
};

BEGIN_EVENT_TABLE(CChangedFileDialog, wxDialogEx)
EVT_BUTTON(wxID_YES, CChangedFileDialog::OnYes)
EVT_BUTTON(wxID_NO, CChangedFileDialog::OnNo)
END_EVENT_TABLE()

void CChangedFileDialog::OnYes(wxCommandEvent& event)
{
	EndDialog(wxID_YES);
}

void CChangedFileDialog::OnNo(wxCommandEvent& event)
{
	EndDialog(wxID_NO);
}

//-------------

BEGIN_EVENT_TABLE(CEditHandler, wxEvtHandler)
EVT_TIMER(wxID_ANY, CEditHandler::OnTimerEvent)
END_EVENT_TABLE()

CEditHandler* CEditHandler::m_pEditHandler = 0;

CEditHandler::CEditHandler()
{
	m_pQueue = 0;

	m_timer.SetOwner(this);

#ifdef __WXMSW__
	m_lockfile_handle = INVALID_HANDLE_VALUE;
#else
	m_lockfile_descriptor = -1;
#endif
}

CEditHandler* CEditHandler::Create()
{
	if (!m_pEditHandler)
		m_pEditHandler = new CEditHandler();

	return m_pEditHandler;
}

CEditHandler* CEditHandler::Get()
{
	return m_pEditHandler;
}

void CEditHandler::RemoveTemporaryFiles(const wxString& temp)
{
	wxDir dir(temp);
	if (!dir.IsOpened())
		return;

	wxString file;
	if (!dir.GetFirst(&file, _T("fz3temp-*"), wxDIR_DIRS))
		return;

	const wxChar& sep = wxFileName::GetPathSeparator();
	do
	{
		const wxString lockfile = temp + file + sep + _("fz3temp-lockfile");
		if (wxFileName::FileExists(lockfile))
		{
#ifndef __WXMSW__
			int fd = open(lockfile.mb_str(), O_RDWR, 0);
			if (fd >= 0)
			{
				// Try to lock 1 byte region in the lockfile. m_type specifies the byte to lock.
				struct flock f = {0};
				f.l_type = F_WRLCK;
				f.l_whence = SEEK_SET;
				f.l_start = 0;
				f.l_len = 1;
				f.l_pid = getpid();
				if (fcntl(fd, F_SETLK, &f))
				{
					// In use by other process
					continue;
				}
				close(fd);
			}
#endif
			wxRemoveFile(lockfile);
			if (wxFileName::FileExists(lockfile))
				continue;
		}

		{
			wxString file2;
			wxDir dir2(temp + file);
			bool res;
			for ((res = dir2.GetFirst(&file2, _T(""), wxDIR_FILES)); res; res = dir2.GetNext(&file2));
				wxRemoveFile(temp + file + sep + file2);
		}

		wxRmdir(temp + file + sep);
	} while (dir.GetNext(&file));
}

wxString CEditHandler::GetLocalDirectory()
{
	if (m_localDir != _T(""))
		return m_localDir;

	wxString dir = wxFileName::GetTempDir();
	if (dir == _T("") || !wxFileName::DirExists(dir))
		return _T("");

	if (dir.Last() != wxFileName::GetPathSeparator())
		dir += wxFileName::GetPathSeparator();

	RemoveTemporaryFiles(dir);

	// On POSIX, the permissions of the created directory (700) ensure
	// that this is a safe operation.
	// On Windows, the user's profile directory and associated temp dir
	// already has the correct permissions which get inherited.
	int i = 1;
	do
	{
		wxString newDir = dir + wxString::Format(_T("fz3temp-%d"), i++);
		if (wxFileName::FileExists(newDir) || wxFileName::DirExists(newDir))
			continue;

		if (!wxMkdir(newDir, 0700))
			return _T("");

		m_localDir = newDir + wxFileName::GetPathSeparator();
		break;
	} while (true);

#ifdef __WXMSW__
	m_lockfile_handle = ::CreateFile(m_localDir + _T("fz3temp-lockfile"), GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, 0);
	if (m_lockfile_handle == INVALID_HANDLE_VALUE)
	{
		wxRmdir(m_localDir);
		m_localDir = _T("");
	}
#else
	wxString file = m_localDir + _T("fz3temp-lockfile");
	m_lockfile_descriptor = open(file.mb_str(), O_CREAT | O_RDWR, 0600);
	if (m_lockfile_descriptor >= 0)
	{
		// Lock 1 byte region in the lockfile.
		struct flock f = {0};
		f.l_type = F_WRLCK;
		f.l_whence = SEEK_SET;
		f.l_start = 0;
		f.l_len = 1;
		f.l_pid = getpid();
		fcntl(m_lockfile_descriptor, F_SETLKW, &f);
	}
#endif

	return m_localDir;
}

void CEditHandler::Release()
{
	if (m_timer.IsRunning())
		m_timer.Stop();

	if (m_localDir != _T(""))
	{
#ifdef __WXMSW__
		if (m_lockfile_handle != INVALID_HANDLE_VALUE)
			CloseHandle(m_lockfile_handle);
		wxRemoveFile(m_localDir + _T("fz3temp-lockfile"));
#else
		wxRemoveFile(m_localDir + _T("fz3temp-lockfile"));
		if (m_lockfile_descriptor >= 0)
			close(m_lockfile_descriptor);
#endif

		RemoveAll(true);
		wxRmdir(m_localDir);
	}

	m_pEditHandler = 0;
	delete this;
}

enum CEditHandler::fileState CEditHandler::GetFileState(enum CEditHandler::fileType type, const wxString& fileName) const
{
	wxASSERT(type != none);
	std::list<t_fileData>::const_iterator iter = GetFile(type, fileName);
	if (iter == m_fileDataList[type].end())
		return unknown;

	return iter->state;
}

int CEditHandler::GetFileCount(enum CEditHandler::fileType type, enum CEditHandler::fileState state) const
{
	int count = 0;
	if (state == unknown)
	{
		if (type != remote)
			count += m_fileDataList[local].size();
		if (type != local)
			count += m_fileDataList[remote].size();
	}
	else
	{
		if (type != remote)
		{
			for (std::list<t_fileData>::const_iterator iter = m_fileDataList[local].begin(); iter != m_fileDataList[local].end(); iter++)
			{
				if (iter->state == state)
					count++;
			}
		}
		if (type != local)
		{
			for (std::list<t_fileData>::const_iterator iter = m_fileDataList[remote].begin(); iter != m_fileDataList[remote].end(); iter++)
			{
				if (iter->state == state)
					count++;
			}
		}
	}

	return count;
}

bool CEditHandler::AddFile(enum CEditHandler::fileType type, const wxString& fileName, const CServerPath& remotePath, const CServer& server)
{
	wxASSERT(type != none);
	wxASSERT(GetFileState(type, fileName) == unknown);
	if (GetFileState(type, fileName) != unknown)
		return false;

	t_fileData data;
	data.name = fileName;
	if (type == remote)
		data.state = download;
	else
		data.state = edit;
	data.remotePath = remotePath;
	data.server = server;

	if (type == remote || StartEditing(type, data))
		m_fileDataList[type].push_back(data);

	return true;
}

bool CEditHandler::Remove(enum CEditHandler::fileType type, const wxString& fileName)
{
	wxASSERT(type != none);

	std::list<t_fileData>::iterator iter = GetFile(type, fileName);
	if (iter == m_fileDataList[type].end())
		return true;

	if (type == remote)
	{
		wxASSERT(iter->state != download && iter->state != upload && iter->state != upload_and_remove);
		if (iter->state == download || iter->state == upload || iter->state == upload_and_remove)
			return false;

		if (wxFileName::FileExists(m_localDir + iter->name))
		{
			if (!wxRemoveFile(m_localDir + iter->name))
			{
				iter->state = removing;
				return false;
			}
		}
	}
	else
	{
		wxASSERT(iter->state != upload && iter->state != upload_and_remove);
		if (iter->state == upload || iter->state == upload_and_remove)
			return false;
	}

	m_fileDataList[type].erase(iter);

	return true;
}

bool CEditHandler::RemoveAll(bool force)
{
	std::list<t_fileData> keep;

	for (std::list<t_fileData>::iterator iter = m_fileDataList[remote].begin(); iter != m_fileDataList[remote].end(); iter++)
	{
		if (!force && (iter->state == download || iter->state == upload || iter->state == upload_and_remove))
		{
			keep.push_back(*iter);
			continue;
		}

		if (wxFileName::FileExists(m_localDir + iter->name))
		{
			if (!wxRemoveFile(m_localDir + iter->name))
			{
				iter->state = removing;
				keep.push_back(*iter);
				continue;
			}
		}
	}
	m_fileDataList[remote].clear();
	m_fileDataList[remote].swap(keep);
	keep.clear();

	for (std::list<t_fileData>::iterator iter = m_fileDataList[local].begin(); iter != m_fileDataList[local].end(); iter++)
	{
		if (force)
			continue;

		if (iter->state == upload || iter->state == upload_and_remove)
		{
			keep.push_back(*iter);
			continue;
		}
	}
	m_fileDataList[local].swap(keep);

	return m_fileDataList[local].empty() && m_fileDataList[remote].empty();
}

std::list<CEditHandler::t_fileData>::iterator CEditHandler::GetFile(enum CEditHandler::fileType type, const wxString& fileName)
{
	wxASSERT(type != none);

	std::list<t_fileData>::iterator iter;
	for (iter = m_fileDataList[type].begin(); iter != m_fileDataList[type].end(); iter++)
	{
		if (iter->name == fileName)
			break;
	}

	return iter;
}

std::list<CEditHandler::t_fileData>::const_iterator CEditHandler::GetFile(enum CEditHandler::fileType type, const wxString& fileName) const
{
	wxASSERT(type != none);

	std::list<t_fileData>::const_iterator iter;
	for (iter = m_fileDataList[type].begin(); iter != m_fileDataList[type].end(); iter++)
	{
		if (iter->name == fileName)
			break;
	}

	return iter;
}

void CEditHandler::FinishTransfer(bool successful, enum CEditHandler::fileType type, const wxString& fileName)
{
	wxASSERT(type != none);

	std::list<t_fileData>::iterator iter = GetFile(type, fileName);
	if (iter == m_fileDataList[type].end())
		return;

	if (type == remote)
	{
		wxASSERT(iter->state == download || iter->state == upload || iter->state == upload_and_remove);

		switch (iter->state)
		{
		case upload_and_remove:
			if (wxFileName::FileExists(m_localDir + fileName) && !wxRemoveFile(m_localDir + fileName))
				iter->state = removing;
			else
				m_fileDataList[type].erase(iter);
			break;
		case upload:
			if (wxFileName::FileExists(m_localDir + fileName))
				iter->state = edit;
			else
				m_fileDataList[type].erase(iter);
			break;
		case download:
			if (wxFileName::FileExists(m_localDir + fileName))
			{
				iter->state = edit;
				if (StartEditing(remote, *iter))
					break;
			}
			if (wxFileName::FileExists(m_localDir + fileName) && !wxRemoveFile(m_localDir + fileName))
				iter->state = removing;
			else
				m_fileDataList[type].erase(iter);
			break;
		default:
			return;
		}
	}
	else
	{
		wxASSERT(iter->state == upload || iter->state == upload_and_remove);

		switch (iter->state)
		{
		case upload_and_remove:
			m_fileDataList[type].erase(iter);
			break;
		case upload:
			if (wxFileName::FileExists(fileName))
				iter->state = edit;
			else
				m_fileDataList[type].erase(iter);
			break;
		default:
			return;
		}
	}

	SetTimerState();
}

bool CEditHandler::StartEditing(enum CEditHandler::fileType type, const wxString& file)
{
	wxASSERT(type != none);

	std::list<t_fileData>::iterator iter = GetFile(type, file);
	if (iter == m_fileDataList[type].end())
		return false;

	return StartEditing(type, *iter);
}

bool CEditHandler::StartEditing(enum CEditHandler::fileType type, t_fileData& data)
{
	wxASSERT(type != none);
	wxASSERT(data.state == edit);

	wxFileName fn;
	if (type == remote)
		fn = wxFileName(m_localDir, data.name);
	else
		fn = wxFileName(data.name);

	if (!fn.FileExists())
		return false;
	data.modificationTime = fn.GetModificationTime();

	wxString cmd = GetOpenCommand(fn.GetFullPath());
	if (cmd == _T(""))
		return false;

	if (!wxExecute(cmd))
		return false;

	return true;
}

void CEditHandler::CheckForModifications()
{
	static bool insideCheckForModifications = false;
	if (insideCheckForModifications)
		return;

	insideCheckForModifications = true;

checkmodifications_remote:
	for (std::list<t_fileData>::iterator iter = m_fileDataList[remote].begin(); iter != m_fileDataList[remote].end(); iter++)
	{
		if (iter->state != edit)
			continue;

		wxFileName fn(m_localDir, iter->name);
		if (!fn.FileExists())
		{
			m_fileDataList[remote].erase(iter);

			// Evil goto. Imo the next C++ standard needs a comefrom keyword.
			goto checkmodifications_remote;
		}

		wxDateTime mtime = fn.GetModificationTime();
		if (!mtime.IsValid())
			continue;

		if (iter->modificationTime.IsValid() & iter->modificationTime >= mtime)
			continue;

		// File has changed, ask user what to do
		CChangedFileDialog dlg;
		if (!dlg.Load(wxTheApp->GetTopWindow(), _T("ID_CHANGEDFILE")))
			continue;
		XRCCTRL(dlg, "ID_DESC_UPLOAD_LOCAL", wxStaticText)->Hide();

		dlg.SetLabel(XRCID("ID_FILENAME"), iter->name);

		int res = dlg.ShowModal();

		const bool remove = XRCCTRL(dlg, "ID_DELETE", wxCheckBox)->IsChecked();

		if (res == wxID_YES)
		{
			UploadFile(remote, iter->name, remove);
			goto checkmodifications_remote;
		}
		else
		{
			if (!fn.FileExists() || wxRemoveFile(fn.GetFullPath()))
			{
				m_fileDataList[remote].erase(iter);
				goto checkmodifications_remote;
			}

			iter->state = removing;
		}
	}

checkmodifications_local:
	for (std::list<t_fileData>::iterator iter = m_fileDataList[local].begin(); iter != m_fileDataList[local].end(); iter++)
	{
		if (iter->state != edit)
			continue;

		wxFileName fn(iter->name);
		if (!fn.FileExists())
		{
			m_fileDataList[local].erase(iter);

			// Evil goto. Imo the next C++ standard needs a comefrom keyword.
			goto checkmodifications_local;
		}

		wxDateTime mtime = fn.GetModificationTime();
		if (!mtime.IsValid())
			continue;

		if (iter->modificationTime.IsValid() & iter->modificationTime >= mtime)
			continue;

		// File has changed, ask user what to do
		CChangedFileDialog dlg;
		if (!dlg.Load(wxTheApp->GetTopWindow(), _T("ID_CHANGEDFILE")))
			continue;
		XRCCTRL(dlg, "ID_DESC_UPLOAD_REMOTE", wxStaticText)->Hide();
		XRCCTRL(dlg, "ID_DELETE", wxCheckBox)->SetLabel(_("&Finish editing"));

		dlg.SetLabel(XRCID("ID_FILENAME"), iter->name);

		int res = dlg.ShowModal();

		const bool remove = XRCCTRL(dlg, "ID_DELETE", wxCheckBox)->IsChecked();

		if (res == wxID_YES)
			UploadFile(local, iter->name, remove);
		else
			m_fileDataList[local].erase(iter);
		goto checkmodifications_local;
	}

	SetTimerState();

	insideCheckForModifications = false;
}

bool CEditHandler::UploadFile(enum CEditHandler::fileType type, const wxString& fileName, bool unedit)
{
	wxASSERT(type != none);

	std::list<t_fileData>::iterator iter = GetFile(type, fileName);
	if (iter == m_fileDataList[type].end())
		return false;

	wxASSERT(iter->state == edit);
	if (iter->state != edit)
		return false;

	iter->state = unedit ? upload_and_remove : upload;

	wxFileName fn;
	if (type == remote)
		fn = wxFileName(m_localDir, iter->name);
	else
		fn = wxFileName(iter->name);

	if (!fn.FileExists())
	{
		m_fileDataList[type].erase(iter);
		return false;
	}

	wxDateTime mtime = fn.GetModificationTime();
	if (!mtime.IsValid())
		mtime = wxDateTime::Now();

	iter->modificationTime = mtime;

	wxASSERT(m_pQueue);
	wxULongLong size = fn.GetSize();
	
	m_pQueue->QueueFile(false, false, fn.GetFullPath(), fn.GetFullName(), iter->remotePath, iter->server, wxLongLong(size.GetHi(), size.GetLo()), type);

	return true;
}

void CEditHandler::OnTimerEvent(wxTimerEvent& event)
{
	CheckForModifications();
}

void CEditHandler::SetTimerState()
{
	bool editing = GetFileCount(none, edit) != 0;

	if (m_timer.IsRunning())
	{
		if (!editing)
			m_timer.Stop();
	}
	else if (editing)
		m_timer.Start(15000);
}

bool CEditHandler::CanOpen(enum CEditHandler::fileType type, const wxString& fileName, bool &dangerous)
{
	wxASSERT(type != none);

	wxString command = GetOpenCommand(fileName);
	if (command == _T(""))
		return false;

	wxFileName fn;
	if (type == remote)
		fn = wxFileName(m_localDir, fileName);
	else
		fn = wxFileName(fileName);

	wxString name = fn.GetFullPath();
	wxString tmp = command;
	wxString args;
	if (UnquoteCommand(tmp, args) && tmp == name)
		dangerous = true;
	else
		dangerous = false;

	return true;
}

wxString CEditHandler::GetOpenCommand(const wxString& file)
{
	if (!COptions::Get()->GetOptionVal(OPTION_EDIT_ALWAYSDEFAULT))
	{
		if (COptions::Get()->GetOptionVal(OPTION_EDIT_INHERITASSOCIATIONS))
		{
			const wxString command = GetSystemOpenCommand(file);
			if (command != _T(""))
				return command;
		}

		const wxString command = GetCustomOpenCommand(file);
		if (command != _T(""))
			return command;
	}

	wxString command = COptions::Get()->GetOption(OPTION_EDIT_DEFAULTEDITOR);
	if (command == _T(""))
		return _T("");

	wxString args;
	wxString editor = command;
	if (!UnquoteCommand(editor, args))
		return _T("");

	if (!ProgramExists(editor))
		return _T("");

	return command + _T(" \"") + file + _T("\"");
}

wxString CEditHandler::GetSystemOpenCommand(const wxString& file)
{
	wxFileName fn(file);

	const wxString& ext = fn.GetExt();
	if (ext == _T(""))
		return _T("");

	wxFileType* pType = wxTheMimeTypesManager->GetFileTypeFromExtension(ext);
	if (!pType)
		return _T("");

	wxString cmd;
	if (!pType->GetOpenCommand(&cmd, wxFileType::MessageParameters(file)))
	{
		delete pType;
		return _T("");
	}
	delete pType;

	return cmd;
}

wxString CEditHandler::GetCustomOpenCommand(const wxString& file)
{
	wxFileName fn(file);

	const wxString& ext = fn.GetExt();
	if (ext == _T(""))
		return _T("");

	wxString associations = COptions::Get()->GetOption(OPTION_EDIT_CUSTOMASSOCIATIONS) + _T("\n");
	associations.Replace(_T("\r"), _T(""));
	int pos;
	while ((pos = associations.Find('\n')) != -1)
	{
		wxString assoc = associations.Left(pos);
		associations = associations.Mid(pos + 1);

		if (assoc == _T(""))
			continue;

		wxString command;
		if (!UnquoteCommand(assoc, command))
			continue;

		if (assoc != ext)
			continue;

		wxString args;
		if (!UnquoteCommand(command, args))
			return _T("");

		if (command == _T(""))
			return _T("");

		if (!ProgramExists(command))
			return _T("");

		return command + _T(" \"") + fn.GetFullPath() + _T("\"");
	}

	return _T("");
}

BEGIN_EVENT_TABLE(CEditHandlerStatusDialog, wxDialogEx)
EVT_LIST_ITEM_SELECTED(wxID_ANY, CEditHandlerStatusDialog::OnSelectionChanged)
EVT_BUTTON(XRCID("ID_UNEDIT"), CEditHandlerStatusDialog::OnUnedit)
EVT_BUTTON(XRCID("ID_UPLOAD"), CEditHandlerStatusDialog::OnUpload)
EVT_BUTTON(XRCID("ID_UPLOADANDUNEDIT"), CEditHandlerStatusDialog::OnUpload)
EVT_BUTTON(XRCID("ID_EDIT"), CEditHandlerStatusDialog::OnEdit)
END_EVENT_TABLE()

CEditHandlerStatusDialog::CEditHandlerStatusDialog(wxWindow* parent)
	: m_pParent(parent)
{
}

int CEditHandlerStatusDialog::ShowModal()
{
	const CEditHandler* const pEditHandler = CEditHandler::Get();
	if (!pEditHandler)
		return wxID_CANCEL;

	if (!pEditHandler->GetFileCount(CEditHandler::none, CEditHandler::unknown))
	{
		wxMessageBox(_("No files are currently being edited"), _("Cannot show dialog"), wxICON_INFORMATION, m_pParent);
		return wxID_CANCEL;
	}

	if (!Load(m_pParent, _T("ID_EDITING")))
		return wxID_CANCEL;

	wxListCtrl* pListCtrl = XRCCTRL(*this, "ID_FILES", wxListCtrl);
	if (!pListCtrl)
		return wxID_CANCEL;

	pListCtrl->InsertColumn(0, _("Filename"));
	pListCtrl->InsertColumn(1, _("Type"));
	pListCtrl->InsertColumn(2, _("Status"));

	{
		const std::list<CEditHandler::t_fileData>& files = pEditHandler->GetFiles(CEditHandler::remote);
		unsigned int i = 0;
		for (std::list<CEditHandler::t_fileData>::const_iterator iter = files.begin(); iter != files.end(); iter++, i++)
		{
			pListCtrl->InsertItem(i, iter->name);
			pListCtrl->SetItem(i, 1, _("Remote"));
			switch (iter->state)
			{
			case CEditHandler::download:
				pListCtrl->SetItem(i, 2, _("Downloading"));
				break;
			case CEditHandler::upload:
				pListCtrl->SetItem(i, 2, _("Uploading"));
				break;
			case CEditHandler::upload_and_remove:
				pListCtrl->SetItem(i, 2, _("Uploading and pending removal"));
				break;
			case CEditHandler::removing:
				pListCtrl->SetItem(i, 2, _("Pending removal"));
				break;
			case CEditHandler::edit:
				pListCtrl->SetItem(i, 2, _("Being edited"));
				break;
			default:
				pListCtrl->SetItem(i, 2, _("Unknown"));
				break;
			}
			pListCtrl->SetItemData(i, CEditHandler::remote);
		}
	}

	{
		const std::list<CEditHandler::t_fileData>& files = pEditHandler->GetFiles(CEditHandler::local);
		unsigned int i = 0;
		for (std::list<CEditHandler::t_fileData>::const_iterator iter = files.begin(); iter != files.end(); iter++, i++)
		{
			pListCtrl->InsertItem(i, iter->name);
			pListCtrl->SetItem(i, 1, _("Local"));
			switch (iter->state)
			{
			case CEditHandler::upload:
				pListCtrl->SetItem(i, 2, _("Uploading"));
				break;
			case CEditHandler::upload_and_remove:
				pListCtrl->SetItem(i, 2, _("Uploading and unediting"));
				break;
			case CEditHandler::edit:
				pListCtrl->SetItem(i, 2, _("Being edited"));
				break;
			default:
				pListCtrl->SetItem(i, 2, _("Unknown"));
				break;
			}
			pListCtrl->SetItemData(i, CEditHandler::local);
		}
	}

	pListCtrl->SetColumnWidth(0, wxLIST_AUTOSIZE);
	pListCtrl->SetColumnWidth(1, wxLIST_AUTOSIZE);
	pListCtrl->SetColumnWidth(2, wxLIST_AUTOSIZE);
	pListCtrl->SetMinSize(wxSize(pListCtrl->GetColumnWidth(0) + pListCtrl->GetColumnWidth(1) + pListCtrl->GetColumnWidth(2) + 10, pListCtrl->GetMinSize().GetHeight()));
	GetSizer()->Fit(this);

	SetCtrlState();

	return wxDialogEx::ShowModal();
}

void CEditHandlerStatusDialog::SetCtrlState()
{
	const CEditHandler* const pEditHandler = CEditHandler::Get();
	if (!pEditHandler)
		return;

	wxListCtrl* pListCtrl = XRCCTRL(*this, "ID_FILES", wxListCtrl);

	bool selectedEdited = false;
	bool selectedOther = false;

	int item = -1;
	while ((item = pListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
	{
		const wxString name = pListCtrl->GetItemText(item);
		enum CEditHandler::fileType type = (enum CEditHandler::fileType)pListCtrl->GetItemData(item);
		if (pEditHandler->GetFileState(type, name) == CEditHandler::edit)
			selectedEdited = true;
		else
			selectedOther = true;
	}

	bool select = selectedEdited && !selectedOther;
	XRCCTRL(*this, "ID_UNEDIT", wxWindow)->Enable(select);
	XRCCTRL(*this, "ID_UPLOAD", wxWindow)->Enable(select);
	XRCCTRL(*this, "ID_UPLOADANDUNEDIT", wxWindow)->Enable(select);
	XRCCTRL(*this, "ID_EDIT", wxWindow)->Enable(select);
}

void CEditHandlerStatusDialog::OnSelectionChanged(wxListEvent& event)
{
	SetCtrlState();
}

void CEditHandlerStatusDialog::OnUnedit(wxCommandEvent& event)
{
	CEditHandler* const pEditHandler = CEditHandler::Get();
	if (!pEditHandler)
		return;

	wxListCtrl* pListCtrl = XRCCTRL(*this, "ID_FILES", wxListCtrl);

	std::list<int> files;
	int item = -1;
	while ((item = pListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
	{
		pListCtrl->SetItemState(item, 0, wxLIST_STATE_SELECTED);
		const wxString name = pListCtrl->GetItemText(item);
		enum CEditHandler::fileType type = (enum CEditHandler::fileType)pListCtrl->GetItemData(item);
		if (pEditHandler->GetFileState(type, name) != CEditHandler::edit)
		{
			wxBell();
			return;
		}

		files.push_front(item);
	}

	for (std::list<int>::const_iterator iter = files.begin(); iter != files.end(); iter++)
	{
		const int i = *iter;

		const wxString name = pListCtrl->GetItemText(i);
		enum CEditHandler::fileType type = (enum CEditHandler::fileType)pListCtrl->GetItemData(i);
		
		if (pEditHandler->Remove(type, name) || type == CEditHandler::local)
			pListCtrl->DeleteItem(i);
		else
			pListCtrl->SetItem(i, 2, _("Pending removal"));
	}

	SetCtrlState();
}

void CEditHandlerStatusDialog::OnUpload(wxCommandEvent& event)
{
	CEditHandler* const pEditHandler = CEditHandler::Get();
	if (!pEditHandler)
		return;

	wxListCtrl* pListCtrl = XRCCTRL(*this, "ID_FILES", wxListCtrl);

	std::list<int> files;
	int item = -1;
	while ((item = pListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
	{
		pListCtrl->SetItemState(item, 0, wxLIST_STATE_SELECTED);
		const wxString name = pListCtrl->GetItemText(item);
		enum CEditHandler::fileType type = (enum CEditHandler::fileType)pListCtrl->GetItemData(item);

		if (pEditHandler->GetFileState(type, name) != CEditHandler::edit)
		{
			wxBell();
			return;
		}
		files.push_front(item);
	}

	for (std::list<int>::const_iterator iter = files.begin(); iter != files.end(); iter++)
	{
		const int i = *iter;

		const wxString name = pListCtrl->GetItemText(i);
		enum CEditHandler::fileType type = (enum CEditHandler::fileType)pListCtrl->GetItemData(i);

		bool unedit = event.GetId() == XRCID("ID_UPLOADANDUNEDIT");
		pEditHandler->UploadFile(type, name, unedit);
		
		if (!unedit)
			pListCtrl->SetItem(i, 2, _("Uploading"));
		else if (type == CEditHandler::remote)
			pListCtrl->SetItem(i, 2, _("Uploading and pending removal"));
		else
			pListCtrl->SetItem(i, 2, _("Uploading and unediting"));
	}

	SetCtrlState();
}

void CEditHandlerStatusDialog::OnEdit(wxCommandEvent& event)
{
	CEditHandler* const pEditHandler = CEditHandler::Get();
	if (!pEditHandler)
		return;

	wxListCtrl* pListCtrl = XRCCTRL(*this, "ID_FILES", wxListCtrl);

	std::list<int> files;
	int item = -1;
	while ((item = pListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
	{
		pListCtrl->SetItemState(item, 0, wxLIST_STATE_SELECTED);
		const wxString name = pListCtrl->GetItemText(item);
		enum CEditHandler::fileType type = (enum CEditHandler::fileType)pListCtrl->GetItemData(item);

		if (pEditHandler->GetFileState(type, name) != CEditHandler::edit)
		{
			wxBell();
			return;
		}
		files.push_front(item);
	}

	for (std::list<int>::const_iterator iter = files.begin(); iter != files.end(); iter++)
	{
		const int i = *iter;

		const wxString name = pListCtrl->GetItemText(i);
		enum CEditHandler::fileType type = (enum CEditHandler::fileType)pListCtrl->GetItemData(i);

		if (!pEditHandler->StartEditing(type, name))
		{
			if (pEditHandler->Remove(type, name))
				pListCtrl->DeleteItem(i);
			else
				pListCtrl->SetItem(i, 2, _("Pending removal"));
		}
	}

	SetCtrlState();
}
