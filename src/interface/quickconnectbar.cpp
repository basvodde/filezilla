#include "FileZilla.h"
#include "quickconnectbar.h"
#include "recentserverlist.h"
#include "commandqueue.h"
#include "state.h"
#include "Options.h"

BEGIN_EVENT_TABLE(CQuickconnectBar, wxPanel)
EVT_BUTTON(XRCID("ID_QUICKCONNECT_OK"), CQuickconnectBar::OnQuickconnect)
EVT_BUTTON(XRCID("ID_QUICKCONNECT_DROPDOWN"), CQuickconnectBar::OnQuickconnectDropdown)
EVT_MENU(wxID_ANY, CQuickconnectBar::OnMenu)
END_EVENT_TABLE();

CQuickconnectBar::CQuickconnectBar()
{
}

CQuickconnectBar::~CQuickconnectBar()
{
}

bool CQuickconnectBar::Create(wxWindow* pParent, CState* pState)
{
	m_pState = pState;

    if (!wxXmlResource::Get()->LoadPanel(this, pParent, _T("ID_QUICKCONNECTBAR")))
	{
		wxLogError(_("Cannot load Quickconnect bar from resource file"));
		return false;
	}

	XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetMaxLength(5);

	return true;
}

void CQuickconnectBar::OnQuickconnect(wxCommandEvent &event)
{	
	if (!m_pState->m_pEngine)
	{
		wxMessageBox(_("FTP Engine not initialized, can't connect"), _("FileZilla Error"), wxICON_EXCLAMATION);
		return;
	}

	wxString host = XRCCTRL(*this, "ID_QUICKCONNECT_HOST", wxTextCtrl)->GetValue();
	wxString user = XRCCTRL(*this, "ID_QUICKCONNECT_USER", wxTextCtrl)->GetValue();
	wxString pass = XRCCTRL(*this, "ID_QUICKCONNECT_PASS", wxTextCtrl)->GetValue();
	wxString port = XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->GetValue();
	
	long numericPort = 0;
	if (port != _T(""))
		port.ToLong(&numericPort);
	
	CServer server;
	wxString error;

	CServerPath path;
	if (!server.ParseUrl(host, numericPort, user, pass, error, path))
	{
		wxString msg = _("Could not parse server address:");
		msg += _T("\n");
		msg += error;
		wxMessageBox(msg, _("Syntax error"), wxICON_EXCLAMATION);
		return;
	}

	host = server.GetHost();
	ServerProtocol protocol = server.GetProtocol();
	switch (protocol)
	{
	case SFTP:
		host = _T("sftp://") + host;
		break;
	case FTP:
	default:
		// do nothing
		break;
	}
	
	XRCCTRL(*this, "ID_QUICKCONNECT_HOST", wxTextCtrl)->SetValue(host);
	switch (protocol)
	{
	case SFTP:
		if (server.GetPort() != 22)
			XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), numericPort));
		else
			XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetValue(_T(""));
		break;
	case FTP:
	default:
		if (server.GetPort() != 21)
			XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), numericPort));
		else
			XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetValue(_T(""));
		break;
	}
	XRCCTRL(*this, "ID_QUICKCONNECT_USER", wxTextCtrl)->SetValue(server.GetUser());
	XRCCTRL(*this, "ID_QUICKCONNECT_PASS", wxTextCtrl)->SetValue(server.GetPass());

	if (!m_pState->Connect(server, true))
		return;

	CRecentServerList::SetMostRecentServer(server);
}

void CQuickconnectBar::OnQuickconnectDropdown(wxCommandEvent& event)
{
	wxMenu* pMenu = new wxMenu;
	pMenu->Append(0, _("Clear quickconnect bar"));
	pMenu->Append(1, _("Clear history"));
	pMenu->AppendSeparator();

	m_recentServers = CRecentServerList::GetMostRecentServers();
	unsigned int i = 0;
	for (std::list<CServer>::const_iterator iter = m_recentServers.begin();
		iter != m_recentServers.end();
		iter++, i++)
	{
		pMenu->Append(10 + i, iter->FormatServer());
	}

	XRCCTRL(*this, "ID_QUICKCONNECT_DROPDOWN", wxButton)->PopupMenu(pMenu);
	delete pMenu;
	m_recentServers.clear();
}

void CQuickconnectBar::OnMenu(wxCommandEvent& event)
{
	const int id = event.GetId();
	if (!id)
	{
		XRCCTRL(*this, "ID_QUICKCONNECT_HOST", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_QUICKCONNECT_USER", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_QUICKCONNECT_PASS", wxTextCtrl)->SetValue(_T(""));
		return;
	}
	else if (id == 1)
	{
		CRecentServerList::Clear();
	}

	if (id < 10)
		return;

	unsigned int index = id - 10;
	if (index >= m_recentServers.size())
		return;

	std::list<CServer>::const_iterator iter;
	for (iter = m_recentServers.begin(); index; index--, iter++);

	m_pState->Connect(*iter, true);
}
