#include "FileZilla.h"
#include "manual_transfer.h"
#include "local_filesys.h"
#include "auto_ascii_files.h"
#include "state.h"
#include "Options.h"

BEGIN_EVENT_TABLE(CManualTransfer, wxDialogEx)
EVT_TEXT(XRCID("ID_LOCALFILE"), CManualTransfer::OnLocalChanged)
EVT_TEXT(XRCID("ID_REMOTEFILE"), CManualTransfer::OnRemoteChanged)
EVT_BUTTON(XRCID("ID_BROWSE"), CManualTransfer::OnLocalBrowse)
EVT_RADIOBUTTON(XRCID("ID_DOWNLOAD"), CManualTransfer::OnDirection)
EVT_RADIOBUTTON(XRCID("ID_UPLOAD"), CManualTransfer::OnDirection)
EVT_RADIOBUTTON(XRCID("ID_SERVER_CURRENT"), CManualTransfer::OnServerTypeChanged)
EVT_RADIOBUTTON(XRCID("ID_SERVER_SITE"), CManualTransfer::OnServerTypeChanged)
EVT_RADIOBUTTON(XRCID("ID_SERVER_CUSTOM"), CManualTransfer::OnServerTypeChanged)
EVT_BUTTON(XRCID("wxID_OK"), CManualTransfer::OnOK)
END_EVENT_TABLE()

CManualTransfer::CManualTransfer()
{
	m_local_file_exists = false;
	m_pServer = 0;
}

CManualTransfer::~CManualTransfer()
{
	delete m_pServer;
}

void CManualTransfer::Show(wxWindow* pParent, CState* pState)
{
	if (!Load(pParent, _T("ID_MANUALTRANSFER")))
		return;

	m_pState = pState;

	wxChoice *pProtocol = XRCCTRL(*this, "ID_PROTOCOL", wxChoice);
	pProtocol->Append(CServer::GetProtocolName(FTP));
	pProtocol->Append(CServer::GetProtocolName(SFTP));
	pProtocol->Append(CServer::GetProtocolName(FTPS));
	pProtocol->Append(CServer::GetProtocolName(FTPES));

	wxChoice* pChoice = XRCCTRL(*this, "ID_LOGONTYPE", wxChoice);
	wxASSERT(pChoice);
	for (int i = 0; i < LOGONTYPE_MAX; i++)
		pChoice->Append(CServer::GetNameFromLogonType((enum LogonType)i));

	if (m_pState->GetServer())
	{
		m_pServer = new CServer(*m_pState->GetServer());
		XRCCTRL(*this, "ID_SERVER_CURRENT", wxRadioButton)->SetValue(true);
		DisplayServer();
	}
	else
	{
		XRCCTRL(*this, "ID_SERVER_CUSTOM", wxRadioButton)->SetValue(true);
		XRCCTRL(*this, "ID_SERVER_CURRENT", wxRadioButton)->Disable();
	}

	wxString localPath = m_pState->GetLocalDir();
	if (localPath.Last() != CLocalFileSystem::path_separator)
		localPath += CLocalFileSystem::path_separator;
	XRCCTRL(*this, "ID_LOCALFILE", wxTextCtrl)->ChangeValue(localPath);

	XRCCTRL(*this, "ID_REMOTEPATH", wxTextCtrl)->ChangeValue(m_pState->GetRemotePath().GetPath());

	SetControlState();

	ShowModal();
}

void CManualTransfer::SetControlState()
{
	SetServerState();
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
		else if (CAutoAsciiFiles::TransferLocalAsAscii(remote_file, m_pServer ? m_pServer->GetType() : UNIX))
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
		else if (CAutoAsciiFiles::TransferLocalAsAscii(local_file, m_pServer ? m_pServer->GetType() : UNIX))
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

void CManualTransfer::SetServerState()
{
	bool server_enabled = XRCCTRL(*this, "ID_SERVER_CUSTOM", wxRadioButton)->GetValue();
	XRCCTRL(*this, "ID_HOST", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_PORT", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_PROTOCOL", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_LOGONTYPE", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_USER", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_PASS", wxWindow)->Enable(server_enabled);
	XRCCTRL(*this, "ID_ACCOUNT", wxWindow)->Enable(server_enabled);
}

void CManualTransfer::DisplayServer()
{
	if (m_pServer)
	{
		XRCCTRL(*this, "ID_HOST", wxTextCtrl)->ChangeValue(m_pServer->FormatHost(true));
		unsigned int port = m_pServer->GetPort();

		if (port != CServer::GetDefaultPort(m_pServer->GetProtocol()))
			XRCCTRL(*this, "ID_PORT", wxTextCtrl)->ChangeValue(wxString::Format(_T("%d"), port));
		else
			XRCCTRL(*this, "ID_PORT", wxTextCtrl)->ChangeValue(_T(""));

		const wxString& protocolName = CServer::GetProtocolName(m_pServer->GetProtocol());
		if (protocolName != _T(""))
			XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->SetStringSelection(protocolName);
		else
			XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->SetStringSelection(CServer::GetProtocolName(FTP));

		XRCCTRL(*this, "ID_USER", wxTextCtrl)->Enable(m_pServer->GetLogonType() != ANONYMOUS);
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->Enable(m_pServer->GetLogonType() == NORMAL || m_pServer->GetLogonType() == ACCOUNT);
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->Enable(m_pServer->GetLogonType() == ACCOUNT);

		switch (m_pServer->GetLogonType())
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

		XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetValue(m_pServer->GetUser());
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->SetValue(m_pServer->GetAccount());
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->SetValue(m_pServer->GetPass());
	}
	else
	{
		XRCCTRL(*this, "ID_HOST", wxTextCtrl)->ChangeValue(_T(""));
		XRCCTRL(*this, "ID_PORT", wxTextCtrl)->ChangeValue(_T(""));
		XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->SetStringSelection(CServer::GetProtocolName(FTP));
		XRCCTRL(*this, "ID_USER", wxTextCtrl)->Enable(false);
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->Enable(false);
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->Enable(false);
		XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetStringSelection(_("Anonymous"));

		XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->SetValue(_T(""));
	}
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

void CManualTransfer::OnServerTypeChanged(wxCommandEvent& event)
{
	if (event.GetId() == XRCID("ID_SERVER_CURRENT"))
	{
		delete m_pServer;
		if (m_pState->GetServer())
			m_pServer = new CServer(*m_pState->GetServer());
		else
			m_pServer = 0;
	}
	else if (event.GetId() == XRCID("ID_SERVER_SITE"))
	{
		delete m_pServer;
		m_pServer = 0;
	}
	DisplayServer();
	SetServerState();
}

void CManualTransfer::OnOK(wxCommandEvent& event)
{
	if (!UpdateServer())
		return;

	if (!m_pServer)
	{
		wxMessageBox(_("You need to specify a server."), _("Manual transfer"), wxICON_EXCLAMATION);
		return;
	}

	EndModal(wxID_OK);
}

bool CManualTransfer::UpdateServer()
{
	if (!XRCCTRL(*this, "ID_SERVER_CUSTOM", wxRadioButton)->GetValue())
		return true;

	if (!VerifyServer())
		return false;

	CServer server;

	unsigned long port;
	XRCCTRL(*this, "ID_PORT", wxTextCtrl)->GetValue().ToULong(&port);
	wxString host = XRCCTRL(*this, "ID_HOST", wxTextCtrl)->GetValue();
	// SetHost does not accept URL syntax
	if (host[0] == '[')
	{
		host.RemoveLast();
		host = host.Mid(1);
	}
	server.SetHost(host, port);

	const wxString& protocolName = XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->GetStringSelection();
	const enum ServerProtocol protocol = CServer::GetProtocolFromName(protocolName);
	if (protocol != UNKNOWN)
		server.SetProtocol(protocol);
	else
		server.SetProtocol(FTP);

	enum LogonType logon_type = CServer::GetLogonTypeFromName(XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->GetStringSelection());
	server.SetLogonType(logon_type);

	server.SetUser(XRCCTRL(*this, "ID_USER", wxTextCtrl)->GetValue(),
						   XRCCTRL(*this, "ID_PASS", wxTextCtrl)->GetValue());
	server.SetAccount(XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->GetValue());

	delete m_pServer;
	m_pServer = new CServer(server);

	return true;
}

bool CManualTransfer::VerifyServer()
{
	const wxString& host = XRCCTRL(*this, "ID_HOST", wxTextCtrl)->GetValue();
	if (host == _T(""))
	{
		XRCCTRL(*this, "ID_HOST", wxTextCtrl)->SetFocus();
		wxMessageBox(_("You have to enter a hostname."));
		return false;
	}

	enum LogonType logon_type = CServer::GetLogonTypeFromName(XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->GetStringSelection());

	wxString protocolName = XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->GetStringSelection();
	enum ServerProtocol protocol = CServer::GetProtocolFromName(protocolName);
	if (protocol == SFTP &&
		logon_type == ACCOUNT)
	{
		XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetFocus();
		wxMessageBox(_("'Account' logontype not supported by selected protocol"));
		return false;
	}

	if (COptions::Get()->GetDefaultVal(DEFAULT_KIOSKMODE) != 0 &&
		(logon_type == ACCOUNT || logon_type == NORMAL))
	{
		XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetFocus();
		wxMessageBox(_("FileZilla is running in kiosk mode.\n'Normal' and 'Account' logontypes are not available in this mode."));
		return false;
	}

	CServer server;

	// Set selected type
	server.SetLogonType(logon_type);

	if (protocol != UNKNOWN)
		server.SetProtocol(protocol);

	unsigned long port;
	XRCCTRL(*this, "ID_PORT", wxTextCtrl)->GetValue().ToULong(&port);
	CServerPath path;
	wxString error;
	if (!server.ParseUrl(host, port, _T(""), _T(""), error, path))
	{
		XRCCTRL(*this, "ID_HOST", wxTextCtrl)->SetFocus();
		wxMessageBox(error);
		return false;
	}

	XRCCTRL(*this, "ID_HOST", wxTextCtrl)->ChangeValue(server.FormatHost(true));
	XRCCTRL(*this, "ID_PORT", wxTextCtrl)->ChangeValue(wxString::Format(_T("%d"), server.GetPort()));

	protocolName = CServer::GetProtocolName(server.GetProtocol());
	if (protocolName == _T(""))
		CServer::GetProtocolName(FTP);
	XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->SetStringSelection(protocolName);

	// Require username for non-anonymous, non-ask logon type
	const wxString user = XRCCTRL(*this, "ID_USER", wxTextCtrl)->GetValue();
	if (logon_type != ANONYMOUS &&
		logon_type != ASK &&
		logon_type != INTERACTIVE &&
		user == _T(""))
	{
		XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetFocus();
		wxMessageBox(_("You have to specify a user name"));
		return false;
	}

	// The way TinyXML handles blanks, we can't use username of only spaces
	if (user != _T(""))
	{
		bool space_only = true;
		for (unsigned int i = 0; i < user.Len(); i++)
		{
			if (user[i] != ' ')
			{
				space_only = false;
				break;
			}
		}
		if (space_only)
		{
			XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetFocus();
			wxMessageBox(_("Username cannot be a series of spaces"));
			return false;
		}

	}

	// Require account for account logon type
	if (logon_type == ACCOUNT &&
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->GetValue() == _T(""))
	{
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->SetFocus();
		wxMessageBox(_("You have to enter an account name"));
		return false;
	}

	return true;
}
