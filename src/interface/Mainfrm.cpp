#include "FileZilla.h"
#include "LocalTreeView.h"
#include "LocalListView.h"
#include "RemoteTreeView.h"
#include "RemoteListView.h"
#include "StatusView.h"
#include "QueueView.h"
#include "Mainfrm.h"
#include "state.h"
#include "Options.h"
#include "commandqueue.h"
#include "asyncrequestqueue.h"

#ifndef __WXMSW__
#include "resources/filezilla.xpm"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(CMainFrame, wxFrame)
    EVT_SIZE(CMainFrame::OnSize)
	EVT_SPLITTER_SASH_POS_CHANGED(-1, CMainFrame::OnViewSplitterPosChanged)
	EVT_MENU(wxID_ANY, CMainFrame::OnMenuHandler)
	EVT_BUTTON(XRCID("ID_QUICKCONNECT_OK"), CMainFrame::OnQuickconnect)
	EVT_FZ_NOTIFICATION(wxID_ANY, CMainFrame::OnEngineEvent)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_DISCONNECT"), CMainFrame::OnUpdateToolbarDisconnect)
	EVT_TOOL(XRCID("ID_TOOLBAR_DISCONNECT"), CMainFrame::OnDisconnect)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_CANCEL"), CMainFrame::OnUpdateToolbarCancel)
	EVT_TOOL(XRCID("ID_TOOLBAR_CANCEL"), CMainFrame::OnCancel)
	EVT_SPLITTER_SASH_POS_CHANGING(wxID_ANY, CMainFrame::OnSplitterSashPosChanging) 
    EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, CMainFrame::OnSplitterSashPosChanged) 
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_RECONNECT"), CMainFrame::OnUpdateToolbarReconnect)
	EVT_TOOL(XRCID("ID_TOOLBAR_RECONNECT"), CMainFrame::OnReconnect)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_REFRESH"), CMainFrame::OnUpdateToolbarRefresh)
	EVT_TOOL(XRCID("ID_TOOLBAR_REFRESH"), CMainFrame::OnRefresh)
	EVT_CLOSE(CMainFrame::OnClose)
END_EVENT_TABLE()

CMainFrame::CMainFrame() : wxFrame(NULL, -1, _T("FileZilla"), wxDefaultPosition, wxSize(900, 750))
{
	SetSizeHints(250, 250);

    SetIcon(wxICON(appicon));

	m_pStatusBar = NULL;
	m_pMenuBar = NULL;
	m_pToolBar = 0;
	m_pQuickconnectBar = NULL;
	m_pTopSplitter = NULL;
	m_pBottomSplitter = NULL;
	m_pViewSplitter = NULL;
	m_pLocalSplitter = NULL;
	m_pRemoteSplitter = NULL;
	m_bInitDone = false;

	m_pState = new CState();

	m_pStatusBar = CreateStatusBar(8, wxST_SIZEGRIP);
	int array[8];
	for (int i = 1; i < 6; i++)
		array[i] = wxSB_NORMAL;
	array[0] = wxSB_FLAT;
	array[6] = wxSB_FLAT;
	array[7] = wxSB_FLAT;
	m_pStatusBar->SetStatusStyles(8, array);

	CreateToolBar();
	CreateMenus();
	CreateQuickconnectBar();

	m_ViewSplitterSashPos = 0.5;

#ifdef __WXMSW__
	long style = wxSP_NOBORDER | wxSP_LIVE_UPDATE;
#else
	long style = wxSP_3DBORDER | wxSP_LIVE_UPDATE;
#endif

	m_pOptions = new COptions;

	m_pEngine = new CFileZillaEngine();
	m_pEngine->Init(this, m_pOptions);
	
	m_pCommandQueue = new CCommandQueue(m_pEngine);

	wxSize clientSize = GetClientSize();
	if (m_pBottomSplitter)
		clientSize.SetHeight(clientSize.GetHeight() - m_pBottomSplitter->GetSize().GetHeight());

	m_pTopSplitter = new wxSplitterWindow(this, -1, wxDefaultPosition, clientSize, style);
	m_pTopSplitter->SetMinimumPaneSize(20);

	m_pBottomSplitter = new wxSplitterWindow(m_pTopSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pBottomSplitter->SetMinimumPaneSize(20);

	m_pViewSplitter = new wxSplitterWindow(m_pBottomSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pViewSplitter->SetMinimumPaneSize(20);

	m_pLocalSplitter = new wxSplitterWindow(m_pViewSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pLocalSplitter->SetMinimumPaneSize(20);

	m_pRemoteSplitter = new wxSplitterWindow(m_pViewSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pRemoteSplitter->SetMinimumPaneSize(20);

	m_pLocalTreeView = new CLocalTreeView(m_pLocalSplitter, -1);
	m_pLocalListView = new CLocalListView(m_pLocalSplitter, -1, m_pState);
	m_pRemoteTreeView = new CRemoteTreeView(m_pRemoteSplitter, -1);
	m_pRemoteListView = new CRemoteListView(m_pRemoteSplitter, -1, m_pState, m_pCommandQueue);
	m_pStatusView = new CStatusView(m_pTopSplitter, -1);
	m_pQueueView = new CQueueView(m_pBottomSplitter, -1);

	m_pTopSplitter->SplitHorizontally(m_pStatusView, m_pBottomSplitter, 100);
	m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueueView, 100);
	m_pViewSplitter->SplitVertically(m_pLocalSplitter, m_pRemoteSplitter);
	m_pLocalSplitter->SplitHorizontally(m_pLocalTreeView, m_pLocalListView);
	m_pRemoteSplitter->SplitHorizontally(m_pRemoteTreeView, m_pRemoteListView);
	wxSize size = m_pBottomSplitter->GetClientSize();
	m_pBottomSplitter->SetSashPosition(size.GetHeight() - 140);

	Layout();

	m_pState->SetLocalListView(m_pLocalListView);
	m_pState->SetRemoteListView(m_pRemoteListView);
	m_pState->SetLocalDir(wxGetCwd());

	m_pAsyncRequestQueue = new CAsyncRequestQueue(this);
}

CMainFrame::~CMainFrame()
{
	delete m_pState;
	delete m_pCommandQueue;
	delete m_pEngine;
	delete m_pOptions;
	delete m_pAsyncRequestQueue;
}

void CMainFrame::OnSize(wxSizeEvent &event)
{
	float ViewSplitterSashPos = m_ViewSplitterSashPos;
	if (!m_pBottomSplitter)
		return;

	int pos = m_pBottomSplitter->GetSashPosition();
	wxSize size = m_pBottomSplitter->GetClientSize();
	
	wxFrame::OnSize(event);

	wxSize clientSize = GetClientSize();
	if (m_pQuickconnectBar)
	{
		wxSize barSize = m_pQuickconnectBar->GetSize();
		m_pQuickconnectBar->SetSize(0, 0, clientSize.GetWidth(), barSize.GetHeight());

		wxButton *pButton = XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_OK", wxButton);
		wxSize buttonSize = pButton->GetSize();
		wxPoint position = pButton->GetPosition();
		wxButton *pDropdownButton = XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_DROPDOWN", wxButton);
		wxSize dropdownSize = pDropdownButton->GetSize();
		pDropdownButton->SetSize(-1, position.y, dropdownSize.GetWidth(), buttonSize.GetHeight());
	}
	if (m_pTopSplitter)
	{
		if (!m_pQuickconnectBar)
			m_pTopSplitter->SetSize(clientSize.GetWidth(), clientSize.GetHeight());
		else
		{
			wxSize panelSize = m_pQuickconnectBar->GetSize();
			m_pTopSplitter->SetSize(0, panelSize.GetHeight(), clientSize.GetWidth(), clientSize.GetHeight() - panelSize.GetHeight());
		}
	}
	wxSize size2 = m_pBottomSplitter->GetClientSize();

	if (m_bInitDone)
		m_pBottomSplitter->SetSashPosition(size2.GetHeight() - size.GetHeight() + pos);
	else
		m_bInitDone = true;

	m_ViewSplitterSashPos = ViewSplitterSashPos;

	if (m_pViewSplitter)
	{
		size = m_pViewSplitter->GetClientSize();
		int pos = static_cast<int>(size.GetWidth() * m_ViewSplitterSashPos);
		if (pos < 20)
			pos = 20;
		else if (pos > size.GetWidth() - 20)
			pos = size.GetWidth() - 20;
		m_pViewSplitter->SetSashPosition(pos);
	}
}

void CMainFrame::OnViewSplitterPosChanged(wxSplitterEvent &event)
{
	if (event.GetEventObject() != m_pViewSplitter)
	{
		event.Skip();
		return;
	}

	wxSize size = m_pViewSplitter->GetClientSize();
	int pos = m_pViewSplitter->GetSashPosition();
	m_ViewSplitterSashPos = pos / (float)size.GetWidth();
}

bool CMainFrame::CreateMenus()
{
	if (m_pMenuBar)
	{
		SetMenuBar(0);
		delete m_pMenuBar;
	}
	m_pMenuBar = wxXmlResource::Get()->LoadMenuBar(_T("ID_MENUBAR"));
	if (!m_pMenuBar)
	{
		wxLogError(_("Cannot load main menu from resource file"));
	}
	SetMenuBar(m_pMenuBar);

	return true;
}

bool CMainFrame::CreateQuickconnectBar()
{
	if (m_pQuickconnectBar)
	{
		delete m_pQuickconnectBar;
	}

	m_pQuickconnectBar = wxXmlResource::Get()->LoadPanel(this, _T("ID_QUICKCONNECTBAR"));

	if (!m_pQuickconnectBar)
	{
		wxLogError(_("Cannot load Quickconnect bar from resource file"));
	}
	else
	{
		XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetMaxLength(5);
	}

	return true;
}

void CMainFrame::OnMenuHandler(wxCommandEvent &event)
{
	if (event.GetId() == XRCID("ID_MENU_FILE_EXIT"))
	{
		Close();
	}
	event.Skip();
}

void CMainFrame::OnQuickconnect(wxCommandEvent &event)
{	
	if (!m_pQuickconnectBar)
		return;

	if (!m_pEngine)
		return;

	wxString host = XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_HOST", wxTextCtrl)->GetValue();
	wxString user = XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_USER", wxTextCtrl)->GetValue();
	wxString pass = XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_PASS", wxTextCtrl)->GetValue();
	wxString port = XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_PORT", wxTextCtrl)->GetValue();
	
	long numericPort = -1;
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
		wxMessageBox(msg, _("FileZilla Error"), wxICON_EXCLAMATION);
		return;
	}

	host = server.GetHost();
	ServerProtocol protocol = server.GetProtocol();
	switch (protocol)
	{
	//TODO: If not ftp, add protocol to host
	case FTP:
	default:
		// do nothing
		break;
	}
	
	XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_HOST", wxTextCtrl)->SetValue(host);
	XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_PORT", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), server.GetPort()));
	XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_USER", wxTextCtrl)->SetValue(server.GetUser());
	XRCCTRL(*m_pQuickconnectBar, "ID_QUICKCONNECT_PASS", wxTextCtrl)->SetValue(server.GetPass());

	if (m_pEngine->IsConnected() || m_pEngine->IsBusy())
	{
		if (wxMessageBox(_("Break current connection?"), _T("FileZilla"), wxYES_NO | wxICON_QUESTION) != wxYES)
			return;
		m_pCommandQueue->Cancel();
	}

	m_pCommandQueue->ProcessCommand(new CConnectCommand(server));
	m_pCommandQueue->ProcessCommand(new CListCommand());
}

void CMainFrame::OnEngineEvent(wxEvent &event)
{
	if (!m_pEngine)
		return;

	CNotification *pNotification = m_pEngine->GetNextNotification();
	while (pNotification)
	{
		switch (pNotification->GetID())
		{
		case nId_logmsg:
			m_pStatusView->AddToLog(reinterpret_cast<CLogmsgNotification *>(pNotification));
			delete pNotification;
			break;
		case nId_operation:
			m_pCommandQueue->Finish(pNotification);
			delete pNotification;
			break;
		case nId_listing:
			m_pState->SetRemoteDir(reinterpret_cast<CDirectoryListingNotification *>(pNotification)->GetDirectoryListing());
			delete pNotification;
			break;
		case nId_asyncrequest:
			m_pAsyncRequestQueue->AddRequest(m_pEngine, reinterpret_cast<CAsyncRequestNotification *>(pNotification));
			break;
		default:
			delete pNotification;
			break;
		}

		pNotification = m_pEngine->GetNextNotification();
	}

}

bool CMainFrame::CreateToolBar()
{
	if (m_pToolBar)
	{
		SetToolBar(0);
		delete m_pToolBar;
	}
	m_pToolBar = wxXmlResource::Get()->LoadToolBar(this, _T("ID_TOOLBAR"));
	if (!m_pToolBar)
	{
		wxLogError(_("Cannot load toolbar from resource file"));
	}
	SetToolBar(m_pToolBar);

	return true;
}

void CMainFrame::OnUpdateToolbarDisconnect(wxUpdateUIEvent& event)
{
	event.Enable(m_pEngine && m_pEngine->IsConnected() && !m_pEngine->IsBusy());
}

void CMainFrame::OnDisconnect(wxCommandEvent& event)
{
	if (!m_pEngine)
		return;

	if (m_pEngine->IsBusy())
		return;

	m_pCommandQueue->ProcessCommand(new CDisconnectCommand());
}

void CMainFrame::OnUpdateToolbarCancel(wxUpdateUIEvent& event)
{
	event.Enable(m_pEngine && m_pEngine->IsBusy());
}

void CMainFrame::OnCancel(wxCommandEvent& event)
{
	if (!m_pEngine || !m_pEngine->IsBusy())
		return;

	if (wxMessageBox(_("Really cancel current operation?"), _T("FileZilla"), wxYES_NO | wxICON_QUESTION) == wxYES)
		m_pCommandQueue->Cancel();
}

void CMainFrame::OnSplitterSashPosChanging(wxSplitterEvent& event)
{
	if (event.GetEventObject() == m_pBottomSplitter)
	{
		if (!m_pRemoteSplitter || !m_pLocalSplitter)
			return;

		if (event.GetSashPosition() < 43)
			event.SetSashPosition(43);
	}
}

void CMainFrame::OnSplitterSashPosChanged(wxSplitterEvent& event)
{
	if (event.GetEventObject() == m_pBottomSplitter)
	{
		if (!m_pRemoteSplitter || !m_pLocalSplitter)
			return;

		if (event.GetSashPosition() < 43)
			event.SetSashPosition(43);

		int delta = event.GetSashPosition() - m_pBottomSplitter->GetSashPosition();

		int newSize = m_pRemoteSplitter->GetClientSize().GetHeight() - m_pRemoteSplitter->GetSashPosition() + delta;
		if (newSize < 0)
			event.Veto();
		else if (newSize < 20)
			m_pRemoteSplitter->SetSashPosition(m_pRemoteSplitter->GetSashPosition() - 20 + newSize);
				
		newSize = m_pLocalSplitter->GetClientSize().GetHeight() - m_pLocalSplitter->GetSashPosition() + delta;
		if (newSize < 0)
			event.Veto();
		else if (newSize < 20)
			m_pLocalSplitter->SetSashPosition(m_pLocalSplitter->GetSashPosition() - 20 + newSize);
	}
}

void CMainFrame::OnClose(wxCloseEvent &event)
{
	if (m_pCommandQueue)
		m_pCommandQueue->Cancel();
	Destroy();
}

void CMainFrame::OnUpdateToolbarReconnect(wxUpdateUIEvent &event)
{
	event.Enable(m_pEngine && !m_pEngine->IsConnected() && !m_pEngine->IsBusy());
}

void CMainFrame::OnReconnect(wxCommandEvent &event)
{
	wxString error;
	CServerPath path;
	CServer server;
	server.ParseUrl(_T("127.0.0.1"), 21, _T("dev"), _T("dev"), error, path);
	m_pCommandQueue->ProcessCommand(new CConnectCommand(server));
	m_pCommandQueue->ProcessCommand(new CListCommand());
}

void CMainFrame::OnUpdateToolbarRefresh(wxUpdateUIEvent &event)
{
}

void CMainFrame::OnRefresh(wxCommandEvent &event)
{
	if (m_pEngine && m_pEngine->IsConnected() && !m_pEngine->IsBusy())
		m_pCommandQueue->ProcessCommand(new CListCommand());

	if (m_pState)
		m_pState->RefreshLocal();
}
