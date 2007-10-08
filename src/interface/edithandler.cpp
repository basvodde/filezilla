#include "FileZilla.h"
#include "edithandler.h"
#include "dialogex.h"
#include "filezillaapp.h"
#include "queue.h"
#include "Options.h"

// Defined in optionspage_edit.cpp
bool UnquoteCommand(wxString& command, wxString& arguments);

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
			// TODO
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
//TODO
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
#endif

		RemoveAll(true);
		wxRmdir(m_localDir);
	}

	m_pEditHandler = 0;
	delete this;
}

enum CEditHandler::fileState CEditHandler::GetFileState(const wxString& fileName)
{
	std::list<t_fileData>::const_iterator iter = GetFile(fileName);
	if (iter == m_fileDataList.end())
		return unknown;
	
	return iter->state;
}

int CEditHandler::GetFileCount(enum CEditHandler::fileState state) const
{
	if (state == unknown)
		return m_fileDataList.size();

	int count = 0;
	for (std::list<t_fileData>::const_iterator iter = m_fileDataList.begin(); iter != m_fileDataList.end(); iter++)
	{
		if (iter->state == state)
			count++;
	}

	return count;
}

bool CEditHandler::AddFile(const wxString& fileName, const CServerPath& remotePath, const CServer& server)
{
	wxASSERT(GetFileState(fileName) == unknown);
	if (GetFileState(fileName) != unknown)
		return false;

	t_fileData data;
	data.name = fileName;
	data.state = download;
	data.remotePath = remotePath;
	data.server = server;
	m_fileDataList.push_back(data);

	return true;
}

bool CEditHandler::Remove(const wxString& fileName)
{
	std::list<t_fileData>::iterator iter = GetFile(fileName);
	if (iter == m_fileDataList.end())
		return true;

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

	m_fileDataList.erase(iter);

	return true;
}

bool CEditHandler::RemoveAll(bool force)
{
	std::list<t_fileData> keep;

	for (std::list<t_fileData>::iterator iter = m_fileDataList.begin(); iter != m_fileDataList.end(); iter++)
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

	m_fileDataList = keep;

	return m_fileDataList.empty();
}

std::list<CEditHandler::t_fileData>::iterator CEditHandler::GetFile(const wxString& fileName)
{
	std::list<t_fileData>::iterator iter;
	for (iter = m_fileDataList.begin(); iter != m_fileDataList.end(); iter++)
	{
		if (iter->name == fileName)
			break;
	}

	return iter;
}

std::list<CEditHandler::t_fileData>::const_iterator CEditHandler::GetFile(const wxString& fileName) const
{
	std::list<t_fileData>::const_iterator iter;
	for (iter = m_fileDataList.begin(); iter != m_fileDataList.end(); iter++)
	{
		if (iter->name == fileName)
			break;
	}

	return iter;
}

void CEditHandler::FinishTransfer(bool successful, const wxString& fileName)
{
	std::list<t_fileData>::iterator iter = GetFile(fileName);
	if (iter == m_fileDataList.end())
		return;

	wxASSERT(iter->state == download || iter->state == upload || iter->state == upload_and_remove);

	switch (iter->state)
	{
	case upload_and_remove:
		if (wxFileName::FileExists(m_localDir + fileName) && !wxRemoveFile(m_localDir + fileName))
			iter->state = removing;
		else
			m_fileDataList.erase(iter);
		break;
	case upload:
		if (wxFileName::FileExists(m_localDir + fileName))
			iter->state = edit;
		else
			m_fileDataList.erase(iter);
		break;
	case download:
		if (wxFileName::FileExists(m_localDir + fileName))
		{
			iter->state = edit;
			if (StartEditing(*iter))
				break;
		}
		if (wxFileName::FileExists(m_localDir + fileName) && !wxRemoveFile(m_localDir + fileName))
			iter->state = removing;
		else
			m_fileDataList.erase(iter);
		break;
	default:
		return;
	}

	SetTimerState();
}

bool CEditHandler::StartEditing(t_fileData& data)
{
	wxASSERT(data.state == edit);

	wxFileName fn(m_localDir, data.name);
	data.modificationTime = fn.GetModificationTime();

	wxString cmd = GetOpenCommand(data.name);
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

checkmodifications:
	for (std::list<t_fileData>::iterator iter = m_fileDataList.begin(); iter != m_fileDataList.end(); iter++)
	{
		if (iter->state != edit)
			continue;

		wxFileName fn(m_localDir, iter->name);
		if (!fn.FileExists())
		{
			m_fileDataList.erase(iter);
			
			// Evil goto. Imo the next C++ standard needs a comefrom keyword.
			goto checkmodifications;
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
		
		dlg.SetLabel(XRCID("ID_FILENAME"), iter->name);
		
		int res = dlg.ShowModal();

		const bool remove = XRCCTRL(dlg, "ID_DELETE", wxCheckBox)->IsChecked();
		
		if (res == wxID_YES)
		{
			iter->state = remove ? upload_and_remove : upload;
			iter->modificationTime = mtime;

			wxASSERT(m_pQueue);
			wxULongLong size = fn.GetSize();
			m_pQueue->QueueFile(false, false, m_localDir + iter->name, iter->name, iter->remotePath, iter->server, wxLongLong(size.GetHi(), size.GetLo()), true);
		}
		else
		{
			if (!fn.FileExists() || wxRemoveFile(fn.GetFullPath()))
			{
				m_fileDataList.erase(iter);
				goto checkmodifications;
			}

			iter->state = removing;
		}
	}

	SetTimerState();

	insideCheckForModifications = false;
}

void CEditHandler::OnTimerEvent(wxTimerEvent& event)
{
	CheckForModifications();
}

void CEditHandler::SetTimerState()
{
	bool editing = false;
	for (std::list<t_fileData>::const_iterator iter = m_fileDataList.begin(); iter != m_fileDataList.end(); iter++)
	{
		if (iter->state == edit)
			editing = true;
	}
	if (m_timer.IsRunning())
	{
		if (!editing)
			m_timer.Stop();
	}
	else if (editing)
		m_timer.Start(15000);
}

bool CEditHandler::CanOpen(const wxString& fileName, bool &dangerous)
{
	wxString command = GetOpenCommand(fileName);
	if (command == _T(""))
		return false;

	wxFileName fn(m_localDir, fileName);
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

	if (!wxFileName::FileExists(editor))
		return _T("");

	return command + _T(" \"") + m_localDir + file + _T("\"");
}

wxString CEditHandler::GetSystemOpenCommand(const wxString& file)
{
	wxFileName fn(m_localDir, file);

	const wxString& ext = fn.GetExt();
	if (ext == _T(""))
		return _T("");

	wxFileType* pType = wxTheMimeTypesManager->GetFileTypeFromExtension(ext);
	if (!pType)
		return _T("");

	wxString cmd;
	if (!pType->GetOpenCommand(&cmd, wxFileType::MessageParameters(m_localDir + file)))
	{
		delete pType;
		return _T("");
	}
	delete pType;

	return cmd;
}

wxString CEditHandler::GetCustomOpenCommand(const wxString& file)
{
	wxFileName fn(m_localDir, file);

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

		if (!wxFileName::FileExists(command))
			return _T("");

		return command + _T(" \"") + fn.GetFullPath() + _T("\"");
	}

	return _T("");
}
