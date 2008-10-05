#include "FileZilla.h"
#include "manual_transfer.h"
#include "local_filesys.h"
#include "auto_ascii_files.h"

BEGIN_EVENT_TABLE(CManualTransfer, wxDialogEx)
EVT_TEXT(XRCID("ID_LOCALFILE"), CManualTransfer::OnLocalChanged)
EVT_TEXT(XRCID("ID_REMOTEFILE"), CManualTransfer::OnRemoteChanged)
EVT_BUTTON(XRCID("ID_BROWSE"), CManualTransfer::OnLocalBrowse)
EVT_RADIOBUTTON(XRCID("ID_DOWNLOAD"), CManualTransfer::OnDirection)
EVT_RADIOBUTTON(XRCID("ID_UPLOAD"), CManualTransfer::OnDirection)
END_EVENT_TABLE()

CManualTransfer::CManualTransfer()
{
	m_local_file_exists = false;
}

void CManualTransfer::Show(wxWindow* pParent, wxString localPath, const CServerPath& remotePath, const CServer* pServer)
{
	if (!Load(pParent, _T("ID_MANUALTRANSFER")))
		return;

	wxChoice *pProtocol = XRCCTRL(*this, "ID_PROTOCOL", wxChoice);
	pProtocol->Append(CServer::GetProtocolName(FTP));
	pProtocol->Append(CServer::GetProtocolName(SFTP));
	pProtocol->Append(CServer::GetProtocolName(FTPS));
	pProtocol->Append(CServer::GetProtocolName(FTPES));

	if (pServer)
	{
		XRCCTRL(*this, "ID_SERVER_CURRENT", wxRadioButton)->SetValue(true);
		DisplayServer(*pServer);
	}
	else
		XRCCTRL(*this, "ID_SERVER_CUSTOM", wxRadioButton)->SetValue(true);

	if (localPath.Last() != CLocalFileSystem::path_separator)
		localPath += CLocalFileSystem::path_separator;
	XRCCTRL(*this, "ID_LOCALFILE", wxTextCtrl)->ChangeValue(localPath);

	XRCCTRL(*this, "ID_REMOTEPATH", wxTextCtrl)->ChangeValue(remotePath.GetPath());

	SetControlState();

	ShowModal();
}

void CManualTransfer::SetControlState()
{
	// Server control state
	bool server_enabled = XRCCTRL(*this, "ID_SERVER_CUSTOM", wxRadioButton)->GetValue();
	XRCCTRL(*this, "ID_HOST", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_PORT", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_PROTOCOL", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_LOGONTYPE", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_USER", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_PASS", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_ACCOUNT", wxWindow)->Enable(server_enabled);

	// Auto ASCII state
	SetAutoAsciiState();
}

void CManualTransfer::SetAutoAsciiState()
{
	if (XRCCTRL(*this, "ID_DOWNLOAD", wxRadioButton)->GetValue())
	{
		wxString remote_file = XRCCTRL(*this, "ID_REMOTEFILE", wxTextCtrl)->GetValue();
		if (remote_file == _T(""))
		{
			XRCCTRL(*this, "ID_TYPE_AUTO_ASCII", wxStaticText)->Hide();
			XRCCTRL(*this, "ID_TYPE_AUTO_BINARY", wxStaticText)->Hide();
		}
		else if (CAutoAsciiFiles::TransferLocalAsAscii(remote_file, m_server.GetType()))
		{
			XRCCTRL(*this, "ID_TYPE_AUTO_ASCII", wxStaticText)->Show();
			XRCCTRL(*this, "ID_TYPE_AUTO_BINARY", wxStaticText)->Hide();
		}
		else
		{
			XRCCTRL(*this, "ID_TYPE_AUTO_ASCII", wxStaticText)->Hide();
			XRCCTRL(*this, "ID_TYPE_AUTO_BINARY", wxStaticText)->Show();
		}
	}
	else
	{
		wxString local_file = XRCCTRL(*this, "ID_LOCALFILE", wxTextCtrl)->GetValue();
		if (!m_local_file_exists)
		{
			XRCCTRL(*this, "ID_TYPE_AUTO_ASCII", wxStaticText)->Hide();
			XRCCTRL(*this, "ID_TYPE_AUTO_BINARY", wxStaticText)->Hide();
		}
		else if (CAutoAsciiFiles::TransferLocalAsAscii(local_file, m_server.GetType()))
		{
			XRCCTRL(*this, "ID_TYPE_AUTO_ASCII", wxStaticText)->Show();
			XRCCTRL(*this, "ID_TYPE_AUTO_BINARY", wxStaticText)->Hide();
		}
		else
		{
			XRCCTRL(*this, "ID_TYPE_AUTO_ASCII", wxStaticText)->Hide();
			XRCCTRL(*this, "ID_TYPE_AUTO_BINARY", wxStaticText)->Show();
		}
	}
	XRCCTRL(*this, "ID_TYPE_AUTO_ASCII", wxStaticText)->GetContainingSizer()->Layout();
}

void CManualTransfer::DisplayServer(const CServer& server)
{
	XRCCTRL(*this, "ID_HOST", wxTextCtrl)->SetValue(server.FormatHost(true));
	unsigned int port = server.GetPort();

	if (port != CServer::GetDefaultPort(server.GetProtocol()))
		XRCCTRL(*this, "ID_PORT", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), port));
	else
		XRCCTRL(*this, "ID_PORT", wxTextCtrl)->SetValue(_T(""));

	const wxString& protocolName = CServer::GetProtocolName(server.GetProtocol());
	if (protocolName != _T(""))
		XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->SetStringSelection(protocolName);
	else
		XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->SetStringSelection(CServer::GetProtocolName(FTP));

	XRCCTRL(*this, "ID_USER", wxTextCtrl)->Enable(server.GetLogonType() != ANONYMOUS);
	XRCCTRL(*this, "ID_PASS", wxTextCtrl)->Enable(server.GetLogonType() == NORMAL || server.GetLogonType() == ACCOUNT);
	XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->Enable(server.GetLogonType() == ACCOUNT);

	switch (server.GetLogonType())
	{
	case NORMAL:
		XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetStringSelection(_("Normal"));
		break;
	case ASK:
		XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetStringSelection(_("Ask for password"));
		break;
	case INTERACTIVE:
		XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetStringSelection(_("Interactive"));
		break;
	case ACCOUNT:
		XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetStringSelection(_("Account"));
		break;
	default:
		XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetStringSelection(_("Anonymous"));
		break;
	}

	XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetValue(server.GetUser());
	XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->SetValue(server.GetAccount());
	XRCCTRL(*this, "ID_PASS", wxTextCtrl)->SetValue(server.GetPass());
}

void CManualTransfer::OnLocalChanged(wxCommandEvent& event)
{
	if (XRCCTRL(*this, "ID_DOWNLOAD", wxRadioButton)->GetValue())
		return;

	wxString file = XRCCTRL(*this, "ID_LOCALFILE", wxTextCtrl)->GetValue();

	m_local_file_exists = CLocalFileSystem::GetFileType(file) == CLocalFileSystem::file;

	SetAutoAsciiState();
}

void CManualTransfer::OnRemoteChanged(wxCommandEvent& event)
{
	SetAutoAsciiState();
}

void CManualTransfer::OnLocalBrowse(wxCommandEvent& event)
{
	int flags;
	wxString title;
	if (XRCCTRL(*this, "ID_DOWNLOAD", wxRadioButton)->GetValue())
	{
		flags = wxFD_SAVE | wxFD_OVERWRITE_PROMPT;
		title = _("Select target filename");
	}
	else
	{
		flags = wxFD_OPEN | wxFD_FILE_MUST_EXIST;
		title = _T("Select file to upload");
	}

	wxFileDialog dlg(this, title, _T(""), _T(""), _T("*.*"), flags);
	int res = dlg.ShowModal();

	if (res != wxID_OK)
		return;

	// SetValue on purpose
	XRCCTRL(*this, "ID_LOCALFILE", wxTextCtrl)->SetValue(dlg.GetPath());
}

void CManualTransfer::OnDirection(wxCommandEvent& event)
{
	if (XRCCTRL(*this, "ID_DOWNLOAD", wxRadioButton)->GetValue())
		SetAutoAsciiState();
	else
	{
		// Need to check for file existence
		OnLocalChanged(event);
	}
}
