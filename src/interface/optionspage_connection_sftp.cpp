#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_connection_sftp.h"
#include "filezillaapp.h"
#include "inputdialog.h"
#include <wx/tokenzr.h>

BEGIN_EVENT_TABLE(COptionsPageConnectionSFTP, COptionsPage)
EVT_END_PROCESS(wxID_ANY, COptionsPageConnectionSFTP::OnEndProcess)
EVT_BUTTON(XRCID("ID_ADDKEY"), COptionsPageConnectionSFTP::OnAdd)
EVT_BUTTON(XRCID("ID_REMOVEKEY"), COptionsPageConnectionSFTP::OnRemove)
EVT_LIST_ITEM_SELECTED(wxID_ANY, COptionsPageConnectionSFTP::OnSelChanged)
EVT_LIST_ITEM_DESELECTED(wxID_ANY, COptionsPageConnectionSFTP::OnSelChanged)
END_EVENT_TABLE()

COptionsPageConnectionSFTP::COptionsPageConnectionSFTP()
{
	m_pProcess = 0;
	m_initialized = 0;
}

COptionsPageConnectionSFTP::~COptionsPageConnectionSFTP()
{
	if (m_pProcess)
	{
		m_pProcess->CloseOutput();
		m_pProcess->Detach();
		m_pProcess = 0;
	}
}

bool COptionsPageConnectionSFTP::LoadPage()
{
	wxListCtrl* pKeys = XRCCTRL(*this, "ID_KEYS", wxListCtrl);
	if (!pKeys)
		return false;
	pKeys->InsertColumn(0, _("Filename"), wxLIST_FORMAT_LEFT, 150);
	pKeys->InsertColumn(1, _("Comment"), wxLIST_FORMAT_LEFT, 100);
	pKeys->InsertColumn(2, _("Data"), wxLIST_FORMAT_LEFT, 350);

	wxString keyFiles = m_pOptions->GetOption(OPTION_SFTP_KEYFILES);
	wxStringTokenizer tokens(keyFiles, _T("\n"), wxTOKEN_DEFAULT);
	while (tokens.HasMoreTokens())
		AddKey(tokens.GetNextToken(), true);

	bool failure = false;

	SetCtrlState();

	return !failure;
}

bool COptionsPageConnectionSFTP::SavePage()
{
	// Don't save keys on process error
	if (!m_initialized || m_pProcess)
	{
		wxString keyFiles;
		wxListCtrl* pKeys = XRCCTRL(*this, "ID_KEYS", wxListCtrl);
		for (int i = 0; i < pKeys->GetItemCount(); i++)
		{
			if (keyFiles != _T(""))
				keyFiles += _T("\n");
			keyFiles += pKeys->GetItemText(i);
		}
		m_pOptions->SetOption(OPTION_SFTP_KEYFILES, keyFiles);
	}

	if (m_pProcess)
	{
		m_pProcess->CloseOutput();
		m_pProcess->Detach();
		m_pProcess = 0;
	}

	return true;
}

bool COptionsPageConnectionSFTP::LoadProcess()
{
	if (m_initialized)
		return m_pProcess != 0;
	
	m_initialized = true;

	wxString executable = m_pOptions->GetOption(OPTION_FZSFTP_EXECUTABLE);
	//FIXME TODO BROKEN
	executable.Replace(_T("fzsftp"), _T("fzputtygen"));

	m_pProcess = wxProcess::Open(executable);
	if (!m_pProcess)
	{
		wxMessageBox(_("fzputtygen could not be started.\nPlease make sure this executable exists in the same directory as the main FileZilla executable."), _("Error starting program"), wxICON_EXCLAMATION);
		return false;
	}

	m_pProcess->Redirect();

	return true;
}

bool COptionsPageConnectionSFTP::Send(const wxString& cmd)
{
	if (!m_pProcess)
		return false;

	wxWX2MBbuf buf = (cmd + _T("\n")).mb_str();
	const size_t len = strlen(buf);

	wxOutputStream* stream = m_pProcess->GetOutputStream();
	stream->Write((const char*)buf, len);

	if (stream->GetLastError() != wxSTREAM_NO_ERROR || stream->LastWrite() != len)
	{
		wxMessageBox(_("Could not send command to fzputtygen."), _("Command failed"), wxICON_EXCLAMATION);
		return false;
	}

	return true;
}

enum COptionsPageConnectionSFTP::ReplyCode COptionsPageConnectionSFTP::GetReply(wxString& reply)
{
	if (!m_pProcess)
		return failure;
	wxInputStream *pStream = m_pProcess->GetInputStream();
	if (!pStream)
	{
		wxMessageBox(_("Could not get reply from fzputtygen."), _("Command failed"), wxICON_EXCLAMATION);
		return failure;
	}

	char buffer[100];

	wxString input;

	while (true)
	{
		int pos = input.Find('\n');
		if (pos == wxNOT_FOUND)
		{
			pStream->Read(buffer, 99);
			int read;
			if (pStream->Eof() || !(read = pStream->LastRead()))
			{
				wxMessageBox(_("Could not get replyfrom fzputtygen."), _("Command failed"), wxICON_EXCLAMATION);
				return failure;
			}
			buffer[read] = 0;

			// Should only ever return ASCII strings so this is ok
			input += wxString(buffer, wxConvUTF8);

			pos = input.Find('\n');
			if (pos == wxNOT_FOUND)
				continue;
		}

		int pos2;
		if (pos && input[pos - 1] == '\r')
			pos2 = pos - 1;
		else
			pos2 = pos;
		if (!pos2)
		{
			input = input.Mid(pos + 1);
			continue;
		}
		wxChar c = input[0];
		
		reply = input.Mid(1, pos2 - 1);
		input = input.Mid(pos + 1);

		if (c == '0' || c == '1')
			return success;
		else if (c == '2')
			return error;
		// Ignore others
	}
}

bool COptionsPageConnectionSFTP::LoadKeyFile(wxString& keyFile, bool silent, wxString& comment, wxString& data)
{
	if (!LoadProcess())
		return false;

	// Get keytype
	if (!Send(_T("file " + keyFile)))
		return false;
	wxString reply;
	enum ReplyCode code = GetReply(reply);
	if (code == failure)
		return false;
	if (code == error || (reply != _T("0") && reply != _T("1")))
	{
		if (!silent)
		{
			const wxString msg = wxString::Format(_("The file '%s' could not be loaded or does not contain a private key."), keyFile.c_str());
			wxMessageBox(msg, _("Could not load keyfile"), wxICON_EXCLAMATION);
		}
		return false;
	}

	bool needs_conversion;
	if (reply == _T("1"))
	{
		if (silent)
			return false;

		needs_conversion = true;
	}
	else
		needs_conversion = false;

	// Check if file is encrypted
	if (!Send(_T("encrypted")))
		return false;
	code = GetReply(reply);
	if (code != success)
	{
		wxASSERT(code != error);
		return false;
	}
	bool encrypted;
	if (reply == _T("1"))
	{
		if (silent)
			return false;
		encrypted = true;
	}
	else
		encrypted = false;

	if (encrypted || needs_conversion)
	{
		wxASSERT(!silent);

		wxString msg;
		if (needs_conversion)
		{
			if (!encrypted)
				msg = wxString::Format(_("The file '%s' is not in a format supported by FileZilla.\nWould you like to convert it into a supported format?"), keyFile.c_str());
			else
				msg = wxString::Format(_("The file '%s' is not in a format supported by FileZilla.\nThe file is also password protected. Password protected keyfiles are not supported by FileZilla yet.\nWould you like to convert it into a supported, unprotected format?"), keyFile.c_str());
		}
		else if (encrypted)
			msg = wxString::Format(_("The file '%s' is password protected. Password protected keyfiles are not supported by FileZilla yet.\nWould you like to convert it into an unprotected file?"), keyFile.c_str());

		int res = wxMessageBox(msg, _("Convert keyfile"), wxICON_QUESTION | wxYES_NO);
		if (res != wxYES)
			return false;
		
		if (encrypted)
		{
			wxString msg = wxString::Format(_("Enter the password for the file '%s'.\nPlease note that the converted file will not be password protected."), keyFile.c_str());
			CInputDialog dlg;
			if (!dlg.Create(this, _("Password required"), msg))
				return false;
			dlg.SetPasswordMode(true);
			if (dlg.ShowModal() != wxID_OK)
				return false;
			if (!Send(_T("password " + dlg.GetValue())))
				return false;
			if (GetReply(reply) != success)
				return false;
		}

		if (!Send(_T("load")))
			return false;
		code = GetReply(reply);
		if (code == failure)
			return false;
		if (code != success)
		{
			wxString msg = wxString::Format(_("Failed to load private key: %s"), reply.c_str());
			wxMessageBox(msg, _("Could not load private key"), wxICON_EXCLAMATION);
			return false;
		}

		wxFileDialog dlg(this, _("Select filename for converted keyfile"), _T(""), _T(""), _T("PuTTY private key files (*.ppk)|*.ppk"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (dlg.ShowModal() != wxID_OK)
			return false;

		wxString newName = dlg.GetPath();
		if (newName == _T(""))
			return false;

		if (newName == keyFile)
		{
			// Not actually a requirement by fzputtygen, but be on the safe side. We don't want the user to lose his keys.
			wxMessageBox(_("Source and target file may not be the same"), _("Could not convert private key"), wxICON_EXCLAMATION);
			return false;
		}

		Send(_T("write ") + newName);
		code = GetReply(reply);
		if (code == failure)
			return false;
		if (code != success)
		{
			wxMessageBox(wxString::Format(_("Could not write keyfile: %s"), reply.c_str()), _("Could not convert private key"), wxICON_EXCLAMATION);
			return false;
		}
		keyFile = newName;
	}
	else
	{
		if (!Send(_T("load")))
			return false;
		code = GetReply(reply);
		if (code != success)
			return false;
	}

	Send(_T("comment"));
	code = GetReply(comment);
	if (code != success)
		return false;

	Send(_T("data"));
	code = GetReply(data);
	if (code != success)
		return false;

	return true;
}

void COptionsPageConnectionSFTP::OnEndProcess(wxProcessEvent& event)
{
	delete m_pProcess;
	m_pProcess = 0;
}

void COptionsPageConnectionSFTP::OnAdd(wxCommandEvent& event)
{
	wxFileDialog dlg(this, _("Select file containing private key"), _T(""), _T(""), _T("*.*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() != wxID_OK)
		return;

	const wxString file = dlg.GetPath();

	AddKey(dlg.GetPath(), false);
}

void COptionsPageConnectionSFTP::OnRemove(wxCommandEvent& event)
{
	wxListCtrl* pKeys = XRCCTRL(*this, "ID_KEYS", wxListCtrl);
	
	int index = pKeys->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (index == -1)
		return;

	pKeys->DeleteItem(index);
}

bool COptionsPageConnectionSFTP::AddKey(wxString keyFile, bool silent)
{
	wxString comment, data;
	if (!LoadKeyFile(keyFile, silent, comment, data))
		return false;

	if (KeyFileExists(keyFile))
	{
		if (!silent)
			wxMessageBox(_("Selected file is already loaded"), _("Cannot load keyfile"), wxICON_INFORMATION);
		return false;
	}

	wxListCtrl* pKeys = XRCCTRL(*this, "ID_KEYS", wxListCtrl);
	int index = pKeys->InsertItem(pKeys->GetItemCount(), keyFile);
	pKeys->SetItem(index, 1, comment);
	pKeys->SetItem(index, 2, data);

	return true;
}

bool COptionsPageConnectionSFTP::KeyFileExists(const wxString& keyFile)
{
	wxListCtrl* pKeys = XRCCTRL(*this, "ID_KEYS", wxListCtrl);
	for (int i = 0; i < pKeys->GetItemCount(); i++)
	{
		if (pKeys->GetItemText(i) == keyFile)
			return true;
	}
	return false;
}

void COptionsPageConnectionSFTP::SetCtrlState()
{
	wxListCtrl* pKeys = XRCCTRL(*this, "ID_KEYS", wxListCtrl);
	
	int index = pKeys->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	XRCCTRL(*this, "ID_REMOVEKEY", wxButton)->Enable(index != -1);
	return;
}

void COptionsPageConnectionSFTP::OnSelChanged(wxListEvent& event)
{
	SetCtrlState();
}
