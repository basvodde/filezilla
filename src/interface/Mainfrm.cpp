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
#include "led.h"
#include "sitemanager.h"
#include "settingsdialog.h"
#include "themeprovider.h"
#include "filezillaapp.h"
#include "view.h"
#include "viewheader.h"
#include "aboutdialog.h"

#ifndef __WXMSW__
#include "resources/filezilla.xpm"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define RECVLED_TIMER_ID wxID_HIGHEST + 1
#define SENDLED_TIMER_ID wxID_HIGHEST + 2
#define TRANSFERSTATUS_TIMER_ID wxID_HIGHEST + 3

static const int statbarWidths[6] = {
#ifdef __WXMSW__
	-2, 90, -1, 150, -1, 41
#else
	-2, 90, -1, 150, -1, 50
#endif
};

BEGIN_EVENT_TABLE(CMainFrame, wxFrame)
	EVT_SIZE(CMainFrame::OnSize)
	EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, CMainFrame::OnViewSplitterPosChanged)
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
	EVT_TOOL(XRCID("ID_TOOLBAR_REFRESH"), CMainFrame::OnRefresh)
	EVT_TOOL(XRCID("ID_TOOLBAR_SITEMANAGER"), CMainFrame::OnSiteManager)
	EVT_CLOSE(CMainFrame::OnClose)
	EVT_TIMER(wxID_ANY, CMainFrame::OnTimer)
	EVT_TOOL(XRCID("ID_TOOLBAR_PROCESSQUEUE"), CMainFrame::OnProcessQueue)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_PROCESSQUEUE"), CMainFrame::OnUpdateToolbarProcessQueue)
	EVT_TOOL(XRCID("ID_TOOLBAR_LOGVIEW"), CMainFrame::OnToggleLogView)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_LOGVIEW"), CMainFrame::OnUpdateToggleLogView)
	EVT_TOOL(XRCID("ID_TOOLBAR_LOCALTREEVIEW"), CMainFrame::OnToggleLocalTreeView)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_LOCALTREEVIEW"), CMainFrame::OnUpdateToggleLocalTreeView)
	EVT_TOOL(XRCID("ID_TOOLBAR_REMOTETREEVIEW"), CMainFrame::OnToggleRemoteTreeView)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_REMOTETREEVIEW"), CMainFrame::OnUpdateToggleRemoteTreeView)
	EVT_TOOL(XRCID("ID_TOOLBAR_QUEUEVIEW"), CMainFrame::OnToggleQueueView)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_QUEUEVIEW"), CMainFrame::OnUpdateToggleQueueView)
	EVT_MENU(wxID_ABOUT, CMainFrame::OnMenuHelpAbout)
END_EVENT_TABLE()

CMainFrame::CMainFrame(COptions* pOptions) : wxFrame(NULL, -1, _T("FileZilla"), wxDefaultPosition, wxSize(900, 750))
{
	m_pOptions = pOptions;

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
	m_bQuit = false;

	m_lastLogViewSplitterPos = 0;
	m_lastLocalTreeSplitterPos = 0;
	m_lastRemoteTreeSplitterPos = 0;
	m_lastQueueSplitterPos = 0;

#ifdef __WXMSW__
	m_windowIsMaximized = false;
#endif

	m_pThemeProvider = new CThemeProvider(m_pOptions);
	m_pState = new CState();

	m_pStatusBar = CreateStatusBar(6, wxST_SIZEGRIP);
	if (m_pStatusBar)
	{
		m_pStatusBar->Connect(wxID_ANY, wxEVT_SIZE, (wxObjectEventFunction)(wxEventFunction)(wxSizeEventFunction)&CMainFrame::OnStatusbarSize, 0, this);
		int array[6];
		for (int i = 1; i < 5; i++)
			array[i] = wxSB_NORMAL;
		array[0] = wxSB_FLAT;
		array[5] = wxSB_FLAT;
		m_pStatusBar->SetStatusStyles(6, array);

		m_pStatusBar->SetStatusWidths(6, statbarWidths);
		
		m_pRecvLed = new CLed(m_pStatusBar, 1);
		m_pSendLed = new CLed(m_pStatusBar, 0);
		m_recvLedTimer.SetOwner(this, RECVLED_TIMER_ID);
		m_sendLedTimer.SetOwner(this, SENDLED_TIMER_ID);
	}
	else
	{
		m_pRecvLed = 0;
		m_pSendLed = 0;
	}

	m_transferStatusTimer.SetOwner(this, TRANSFERSTATUS_TIMER_ID);

	CreateMenus();
	CreateToolBar();
	CreateQuickconnectBar();

	m_ViewSplitterSashPos = 0.5;

	m_pEngine = new CFileZillaEngine();
	m_pEngine->Init(this, m_pOptions);
	
	m_pCommandQueue = new CCommandQueue(m_pEngine, this);

#ifdef __WXMSW__
	long style = wxSP_NOBORDER | wxSP_LIVE_UPDATE;
#else
	long style = wxSP_3DBORDER | wxSP_LIVE_UPDATE;
#endif

	wxSize clientSize = GetClientSize();

	m_pTopSplitter = new wxSplitterWindow(this, -1, wxDefaultPosition, clientSize, style);
	m_pTopSplitter->SetMinimumPaneSize(20);

	m_pBottomSplitter = new wxSplitterWindow(m_pTopSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pBottomSplitter->SetMinimumPaneSize(20);
	m_pBottomSplitter->SetSashGravity(1.0);

	m_pViewSplitter = new wxSplitterWindow(m_pBottomSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pViewSplitter->SetMinimumPaneSize(20);
	
	m_pLocalSplitter = new wxSplitterWindow(m_pViewSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pLocalSplitter->SetMinimumPaneSize(20);
	m_pLocalSplitter->SetSashGravity(0.7);

	m_pRemoteSplitter = new wxSplitterWindow(m_pViewSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pRemoteSplitter->SetMinimumPaneSize(20);
	m_pRemoteSplitter->SetSashGravity(0.7);

	m_pStatusView = new CStatusView(m_pTopSplitter, -1);
	m_pQueueView = new CQueueView(m_pBottomSplitter, -1, this);

	m_pLocalTreeViewPanel = new CView(m_pLocalSplitter);
	m_pLocalListViewPanel = new CView(m_pLocalSplitter);
	m_pLocalTreeView = new CLocalTreeView(m_pLocalTreeViewPanel, -1, m_pState, m_pQueueView);
	m_pLocalListView = new CLocalListView(m_pLocalListViewPanel, -1, m_pState, m_pQueueView);
	m_pLocalTreeViewPanel->SetWindow(m_pLocalTreeView);
	m_pLocalListViewPanel->SetWindow(m_pLocalListView);
	m_pLocalTreeViewPanel->SetHeader(new CLocalViewHeader(m_pLocalSplitter, m_pState));
	
	m_pRemoteTreeViewPanel = new CView(m_pRemoteSplitter);
	m_pRemoteListViewPanel = new CView(m_pRemoteSplitter);
	m_pRemoteTreeView = new CRemoteTreeView(m_pRemoteTreeViewPanel, -1);
	m_pRemoteListView = new CRemoteListView(m_pRemoteListViewPanel, -1, m_pState, m_pCommandQueue, m_pQueueView);
	m_pRemoteTreeViewPanel->SetWindow(m_pRemoteTreeView);
	m_pRemoteListViewPanel->SetWindow(m_pRemoteListView);
	m_pRemoteTreeViewPanel->SetHeader(new CRemoteViewHeader(m_pRemoteSplitter, m_pState, m_pCommandQueue));
	
	m_pTopSplitter->SplitHorizontally(m_pStatusView, m_pBottomSplitter, 100);
	m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueueView, 100);
	m_pViewSplitter->SplitVertically(m_pLocalSplitter, m_pRemoteSplitter);
	m_pLocalSplitter->SplitHorizontally(m_pLocalTreeViewPanel, m_pLocalListViewPanel);
	m_pRemoteSplitter->SplitHorizontally(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel);
	wxSize size = m_pBottomSplitter->GetClientSize();
	m_pBottomSplitter->SetSashPosition(size.GetHeight() - 140);
	
	Layout();

	m_pState->SetLocalListView(m_pLocalListView);
	m_pState->SetLocalTreeView(m_pLocalTreeView);
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
	if (!m_pBottomSplitter)
		return;

	float ViewSplitterSashPos = m_ViewSplitterSashPos;

	wxFrame::OnSize(event);

	wxSize clientSize = GetClientSize();
	if (m_pQuickconnectBar)
	{
		wxSize barSize = m_pQuickconnectBar->GetSize();
		m_pQuickconnectBar->SetSize(0, 0, clientSize.GetWidth(), barSize.GetHeight());
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

	m_ViewSplitterSashPos = ViewSplitterSashPos;

	if (m_pViewSplitter)
	{
		wxSize size = m_pViewSplitter->GetClientSize();
		int pos = static_cast<int>(size.GetWidth() * m_ViewSplitterSashPos);
		if (pos < 20)
			pos = 20;
		else if (pos > size.GetWidth() - 20)
			pos = size.GetWidth() - 20;
		m_pViewSplitter->SetSashPosition(pos);
	}

	ApplySplitterConstraints();
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
	if (event.GetId() == XRCID("wxID_EXIT"))
	{
		Close();
	}
	else if (event.GetId() == XRCID("ID_MENU_FILE_SITEMANAGER"))
	{
		OnSiteManager(event);
	}
	else if (event.GetId() == XRCID("ID_MENU_SERVER_CMD"))
	{
		if (!m_pEngine || !m_pEngine->IsConnected() || m_pEngine->IsBusy() || !m_pCommandQueue->Idle())
			return;

		wxTextEntryDialog dlg(this, _("Please enter raw FTP command.\nUsing raw ftp commands will clear the directory cache."), _("Enter custom command"));
		if (dlg.ShowModal() != wxID_OK)
			return;

		m_pCommandQueue->ProcessCommand(new CRawCommand(dlg.GetValue()));		
	}
	else if (event.GetId() == XRCID("wxID_PREFERENCES"))
	{
		OnMenuEditSettings(event);
	}
	else if (event.GetId() == XRCID("ID_CRASH"))
	{
		// Cause a crash
		int *x = 0;
		*x = 0;
	}
	else
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

	if (m_pEngine->IsConnected() || m_pEngine->IsBusy() || !m_pCommandQueue->Idle())
	{
		if (wxMessageBox(_("Break current connection?"), _T("FileZilla"), wxYES_NO | wxICON_QUESTION) != wxYES)
			return;
		m_pCommandQueue->Cancel();
	}

	m_pState->SetServer(&server);
	m_pCommandQueue->ProcessCommand(new CConnectCommand(server));
	m_pCommandQueue->ProcessCommand(new CListCommand());
	
	m_pOptions->SetServer(_T("Settings/LastServer"), server);
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
			m_pCommandQueue->Finish(reinterpret_cast<COperationNotification*>(pNotification));
			delete pNotification;
			if (m_bQuit)
			{
				Close();
				return;
			}
			break;
		case nId_listing:
			{
				CDirectoryListingNotification* pListingNotification = reinterpret_cast<CDirectoryListingNotification *>(pNotification);
				m_pState->SetRemoteDir(pListingNotification->GetDirectoryListing(), pListingNotification->Modified());
				delete pListingNotification;
			}
			break;
		case nId_asyncrequest:
			m_pAsyncRequestQueue->AddRequest(m_pEngine, reinterpret_cast<CAsyncRequestNotification *>(pNotification));
			break;
		case nId_active:
			{
				CActiveNotification *pActiveNotification = reinterpret_cast<CActiveNotification *>(pNotification);
				if (pActiveNotification->IsRecv())
					UpdateRecvLed();
				else
					UpdateSendLed();
				delete pNotification;
			}
			break;
		case nId_transferstatus:
			{
				CTransferStatusNotification *pTransferStatusNotification = reinterpret_cast<CTransferStatusNotification *>(pNotification);
				const CTransferStatus *pStatus = pTransferStatusNotification->GetStatus();
				if (pStatus && !m_transferStatusTimer.IsRunning())
					m_transferStatusTimer.Start(100);
				else if (!pStatus && m_transferStatusTimer.IsRunning())
					m_transferStatusTimer.Stop();
                
				SetProgress(pStatus);
				delete pNotification;
			}
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
	{
		m_pCommandQueue->Cancel();
		GetRemoteListView()->StopRecursiveOperation();
	}
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
	Show(false);
	m_bQuit = true;
	m_sendLedTimer.Stop();
	m_recvLedTimer.Stop();
	delete m_pSendLed;
	delete m_pRecvLed;
	m_pSendLed = 0;
	m_pRecvLed = 0;

	m_transferStatusTimer.Stop();

	bool res = true;
	if (m_pCommandQueue)
		res = m_pCommandQueue->Cancel();

	if (!res)
	{
		event.Veto();
		return;
	}
	delete m_pEngine;
	m_pEngine = 0;

	if (!m_pQueueView->Quit())
	{
		event.Veto();
		return;
	}

	Destroy();
}

void CMainFrame::OnUpdateToolbarReconnect(wxUpdateUIEvent &event)
{
	if (!m_pEngine || m_pEngine->IsConnected() || m_pEngine->IsBusy() || !m_pOptions)
	{
		event.Enable(false);
		return;
	}
	
	CServer server;
	event.Enable(m_pOptions->GetServer(_T("Settings/LastServer"), server));
}

void CMainFrame::OnReconnect(wxCommandEvent &event)
{
	if (!m_pEngine || m_pEngine->IsConnected() || m_pEngine->IsBusy() || !m_pOptions)
		return;
	
	CServer server;
	if (!m_pOptions->GetServer(_T("Settings/LastServer"), server))
		return;

	if (server.GetLogonType() == ASK)
	{
		if (!GetPassword(server))
			return;
	}
	
	m_pState->SetServer(&server);
	m_pCommandQueue->ProcessCommand(new CConnectCommand(server));
	m_pCommandQueue->ProcessCommand(new CListCommand());
}

void CMainFrame::OnRefresh(wxCommandEvent &event)
{
	if (m_pEngine && m_pEngine->IsConnected() && !m_pEngine->IsBusy())
		m_pCommandQueue->ProcessCommand(new CListCommand(m_pState->GetRemotePath(), _T(""), true));

	if (m_pState)
		m_pState->RefreshLocal();
}

void CMainFrame::OnStatusbarSize(wxSizeEvent& event)
{
	if (!m_pStatusBar)
		return;

#ifdef __WXMSW__
	if (IsMaximized() && !m_windowIsMaximized)
	{
		m_windowIsMaximized = true;
		int widths[6];
		memcpy(widths, statbarWidths, 6 * sizeof(int));
		widths[5] = 35;
		m_pStatusBar->SetStatusWidths(6, widths);
		m_pStatusBar->Refresh();
	}
	else if (!IsMaximized() && m_windowIsMaximized)
	{
		m_windowIsMaximized = false;
		
		m_pStatusBar->SetStatusWidths(6, statbarWidths);
		m_pStatusBar->Refresh();
	}
#endif

	if (m_pSendLed)
	{
		wxRect rect;
		m_pStatusBar->GetFieldRect(5, rect);
		m_pSendLed->SetSize(rect.GetLeft() + 16, rect.GetTop() + (rect.GetHeight() - 11) / 2, -1, -1);
	}

	if (m_pRecvLed)
	{
		wxRect rect;
		m_pStatusBar->GetFieldRect(5, rect);
		m_pRecvLed->SetSize(rect.GetLeft() + 2, rect.GetTop() + (rect.GetHeight() - 11) / 2, -1, -1);
	}
}

void CMainFrame::OnTimer(wxTimerEvent& event)
{
	if (event.GetId() == RECVLED_TIMER_ID && m_recvLedTimer.IsRunning())
	{
		if (!m_pEngine)
		{
			m_recvLedTimer.Stop();
			return;
		}

		if (!m_pEngine->IsActive(true))
		{
			if (m_pRecvLed)
				m_pRecvLed->Unset();
			m_recvLedTimer.Stop();
		}
	}
	else if (event.GetId() == SENDLED_TIMER_ID && m_sendLedTimer.IsRunning())
	{
		if (!m_pEngine)
		{
			m_sendLedTimer.Stop();
			return;
		}

		if (!m_pEngine->IsActive(false))
		{
			if (m_pSendLed)
				m_pSendLed->Unset();
			m_sendLedTimer.Stop();
		}
	}
	else if (event.GetId() == TRANSFERSTATUS_TIMER_ID && m_transferStatusTimer.IsRunning())
	{
		if (!m_pEngine)
		{
			m_transferStatusTimer.Stop();
			return;
		}

		bool changed;
		CTransferStatus status;
		if (!m_pEngine->GetTransferStatus(status, changed))
		{
			SetProgress(0);
			m_transferStatusTimer.Stop();
		}
		else if (changed)
			SetProgress(&status);
		else
			m_transferStatusTimer.Stop();
	}
}

void CMainFrame::SetProgress(const CTransferStatus *pStatus)
{
	return;

	/* TODO: If called during primary transfer, relay to queue, else relay to remote file list statusbar.
	if (!m_pStatusBar)
		return;

	if (!pStatus)
	{
		m_pStatusBar->SetStatusText(_T(""), 1);
		m_pStatusBar->SetStatusText(_T(""), 2);
		m_pStatusBar->SetStatusText(_T(""), 3);
		return;
	}
	
	wxTimeSpan elapsed = wxDateTime::Now().Subtract(pStatus->started);
	m_pStatusBar->SetStatusText(elapsed.Format(_("%H:%M:%S elapsed")), 1);

	int elapsedSeconds = elapsed.GetSeconds().GetLo(); // Assume GetHi is always 0
	if (elapsedSeconds)
	{
		wxFileOffset rate = (pStatus->currentOffset - pStatus->startOffset) / elapsedSeconds;

        if (rate > (1000*1000))
			m_pStatusBar->SetStatusText(wxString::Format(_("%s bytes (%d.%d MB/s)"), wxLongLong(pStatus->currentOffset).ToString().c_str(), (int)(rate / 1000 / 1000), (int)((rate / 1000 / 100) % 10)), 4);
		else if (rate > 1000)
			m_pStatusBar->SetStatusText(wxString::Format(_("%s bytes (%d.%d KB/s)"), wxLongLong(pStatus->currentOffset).ToString().c_str(), (int)(rate / 1000), (int)((rate / 100) % 10)), 4);
		else
			m_pStatusBar->SetStatusText(wxString::Format(_("%s bytes (%d B/s)"), wxLongLong(pStatus->currentOffset).ToString().c_str(), (int)rate), 4);

		if (pStatus->totalSize > 0 && rate > 0)
		{
			int left = ((pStatus->totalSize - pStatus->startOffset) / rate) - elapsedSeconds;
			if (left < 0)
				left = 0;
			wxTimeSpan timeLeft(0, 0, left);
			m_pStatusBar->SetStatusText(timeLeft.Format(_("%H:%M:%S left")), 2);
		}
		else
		{
			m_pStatusBar->SetStatusText(_("--:--:-- left"), 2);
		}
	}
	else
		m_pStatusBar->SetStatusText(wxString::Format(_("%s bytes (? B/s)"), wxLongLong(pStatus->currentOffset).ToString().c_str()), 4);
	*/
}

void CMainFrame::OnSiteManager(wxCommandEvent& event)
{
	CSiteManager dlg(m_pOptions);
	if (!dlg.Create(this))
		return;

	int res = dlg.ShowModal();
	if (res == wxID_YES)
	{
		CSiteManagerItemData data;
		if (!dlg.GetServer(data))
			return;

		if (data.m_server.GetLogonType() == ASK)
		{
			if (!GetPassword(data.m_server, data.m_name))
				return;
		}

		if (m_pEngine->IsConnected() || m_pEngine->IsBusy())
		{
			if (wxMessageBox(_("Break current connection?"), _T("FileZilla"), wxYES_NO | wxICON_QUESTION) != wxYES)
				return;
			m_pCommandQueue->Cancel();
		}

		m_pState->SetServer(&data.m_server);
		m_pCommandQueue->ProcessCommand(new CConnectCommand(data.m_server));
		m_pCommandQueue->ProcessCommand(new CListCommand(data.m_remoteDir));

		m_pOptions->SetServer(_T("Settings/LastServer"), data.m_server);

		if (data.m_localDir != _T(""))
			m_pState->SetLocalDir(data.m_localDir);
	}
}

bool CMainFrame::GetPassword(CServer &server, wxString name /*=_T("")*/, wxString challenge /*=_T("")*/)
{
	if (server.GetLogonType() == ASK)
	{
		std::list<t_passwordcache>::const_iterator iter;
		for (iter = m_passwordCache.begin(); iter != m_passwordCache.end(); iter++)
		{
			if (iter->host != server.GetHost())
				continue;
			if (iter->port != server.GetPort())
				continue;
			if (iter->user != server.GetUser())
				continue;
	
			server.SetUser(server.GetUser(), iter->password);
			return true;
		}
	}

	wxDialog pwdDlg;
	wxXmlResource::Get()->LoadDialog(&pwdDlg, this, _T("ID_ENTERPASSWORD"));
	if (name == _T(""))
	{
		pwdDlg.GetSizer()->Show(XRCCTRL(pwdDlg, "ID_NAMELABEL", wxStaticText), false, true);
		pwdDlg.GetSizer()->Show(XRCCTRL(pwdDlg, "ID_NAME", wxStaticText), false, true);
	}
	else
		XRCCTRL(pwdDlg, "ID_NAME", wxStaticText)->SetLabel(name);
	if (challenge == _T(""))
	{
		pwdDlg.GetSizer()->Show(XRCCTRL(pwdDlg, "ID_CHALLENGELABEL", wxStaticText), false, true);
		pwdDlg.GetSizer()->Show(XRCCTRL(pwdDlg, "ID_CHALLENGE", wxTextCtrl), false, true);
	}
	else
	{
		XRCCTRL(pwdDlg, "ID_CHALLENGE", wxTextCtrl)->SetLabel(challenge);
		pwdDlg.GetSizer()->Show(XRCCTRL(pwdDlg, "ID_REMEMBER", wxCheckBox), false, true);
		XRCCTRL(pwdDlg, "ID_CHALLENGE", wxTextCtrl)->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	}
	XRCCTRL(pwdDlg, "ID_HOST", wxStaticText)->SetLabel(server.FormatHost());
	XRCCTRL(pwdDlg, "ID_USER", wxStaticText)->SetLabel(server.GetUser());
	XRCCTRL(pwdDlg, "wxID_OK", wxButton)->SetId(wxID_OK);
	XRCCTRL(pwdDlg, "wxID_CANCEL", wxButton)->SetId(wxID_CANCEL);
	pwdDlg.GetSizer()->Fit(&pwdDlg);
	pwdDlg.GetSizer()->SetSizeHints(&pwdDlg);
	if (pwdDlg.ShowModal() != wxID_OK)
		return false;

	server.SetUser(server.GetUser(), XRCCTRL(pwdDlg, "ID_PASSWORD", wxTextCtrl)->GetValue());

	if (server.GetLogonType() == ASK && XRCCTRL(pwdDlg, "ID_REMEMBER", wxCheckBox)->GetValue())
	{
		t_passwordcache entry;
		entry.host = server.GetHost();
		entry.port = server.GetPort();
		entry.user = server.GetUser();
		entry.password = server.GetPass();
		m_passwordCache.push_back(entry);
	}

	return true;
}

void CMainFrame::UpdateSendLed()
{
	if (m_pSendLed && !m_sendLedTimer.IsRunning())
	{
		m_pSendLed->Set();
		m_sendLedTimer.Start(100);
	}
}

void CMainFrame::UpdateRecvLed()
{
	if (m_pRecvLed && !m_recvLedTimer.IsRunning())
	{
		m_pRecvLed->Set();
		m_recvLedTimer.Start(100);
	}
}

void CMainFrame::AddToRequestQueue(CFileZillaEngine *pEngine, CAsyncRequestNotification *pNotification)
{
	m_pAsyncRequestQueue->AddRequest(pEngine, reinterpret_cast<CAsyncRequestNotification *>(pNotification));
}

void CMainFrame::OnProcessQueue(wxCommandEvent& event)
{
	if (m_pQueueView)
		m_pQueueView->SetActive(event.IsChecked());
}

void CMainFrame::OnUpdateToolbarProcessQueue(wxUpdateUIEvent& event)
{
	event.Check(m_pQueueView->IsActive() != 0);
}

void CMainFrame::OnMenuEditSettings(wxCommandEvent& event)
{
	CSettingsDialog dlg(m_pOptions);
	if (!dlg.Create(this))
		return;

	wxString oldTheme = m_pOptions->GetOption(OPTION_THEME);
	int oldLang = wxGetApp().GetCurrentLanguage();

	int res = dlg.ShowModal();
	if (res != wxID_OK)
		return;

	wxString newTheme = m_pOptions->GetOption(OPTION_THEME);
	int newLang = wxGetApp().GetCurrentLanguage();

	if (oldTheme != newTheme)
	{
		wxArtProvider::RemoveProvider(m_pThemeProvider);
		m_pThemeProvider = new CThemeProvider(m_pOptions);
	}
	if (oldTheme != newTheme || oldLang != newLang)
		CreateToolBar();
	if (oldLang != newLang)
		CreateMenus();
}

void CMainFrame::OnToggleLogView(wxCommandEvent& event)
{
	if (!m_pTopSplitter)
		return;

	if (m_pTopSplitter->IsSplit())
	{
		m_lastLogViewSplitterPos = m_pTopSplitter->GetSashPosition();
		m_pTopSplitter->Unsplit(m_pStatusView);
	}
	else
	{
		// Sometimes m_pQueueView resizes instead of m_bViewSplitter, save original value
		wxRect rect = m_pBottomSplitter->GetClientSize();
		int queueSplitterPos = rect.GetHeight() - m_pBottomSplitter->GetSashPosition();
		m_pTopSplitter->SplitHorizontally(m_pStatusView, m_pBottomSplitter, m_lastLogViewSplitterPos);

		// Restore previous queue size
		rect = m_pBottomSplitter->GetClientSize();
		if (queueSplitterPos != (rect.GetHeight() - m_pBottomSplitter->GetSashPosition())) 
			m_pBottomSplitter->SetSashPosition(rect.GetHeight() - queueSplitterPos);

		ApplySplitterConstraints();
	}
}

void CMainFrame::OnUpdateToggleLogView(wxUpdateUIEvent& event)
{
	event.Check(m_pTopSplitter && m_pTopSplitter->IsSplit());
}

void CMainFrame::ApplySplitterConstraints()
{
	if (m_pTopSplitter->IsSplit())
	{
		wxSize size = m_pTopSplitter->GetClientSize();
		if (size.GetHeight() - m_pTopSplitter->GetSashPosition() < 100)
			m_pTopSplitter->SetSashPosition(size.GetHeight() - 103);
	}

	if (m_pBottomSplitter->GetSashPosition() < 45)
		m_pBottomSplitter->SetSashPosition(45);
}

void CMainFrame::OnToggleLocalTreeView(wxCommandEvent& event)
{
	if (!m_pTopSplitter)
		return;

	if (m_pLocalSplitter->IsSplit())
	{
		m_pLocalListViewPanel->SetHeader(m_pLocalTreeViewPanel->DetachHeader());
		m_lastLocalTreeSplitterPos = m_pLocalSplitter->GetSashPosition();
		m_pLocalSplitter->Unsplit(m_pLocalTreeViewPanel);
	}
	else
	{
		m_pLocalTreeViewPanel->SetHeader(m_pLocalListViewPanel->DetachHeader());
		wxSize size = m_pLocalSplitter->GetClientSize();
		m_pLocalSplitter->SplitHorizontally(m_pLocalTreeViewPanel, m_pLocalListViewPanel, m_lastLocalTreeSplitterPos);
	}
}

void CMainFrame::OnUpdateToggleLocalTreeView(wxUpdateUIEvent& event)
{
	event.Check(m_pLocalSplitter && m_pLocalSplitter->IsSplit());
}

void CMainFrame::OnToggleRemoteTreeView(wxCommandEvent& event)
{
	if (!m_pTopSplitter)
		return;

	if (m_pRemoteSplitter->IsSplit())
	{
		m_pRemoteListViewPanel->SetHeader(m_pRemoteTreeViewPanel->DetachHeader());
		m_lastRemoteTreeSplitterPos = m_pRemoteSplitter->GetSashPosition();
		m_pRemoteSplitter->Unsplit(m_pRemoteTreeViewPanel);
	}
	else
	{
		m_pRemoteTreeViewPanel->SetHeader(m_pRemoteListViewPanel->DetachHeader());
		wxSize size = m_pRemoteSplitter->GetClientSize();
		m_pRemoteSplitter->SplitHorizontally(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel, m_lastRemoteTreeSplitterPos);
	}
}

void CMainFrame::OnUpdateToggleRemoteTreeView(wxUpdateUIEvent& event)
{
	event.Check(m_pRemoteSplitter && m_pRemoteSplitter->IsSplit());
}

void CMainFrame::OnToggleQueueView(wxCommandEvent& event)
{
	if (!m_pBottomSplitter)
		return;

	if (m_pBottomSplitter->IsSplit())
	{
		wxRect rect = m_pBottomSplitter->GetClientSize();
		m_lastQueueSplitterPos = rect.GetHeight() - m_pBottomSplitter->GetSashPosition();
		m_pBottomSplitter->Unsplit(m_pQueueView);
	}
	else
	{
		wxRect rect = m_pBottomSplitter->GetClientSize();
		m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueueView, rect.GetHeight() - m_lastQueueSplitterPos);
		ApplySplitterConstraints();
	}
}

void CMainFrame::OnUpdateToggleQueueView(wxUpdateUIEvent& event)
{
	event.Check(m_pBottomSplitter && m_pBottomSplitter->IsSplit());
}

void CMainFrame::OnMenuHelpAbout(wxCommandEvent& event)
{
	CAboutDialog dlg;
	if (!dlg.Create(this))
		return;

	dlg.ShowModal();
}
