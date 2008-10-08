#include "FileZilla.h"
#include "manual_transfer.h"
#include "local_filesys.h"
#include "auto_ascii_files.h"
#include "state.h"
#include "Options.h"
#include "sitemanager.h"
#include "queue.h"
#include "QueueView.h"

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
EVT_BUTTON(XRCID("ID_SERVER_SITE_SELECT"), CManualTransfer::OnSelectSite)
EVT_MENU(wxID_ANY, CManualTransfer::OnSelectedSite)
EVT_CHOICE(XRCID("ID_LOGONTYPE"), CManualTransfer::OnLogontypeSelChanged)
END_EVENT_TABLE()

CManualTransfer::CManualTransfer(CQueueView* pQueueView)
{
	m_pQueueView = pQueueView;

	m_local_file_exists = false;
	m_pServer = 0;
	m_pLastSite = 0;
}

CManualTransfer::~CManualTransfer()
{
	delete m_pServer;
	delete m_pLastSite;
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
		DisplayServer();
	}

	wxString localPath = m_pState->GetLocalDir();
	if (localPath.Last() != CLocalFileSystem::path_separator)
		localPath += CLocalFileSystem::path_separator;
	XRCCTRL(*this, "ID_LOCALFILE", wxTextCtrl)->ChangeValue(localPath);

	XRCCTRL(*this, "ID_REMOTEPATH", wxTextCtrl)->ChangeValue(m_pState->GetRemotePath().GetPath());

	SetControlState();

	switch(COptions::Get()->GetOptionVal(OPTION_ASCIIBINARY))
	{
	case 1:
		XRCCTRL(*this, "ID_TYPE_ASCII", wxRadioButton)->SetValue(true);
		break;
	case 2:
		XRCCTRL(*this, "ID_TYPE_BINARY", wxRadioButton)->SetValue(true);
		break;
	default:
		XRCCTRL(*this, "ID_TYPE_AUTO", wxRadioButton)->SetValue(true);
		break;
	}

	ShowModal();
}

void CManualTransfer::SetControlState()
{
	SetServerState();
	SetAutoAsciiState();

	XRCCTRL(*this, "ID_SERVER_SITE_SELECT", wxButton)->Enable(XRCCTRL(*this, "ID_SERVER_SITE", wxRadioButton)->GetValue());
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

	wxString logon_type = XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->GetStringSelection();
	XRCCTRL(*this, "ID_USER", wxTextCtrl)->Enable(server_enabled && logon_type != _("Anonymous"));
	XRCCTRL(*this, "ID_PASS", wxTextCtrl)->Enable(server_enabled && (logon_type == _("Normal") || logon_type == _("Account")));
	XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->Enable(server_enabled && logon_type == _("Account"));
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

		XRCCTRL(*this, "ID_USER", wxTextCtrl)->ChangeValue(m_pServer->GetUser());
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->ChangeValue(m_pServer->GetAccount());
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->ChangeValue(m_pServer->GetPass());
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

		XRCCTRL(*this, "ID_USER", wxTextCtrl)->ChangeValue(_T(""));
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->ChangeValue(_T(""));
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->ChangeValue(_T(""));
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
		if (m_pLastSite)
			m_pServer = new CServer(*m_pLastSite);
		else
			m_pServer = 0;

	}
	XRCCTRL(*this, "ID_SERVER_SITE_SELECT", wxButton)->Enable(event.GetId() == XRCID("ID_SERVER_SITE"));
	DisplayServer();
	SetServerState();
}

void CManualTransfer::OnOK(wxCommandEvent& event)
{
	if (!UpdateServer())
		return;

	bool download = XRCCTRL(*this, "ID_DOWNLOAD", wxRadioButton)->GetValue();

	bool start = XRCCTRL(*this, "ID_START", wxCheckBox)->GetValue();

	if (!m_pServer)
	{
		wxMessageBox(_("You need to specify a server."), _("Manual transfer"), wxICON_EXCLAMATION);
		return;
	}

	wxString local_file = XRCCTRL(*this, "ID_LOCALFILE", wxTextCtrl)->GetValue();
	if (local_file == _T(""))
	{
		wxMessageBox(_("You need to specify a local file."), _("Manual transfer"), wxICON_EXCLAMATION);
		return;
	}

	CLocalFileSystem::local_fileType type = CLocalFileSystem::GetFileType(local_file);
	if (type == CLocalFileSystem::dir)
	{
		wxMessageBox(_("Local file is a directory instead of a regular file."), _("Manual transfer"), wxICON_EXCLAMATION);
		return;
	}
	if (!download && type != CLocalFileSystem::file && start)
	{
		wxMessageBox(_("Local file does not exist."), _("Manual transfer"), wxICON_EXCLAMATION);
		return;
	}

	wxString remote_file = XRCCTRL(*this, "ID_REMOTEFILE", wxTextCtrl)->GetValue();

	if (remote_file == _T(""))
	{
		wxMessageBox(_("You need to specify a remote file."), _("Manual transfer"), wxICON_EXCLAMATION);
		return;
	}

	wxString remote_path_str = XRCCTRL(*this, "ID_REMOTEPATH", wxTextCtrl)->GetValue();
	if (remote_path_str == _T(""))
	{
		wxMessageBox(_("You need to specify a remote path."), _("Manual transfer"), wxICON_EXCLAMATION);
		return;
	}

	CServerPath path(remote_path_str, m_pServer->GetType());
	if (path.IsEmpty())
	{
		wxMessageBox(_("Remote path could not be parsed."), _("Manual transfer"), wxICON_EXCLAMATION);
		return;
	}

	int old_data_type = COptions::Get()->GetOptionVal(OPTION_ASCIIBINARY);
	
	// Set data type for the file to add
	if (XRCCTRL(*this, "ID_TYPE_ASCII", wxRadioButton)->GetValue())
		COptions::Get()->SetOption(OPTION_ASCIIBINARY, 1);
	else if (XRCCTRL(*this, "ID_TYPE_BINARY", wxRadioButton)->GetValue())
		COptions::Get()->SetOption(OPTION_ASCIIBINARY, 2);
	else
		COptions::Get()->SetOption(OPTION_ASCIIBINARY, 0);

	m_pQueueView->QueueFile(!start, download, local_file, remote_file, path, *m_pServer, -1);

	// Restore old data type
	COptions::Get()->SetOption(OPTION_ASCIIBINARY, old_data_type);

	m_pQueueView->QueueFile_Finish(start);

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

void CManualTransfer::OnSelectSite(wxCommandEvent& event)
{
	wxMenu *pMenu = CSiteManager::GetSitesMenu();
	if (!pMenu)
		return;

	PopupMenu(pMenu);

	delete pMenu;
}

void CManualTransfer::OnSelectedSite(wxCommandEvent& event)
{
	CSiteManagerItemData* pData = CSiteManager::GetSiteById(event.GetId());
	if (!pData)
		return;

	delete m_pServer;
	m_pServer = new CServer(pData->m_server);
	delete m_pLastSite;
	m_pLastSite = new CServer(pData->m_server);

	XRCCTRL(*this, "ID_SERVER_SITE_SERVER", wxStaticText)->SetLabel(m_pServer->GetName());

	delete pData;

	DisplayServer();
}

void CManualTransfer::OnLogontypeSelChanged(wxCommandEvent& event)
{
	XRCCTRL(*this, "ID_USER", wxTextCtrl)->Enable(event.GetString() != _("Anonymous"));
	XRCCTRL(*this, "ID_PASS", wxTextCtrl)->Enable(event.GetString() == _("Normal") || event.GetString() == _("Account"));
	XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->Enable(event.GetString() == _("Account"));
}
