#include "FileZilla.h"
#include "Mainfrm.h"
#include "quickconnectbar.h"
#include "recentserverlist.h"
#include "commandqueue.h"
#include "state.h"
#include "options.h"

BEGIN_EVENT_TABLE(CQuickconnectBar, wxPanel)
EVT_BUTTON(XRCID("ID_QUICKCONNECT_OK"), CQuickconnectBar::OnQuickconnect)
EVT_BUTTON(XRCID("ID_QUICKCONNECT_DROPDOWN"), CQuickconnectBar::OnQuickconnectDropdown)
END_EVENT_TABLE();

CQuickconnectBar::CQuickconnectBar()
{
	m_pMainFrame = 0;
}

CQuickconnectBar::~CQuickconnectBar()
{
}

bool CQuickconnectBar::Create(CMainFrame* pMainFrame)
{
	m_pMainFrame = pMainFrame;

    if (!wxXmlResource::Get()->LoadPanel(this, pMainFrame, _T("ID_QUICKCONNECTBAR")))
	{
		wxLogError(_("Cannot load Quickconnect bar from resource file"));
		return false;
	}

	XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetMaxLength(5);

	return true;
}

void CQuickconnectBar::OnQuickconnect(wxCommandEvent &event)
{	
	if (!m_pMainFrame->m_pEngine)
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
		if (numericPort != 22)
			XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), numericPort));
		else
			XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetValue(_T(""));
		break;
	case FTP:
	default:
		if (numericPort != 21)
			XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), numericPort));
		else
			XRCCTRL(*this, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetValue(_T(""));
		break;
	}
	XRCCTRL(*this, "ID_QUICKCONNECT_USER", wxTextCtrl)->SetValue(server.GetUser());
	XRCCTRL(*this, "ID_QUICKCONNECT_PASS", wxTextCtrl)->SetValue(server.GetPass());

	if (m_pMainFrame->m_pEngine->IsConnected() || m_pMainFrame->m_pEngine->IsBusy() || !m_pMainFrame->m_pCommandQueue->Idle())
	{
		if (wxMessageBox(_("Break current connection?"), _T("FileZilla"), wxYES_NO | wxICON_QUESTION) != wxYES)
			return;
		m_pMainFrame->m_pCommandQueue->Cancel();
	}

	m_pMainFrame->GetState()->SetServer(&server);
	m_pMainFrame->m_pCommandQueue->ProcessCommand(new CConnectCommand(server));
	m_pMainFrame->m_pCommandQueue->ProcessCommand(new CListCommand());
	
	m_pMainFrame->m_pOptions->SetLastServer(server);
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
