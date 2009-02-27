#include "FileZilla.h"
#include "LocalTreeView.h"
#include "LocalListView.h"
#include "RemoteTreeView.h"
#include "RemoteListView.h"
#include "StatusView.h"
#include "queue.h"
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
#include "filter.h"
#include "netconfwizard.h"
#include "quickconnectbar.h"
#include "updatewizard.h"
#include "defaultfileexistsdlg.h"
#include "loginmanager.h"
#include "conditionaldialog.h"
#include "clearprivatedata.h"
#include "export.h"
#include "import.h"
#include "recursive_operation.h"
#include <wx/tokenzr.h>
#include "edithandler.h"
#include "inputdialog.h"
#include "window_state_manager.h"
#include "xh_toolb_ex.h"
#include "statusbar.h"
#include "cmdline.h"
#include "buildinfo.h"
#include "filelist_statusbar.h"
#include "manual_transfer.h"
#include "auto_ascii_files.h"
#include "splitter.h"
#include "bookmarks_dialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TRANSFERSTATUS_TIMER_ID wxID_HIGHEST + 3

BEGIN_EVENT_TABLE(CMainFrame, wxFrame)
	EVT_SIZE(CMainFrame::OnSize)
	EVT_MENU(wxID_ANY, CMainFrame::OnMenuHandler)
	EVT_MENU_OPEN(CMainFrame::OnMenuOpenHandler)
	EVT_FZ_NOTIFICATION(wxID_ANY, CMainFrame::OnEngineEvent)
	EVT_TOOL(XRCID("ID_TOOLBAR_DISCONNECT"), CMainFrame::OnDisconnect)
	EVT_MENU(XRCID("ID_MENU_SERVER_DISCONNECT"), CMainFrame::OnDisconnect)
	EVT_TOOL(XRCID("ID_TOOLBAR_CANCEL"), CMainFrame::OnCancel)
	EVT_MENU(XRCID("ID_CANCEL"), CMainFrame::OnCancel)
	EVT_TOOL(XRCID("ID_TOOLBAR_RECONNECT"), CMainFrame::OnReconnect)
	EVT_TOOL(XRCID("ID_MENU_SERVER_RECONNECT"), CMainFrame::OnReconnect)
	EVT_TOOL(XRCID("ID_TOOLBAR_REFRESH"), CMainFrame::OnRefresh)
	EVT_MENU(XRCID("ID_REFRESH"), CMainFrame::OnRefresh)
	EVT_TOOL(XRCID("ID_TOOLBAR_SITEMANAGER"), CMainFrame::OnSiteManager)
	EVT_CLOSE(CMainFrame::OnClose)
#ifdef WITH_LIBDBUS
	EVT_END_SESSION(CMainFrame::OnClose)
#endif
	EVT_TIMER(wxID_ANY, CMainFrame::OnTimer)
	EVT_TOOL(XRCID("ID_TOOLBAR_PROCESSQUEUE"), CMainFrame::OnProcessQueue)
	EVT_TOOL(XRCID("ID_TOOLBAR_LOGVIEW"), CMainFrame::OnToggleLogView)
	EVT_TOOL(XRCID("ID_TOOLBAR_LOCALTREEVIEW"), CMainFrame::OnToggleLocalTreeView)
	EVT_TOOL(XRCID("ID_TOOLBAR_REMOTETREEVIEW"), CMainFrame::OnToggleRemoteTreeView)
	EVT_TOOL(XRCID("ID_TOOLBAR_QUEUEVIEW"), CMainFrame::OnToggleQueueView)
	EVT_MENU(XRCID("ID_VIEW_MESSAGELOG"), CMainFrame::OnToggleLogView)
	EVT_MENU(XRCID("ID_VIEW_LOCALTREE"), CMainFrame::OnToggleLocalTreeView)
	EVT_MENU(XRCID("ID_VIEW_REMOTETREE"), CMainFrame::OnToggleRemoteTreeView)
	EVT_MENU(XRCID("ID_VIEW_QUEUE"), CMainFrame::OnToggleQueueView)
	EVT_MENU(wxID_ABOUT, CMainFrame::OnMenuHelpAbout)
	EVT_TOOL(XRCID("ID_TOOLBAR_FILTER"), CMainFrame::OnFilter)
	EVT_TOOL_RCLICKED(XRCID("ID_TOOLBAR_FILTER"), CMainFrame::OnFilterRightclicked)
#if FZ_MANUALUPDATECHECK
	EVT_MENU(XRCID("ID_CHECKFORUPDATES"), CMainFrame::OnCheckForUpdates)
#endif //FZ_MANUALUPDATECHECK
	EVT_TOOL_RCLICKED(XRCID("ID_TOOLBAR_SITEMANAGER"), CMainFrame::OnSitemanagerDropdown)
#ifdef EVT_TOOL_DROPDOWN
	EVT_TOOL_DROPDOWN(XRCID("ID_TOOLBAR_SITEMANAGER"), CMainFrame::OnSitemanagerDropdown)
#endif
	EVT_NAVIGATION_KEY(CMainFrame::OnNavigationKeyEvent)
	EVT_SET_FOCUS(CMainFrame::OnGetFocus)
	EVT_CHAR_HOOK(CMainFrame::OnChar)
	EVT_MENU(XRCID("ID_MENU_VIEW_FILTERS"), CMainFrame::OnFilter)
	EVT_ACTIVATE(CMainFrame::OnActivate)
	EVT_TOOL(XRCID("ID_TOOLBAR_COMPARISON"), CMainFrame::OnToolbarComparison)
	EVT_TOOL_RCLICKED(XRCID("ID_TOOLBAR_COMPARISON"), CMainFrame::OnToolbarComparisonDropdown)
#ifdef EVT_TOOL_DROPDOWN
	EVT_TOOL_DROPDOWN(XRCID("ID_TOOLBAR_COMPARISON"), CMainFrame::OnToolbarComparisonDropdown)
#endif
	EVT_MENU(XRCID("ID_COMPARE_SIZE"), CMainFrame::OnDropdownComparisonMode)
	EVT_MENU(XRCID("ID_COMPARE_DATE"), CMainFrame::OnDropdownComparisonMode)
	EVT_TOOL(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), CMainFrame::OnSyncBrowse)
END_EVENT_TABLE()

class CMainFrameStateEventHandler : public CStateEventHandler
{
public:
	CMainFrameStateEventHandler(CState* pState, CMainFrame* pMainFrame)
		: CStateEventHandler(pState)
	{
		m_pMainFrame = pMainFrame;

		pState->RegisterHandler(this, STATECHANGE_REMOTE_IDLE);
		pState->RegisterHandler(this, STATECHANGE_SERVER);
		pState->RegisterHandler(this, STATECHANGE_QUEUEPROCESSING);
		pState->RegisterHandler(this, STATECHANGE_SYNC_BROWSE);
	}

protected:
	virtual void OnStateChange(enum t_statechange_notifications notification, const wxString& data)
	{
		if (notification == STATECHANGE_QUEUEPROCESSING)
		{
			const bool check = m_pMainFrame->GetQueue() && m_pMainFrame->GetQueue()->IsActive() != 0;

			wxToolBar* pToolBar = m_pMainFrame->GetToolBar();
			if (pToolBar)
				pToolBar->ToggleTool(XRCID("ID_TOOLBAR_PROCESSQUEUE"), check);

			wxMenuBar* pMenuBar = m_pMainFrame->GetMenuBar();
			if (pMenuBar)
				pMenuBar->Check(XRCID("ID_MENU_TRANSFER_PROCESSQUEUE"), check);
			return;
		}
		if (notification == STATECHANGE_SERVER)
		{
			const CServer* pServer = m_pState->GetServer();

			if (pServer && pServer->GetName() == _T(""))
			{
				// Can only happen through quickconnect bar
				m_pMainFrame->ClearBookmarks();
			}
		}
		if (notification == STATECHANGE_SYNC_BROWSE)
		{
			if (m_pMainFrame->GetToolBar())
				m_pMainFrame->GetToolBar()->ToggleTool(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), m_pState->GetSyncBrowse());
			if (m_pMainFrame->GetMenuBar())
				m_pMainFrame->GetMenuBar()->Check(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), m_pState->GetSyncBrowse());
			return;
		}

		m_pMainFrame->UpdateMenubarState();
		m_pMainFrame->UpdateToolbarState();
	}

	CMainFrame* m_pMainFrame;
};

CMainFrame::CMainFrame()
{
	wxRect screen_size = CWindowStateManager::GetScreenDimensions();

	wxSize initial_size;
	initial_size.x = wxMin(900, screen_size.GetWidth() - 10);
	initial_size.y = wxMin(750, screen_size.GetHeight() - 50);

	Create(NULL, -1, _T("FileZilla"), wxDefaultPosition, initial_size);
	SetSizeHints(250, 250);

#ifdef __WXMSW__
	// In order for the --close commandline argument to work,
	// there has to be a way to find other instances.
	// Create a hidden window with a title no other program uses
	wxWindow* pChild = new wxWindow();
	pChild->Hide();
	pChild->Create(this, wxID_ANY);
	::SetWindowText((HWND)pChild->GetHandle(), _T("FileZilla process identificator 3919DB0A-082D-4560-8E2F-381A35969FB4"));
#endif

#ifdef __WXMSW__
	SetIcon(wxICON(appicon));
#else
	SetIcons(CThemeProvider::GetIconBundle(_T("ART_FILEZILLA")));
#endif

	m_pStatusBar = NULL;
	m_pMenuBar = NULL;
	m_pToolBar = 0;
	m_pQuickconnectBar = NULL;
	m_pTopSplitter = NULL;
	m_pBottomSplitter = NULL;
	m_pViewSplitter = NULL;
	m_pLocalSplitter = NULL;
	m_pRemoteSplitter = NULL;
	m_pQueueLogSplitter = 0;
	m_bInitDone = false;
	m_bQuit = false;
	m_closeEvent = 0;
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	m_pUpdateWizard = 0;
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	m_pQueuePane = 0;
	m_pStatusView = 0;

	m_pThemeProvider = new CThemeProvider();
	m_pState = new CState(this);
	m_pStateEventHandler = new CMainFrameStateEventHandler(m_pState, this);

	m_pStatusBar = new CStatusBar(this);
	if (m_pStatusBar)
	{
		m_pActivityLed[0] = new CLed(m_pStatusBar, 0);
		m_pActivityLed[1] = new CLed(m_pStatusBar, 1);

		m_pStatusBar->AddChild(-1, m_pActivityLed[1], 2);
		m_pStatusBar->AddChild(-1, m_pActivityLed[0], 16);

		SetStatusBar(m_pStatusBar);
	}
	else
	{
		m_pActivityLed[0] = 0;
		m_pActivityLed[1] = 0;
	}

	m_transferStatusTimer.SetOwner(this, TRANSFERSTATUS_TIMER_ID);
	m_closeEventTimer.SetOwner(this);

	if (CFilterManager::HasActiveFilters(true))
	{
		if (COptions::Get()->GetOptionVal(OPTION_FILTERTOGGLESTATE))
			CFilterManager::ToggleFilters();
	}

	m_last_bookmark_path = COptions::Get()->GetOption(OPTION_LAST_CONNECTED_SITE);
	CSiteManager::GetBookmarks(m_last_bookmark_path, m_bookmarks);

	CreateMenus();
	CreateToolBar();
	if (COptions::Get()->GetOptionVal(OPTION_SHOW_QUICKCONNECT))
		CreateQuickconnectBar();

	m_pAsyncRequestQueue = new CAsyncRequestQueue(this);

	if (!m_pState->CreateEngine())
	{
		wxMessageBox(_("Failed to initialize FTP engine"));
	}

	if (m_pActivityLed[0])
	{
		m_pActivityLed[0]->SetEngine(m_pState->m_pEngine);
		m_pActivityLed[1]->SetEngine(m_pState->m_pEngine);
	}

#ifdef __WXMSW__
	long style = wxSP_NOBORDER | wxSP_LIVE_UPDATE;
#else
	long style = wxSP_3DBORDER | wxSP_LIVE_UPDATE;
#endif

	wxSize clientSize = GetClientSize();

	m_pTopSplitter = new CSplitterWindowEx(this, -1, wxDefaultPosition, clientSize, style);
	m_pTopSplitter->SetMinimumPaneSize(50);

	m_pBottomSplitter = new CSplitterWindowEx(m_pTopSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pBottomSplitter->SetMinimumPaneSize(10, 60);
	m_pBottomSplitter->SetSashGravity(1.0);

	m_pViewSplitter = new CSplitterWindowEx(m_pBottomSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pViewSplitter->SetMinimumPaneSize(50, 100);
	m_pViewSplitter->SetSashGravity(0.5);

	m_pLocalSplitter = new CSplitterWindowEx(m_pViewSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pLocalSplitter->SetMinimumPaneSize(50, 100);

	m_pRemoteSplitter = new CSplitterWindowEx(m_pViewSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pRemoteSplitter->SetMinimumPaneSize(50, 100);

	const int message_log_position = COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION);
	m_pQueueLogSplitter = new CSplitterWindowEx(m_pBottomSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pQueueLogSplitter->SetMinimumPaneSize(50, 250);
	m_pQueueLogSplitter->SetSashGravity(0.5);
	m_pQueuePane = new CQueue(m_pQueueLogSplitter, this, m_pAsyncRequestQueue);

	if (message_log_position == 1)
		m_pStatusView = new CStatusView(m_pQueueLogSplitter, -1);
	else
		m_pStatusView = new CStatusView(m_pTopSplitter, -1);

	m_pQueueView = m_pQueuePane->GetQueueView();

	bool show_filelist_statusbars = COptions::Get()->GetOptionVal(OPTION_FILELIST_STATUSBAR) != 0;

	m_pLocalTreeViewPanel = new CView(m_pLocalSplitter);
	m_pLocalListViewPanel = new CView(m_pLocalSplitter);
	m_pLocalTreeView = new CLocalTreeView(m_pLocalTreeViewPanel, -1, m_pState, m_pQueueView);
	m_pLocalListView = new CLocalListView(m_pLocalListViewPanel, m_pState, m_pQueueView);
	m_pLocalTreeViewPanel->SetWindow(m_pLocalTreeView);
	m_pLocalListViewPanel->SetWindow(m_pLocalListView);

	CFilelistStatusBar* pLocalFilelistStatusBar = new CFilelistStatusBar(m_pLocalListViewPanel);
	if (!show_filelist_statusbars)
		pLocalFilelistStatusBar->Hide();
	m_pLocalListViewPanel->SetStatusBar(pLocalFilelistStatusBar);
	m_pLocalListView->SetFilelistStatusBar(pLocalFilelistStatusBar);

	m_pRemoteTreeViewPanel = new CView(m_pRemoteSplitter);
	m_pRemoteListViewPanel = new CView(m_pRemoteSplitter);
	m_pRemoteTreeView = new CRemoteTreeView(m_pRemoteTreeViewPanel, -1, m_pState, m_pQueueView);
	m_pRemoteListView = new CRemoteListView(m_pRemoteListViewPanel, m_pState, m_pQueueView);
	m_pRemoteTreeViewPanel->SetWindow(m_pRemoteTreeView);
	m_pRemoteListViewPanel->SetWindow(m_pRemoteListView);

	CFilelistStatusBar* pRemoteFilelistStatusBar = new CFilelistStatusBar(m_pRemoteListViewPanel);
	if (!show_filelist_statusbars)
		pRemoteFilelistStatusBar->Hide();
	m_pRemoteListViewPanel->SetStatusBar(pRemoteFilelistStatusBar);
	m_pRemoteListView->SetFilelistStatusBar(pRemoteFilelistStatusBar);

	switch (message_log_position)
	{
	case 1:
		m_pTopSplitter->Initialize(m_pBottomSplitter);
		if (COptions::Get()->GetOptionVal(OPTION_SHOW_MESSAGELOG))
		{
			if (COptions::Get()->GetOptionVal(OPTION_SHOW_QUEUE))
				m_pQueueLogSplitter->SplitVertically(m_pQueuePane, m_pStatusView);
			else
			{
				m_pQueueLogSplitter->Initialize(m_pStatusView);
				m_pQueuePane->Hide();
			}
		}
		else
		{
			if (COptions::Get()->GetOptionVal(OPTION_SHOW_QUEUE))
			{
				m_pStatusView->Hide();
				m_pQueueLogSplitter->Initialize(m_pQueuePane);
			}
			else
			{
				m_pQueuePane->Hide();
				m_pStatusView->Hide();
				m_pQueueLogSplitter->Hide();
			}
		}
		break;
	case 2:
		m_pTopSplitter->Initialize(m_pBottomSplitter);
		if (COptions::Get()->GetOptionVal(OPTION_SHOW_QUEUE))
			m_pQueueLogSplitter->Initialize(m_pQueuePane);
		else
		{
			m_pQueueLogSplitter->Hide();
			m_pQueuePane->Hide();
		}
		m_pQueuePane->AddPage(m_pStatusView, _("Message log"));
		break;
	default:
		if (COptions::Get()->GetOptionVal(OPTION_SHOW_QUEUE))
			m_pQueueLogSplitter->Initialize(m_pQueuePane);
		else
		{
			m_pQueuePane->Hide();
			m_pQueueLogSplitter->Hide();
		}
		if (COptions::Get()->GetOptionVal(OPTION_SHOW_MESSAGELOG))
			m_pTopSplitter->SplitHorizontally(m_pStatusView, m_pBottomSplitter);
		else
		{
			m_pStatusView->Hide();
			m_pTopSplitter->Initialize(m_pBottomSplitter);
		}
		break;
	}

	if (m_pQueueLogSplitter->IsShown())
		m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueueLogSplitter);
	else
	{
		m_pQueueLogSplitter->Hide();
		m_pBottomSplitter->Initialize(m_pViewSplitter);
	}

	const int layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);
	const int swap = COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP);

	if (layout == 1)
	{
		if (swap)
			m_pViewSplitter->SplitHorizontally(m_pRemoteSplitter, m_pLocalSplitter);
		else
			m_pViewSplitter->SplitHorizontally(m_pLocalSplitter, m_pRemoteSplitter);
	}
	else
	{
		if (swap)
			m_pViewSplitter->SplitVertically(m_pRemoteSplitter, m_pLocalSplitter);
		else
			m_pViewSplitter->SplitVertically(m_pLocalSplitter, m_pRemoteSplitter);
	}

	m_pLocalViewHeader = new CLocalViewHeader(m_pLocalSplitter, m_pState);
	if (COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_LOCAL))
	{
		m_pLocalTreeViewPanel->SetHeader(m_pLocalViewHeader);
		if (layout == 3 && swap)
			m_pLocalSplitter->SplitVertically(m_pLocalListViewPanel, m_pLocalTreeViewPanel);
		else if (layout)
			m_pLocalSplitter->SplitVertically(m_pLocalTreeViewPanel, m_pLocalListViewPanel);
		else
			m_pLocalSplitter->SplitHorizontally(m_pLocalTreeViewPanel, m_pLocalListViewPanel);
	}
	else
	{
		m_pLocalTreeViewPanel->Hide();
		m_pLocalListViewPanel->SetHeader(m_pLocalViewHeader);
		m_pLocalSplitter->Initialize(m_pLocalListViewPanel);
	}

	m_pRemoteViewHeader = new CRemoteViewHeader(m_pRemoteSplitter, m_pState);
	if (COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_REMOTE))
	{
		m_pRemoteTreeViewPanel->SetHeader(m_pRemoteViewHeader);
		if (layout == 3 && !swap)
			m_pRemoteSplitter->SplitVertically(m_pRemoteListViewPanel, m_pRemoteTreeViewPanel);
		else if (layout)
			m_pRemoteSplitter->SplitVertically(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel);
		else
			m_pRemoteSplitter->SplitHorizontally(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel);
	}
	else
	{
		m_pRemoteTreeViewPanel->Hide();
		m_pRemoteListViewPanel->SetHeader(m_pRemoteViewHeader);
		m_pRemoteSplitter->Initialize(m_pRemoteListViewPanel);
	}

	if (layout == 3)
	{
		if (!swap)
			m_pRemoteSplitter->SetSashGravity(1.0);
		else
			m_pLocalSplitter->SetSashGravity(1.0);
	}

	m_pWindowStateManager = new CWindowStateManager(this);
	m_pWindowStateManager->Restore(OPTION_MAINWINDOW_POSITION);

	Layout();
	HandleResize();

	if (!RestoreSplitterPositions())
		SetDefaultSplitterPositions();

	wxString localDir = COptions::Get()->GetOption(OPTION_LASTLOCALDIR);
	if (!m_pState->SetLocalDir(localDir))
		m_pState->SetLocalDir(_T("/"));

	wxAcceleratorEntry entries[1];
	entries[0].Set(wxACCEL_NORMAL, WXK_F5, XRCID("ID_TOOLBAR_REFRESH"));
	wxAcceleratorTable accel(1, entries);
	SetAcceleratorTable(accel);

#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	if (!COptions::Get()->GetDefaultVal(DEFAULT_DISABLEUPDATECHECK) && COptions::Get()->GetOptionVal(OPTION_UPDATECHECK))
	{
		m_pUpdateWizard = new CUpdateWizard(this);
		m_pUpdateWizard->InitAutoUpdateCheck();
	}
	else
		m_pUpdateWizard = 0;
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

	m_pState->GetRecursiveOperationHandler()->SetQueue(m_pQueueView);

	ConnectNavigationHandler(m_pStatusView);
	ConnectNavigationHandler(m_pLocalListView);
	ConnectNavigationHandler(m_pRemoteListView);
	ConnectNavigationHandler(m_pLocalTreeView);
	ConnectNavigationHandler(m_pRemoteTreeView);
	ConnectNavigationHandler(m_pLocalViewHeader);
	ConnectNavigationHandler(m_pRemoteViewHeader);
	ConnectNavigationHandler(m_pQueuePane);

	wxNavigationKeyEvent evt;
	evt.SetDirection(true);
	AddPendingEvent(evt);

	CEditHandler::Create()->SetQueue(m_pQueueView);

	m_pComparisonManager = 0;

	InitMenubarState();
	InitToolbarState();

	CAutoAsciiFiles::SettingsChanged();
}

CMainFrame::~CMainFrame()
{
	delete m_pStateEventHandler;
	delete m_pState;
	delete m_pAsyncRequestQueue;
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	delete m_pUpdateWizard;
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	delete m_pComparisonManager;

	CEditHandler* pEditHandler = CEditHandler::Get();
	if (pEditHandler)
	{
		// This might leave temporary files behind,
		// edit handler should clean them on next startup
		pEditHandler->Release();
	}
}

void CMainFrame::HandleResize()
{
	wxSize clientSize = GetClientSize();
	if (m_pQuickconnectBar)
		m_pQuickconnectBar->SetSize(0, 0, clientSize.GetWidth(), -1, wxSIZE_USE_EXISTING);
	if (m_pTopSplitter)
	{
		if (!m_pQuickconnectBar)
			m_pTopSplitter->SetSize(0, 0, clientSize.GetWidth(), clientSize.GetHeight());
		else
		{
			wxSize panelSize = m_pQuickconnectBar->GetSize();
			m_pTopSplitter->SetSize(0, panelSize.GetHeight(), clientSize.GetWidth(), clientSize.GetHeight() - panelSize.GetHeight());
		}
	}
}

void CMainFrame::OnSize(wxSizeEvent &event)
{
	wxFrame::OnSize(event);

	if (!m_pBottomSplitter)
		return;

	HandleResize();

#ifdef __WXGTK__
	if (m_pWindowStateManager && m_pWindowStateManager->m_maximize_requested && IsMaximized())
	{
		m_pWindowStateManager->m_maximize_requested = 0;
		if (!RestoreSplitterPositions())
			SetDefaultSplitterPositions();
	}
#endif
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

#if FZ_MANUALUPDATECHECK
	if (COptions::Get()->GetDefaultVal(DEFAULT_DISABLEUPDATECHECK))
#endif
	{
		if (m_pMenuBar)
		{
			wxMenu *helpMenu;

			wxMenuItem* pUpdateItem = m_pMenuBar->FindItem(XRCID("ID_CHECKFORUPDATES"), &helpMenu);
			if (pUpdateItem)
			{
				// Get rid of separator
				unsigned int count = helpMenu->GetMenuItemCount();
				for (unsigned int i = 0; i < count - 1; i++)
				{
					if (helpMenu->FindItemByPosition(i) == pUpdateItem)
					{
						helpMenu->Delete(helpMenu->FindItemByPosition(i + 1));
						break;
					}
				}

				helpMenu->Delete(pUpdateItem);
			}
		}
	}

	if (COptions::Get()->GetOptionVal(OPTION_DEBUG_MENU) && m_pMenuBar)
	{
		wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_DEBUG"));
		if (pMenu)
			m_pMenuBar->Append(pMenu, _("&Debug"));
	}

	if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) == 2)
	{
		wxMenu* pMenu;
		wxMenuItem* pItem = m_pMenuBar->FindItem(XRCID("ID_VIEW_MESSAGELOG"), &pMenu);
		if (pItem)
			pMenu->Delete(pItem);
	}

	SetMenuBar(m_pMenuBar);
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	if (m_pUpdateWizard)
		m_pUpdateWizard->DisplayUpdateAvailability(false, true);
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

	if (m_pMenuBar)
	{
		m_pMenuBar->FindItem(XRCID("ID_MENU_SERVER_VIEWHIDDEN"), 0)->Check(COptions::Get()->GetOptionVal(OPTION_VIEW_HIDDEN_FILES) ? true : false);

		int mode = COptions::Get()->GetOptionVal(OPTION_COMPARISONMODE);
		if (mode != 1)
			m_pMenuBar->Check(XRCID("ID_COMPARE_SIZE"), true);
		else
			m_pMenuBar->Check(XRCID("ID_COMPARE_DATE"), true);
	}

	UpdateBookmarkMenu();

	InitMenubarState();
	UpdateMenubarState();

	return true;
}

bool CMainFrame::CreateQuickconnectBar()
{
	if (m_pQuickconnectBar)
		delete m_pQuickconnectBar;

	m_pQuickconnectBar = new CQuickconnectBar();
	if (!m_pQuickconnectBar->Create(this, m_pState))
	{
		delete m_pQuickconnectBar;
		m_pQuickconnectBar = 0;
	}
	else
	{
		wxSize clientSize = GetClientSize();
		if (m_pTopSplitter)
		{
			wxSize panelSize = m_pQuickconnectBar->GetSize();
			m_pTopSplitter->SetSize(-1, panelSize.GetHeight(), -1, clientSize.GetHeight() - panelSize.GetHeight(), wxSIZE_USE_EXISTING);
		}
		m_pQuickconnectBar->SetSize(0, 0, clientSize.GetWidth(), -1);
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
		OpenSiteManager();
	}
	else if (event.GetId() == XRCID("ID_MENU_FILE_COPYSITEMANAGER"))
	{
		const CServer* pServer = m_pState->GetServer();
		if (!pServer)
		{
			wxMessageBox(_("Not connected to any server."), _("Cannot add server to Site Manager"), wxICON_EXCLAMATION);
			return;
		}
		OpenSiteManager(pServer);
	}
	else if (event.GetId() == XRCID("ID_MENU_SERVER_CMD"))
	{
		if (!m_pState || !m_pState->m_pCommandQueue || !m_pState->IsRemoteConnected() || !m_pState->IsRemoteIdle())
			return;

		CInputDialog dlg;
		dlg.Create(this, _("Enter custom command"), _("Please enter raw FTP command.\nUsing raw ftp commands will clear the directory cache."));
		if (dlg.ShowModal() != wxID_OK)
			return;

		if (!m_pState || !m_pState->m_pCommandQueue || !m_pState->IsRemoteConnected() || !m_pState->IsRemoteIdle())
		{
			wxBell();
			return;
		}

		const wxString &command = dlg.GetValue();

		if (!command.Left(5).CmpNoCase(_T("quote")) || !command.Left(6).CmpNoCase(_T("quote ")))
		{
			CConditionalDialog dlg(this, CConditionalDialog::rawcommand_quote, CConditionalDialog::yesno);
			dlg.SetTitle(_("Raw FTP command"));

			dlg.AddText(_("'quote' is usually a local command used by commandline clients to send the arguments following 'quote' to the server. You might want to enter the raw command without the leading 'quote'."));
			dlg.AddText(wxString::Format(_("Do you really want to send '%s' to the server?"), command.c_str()));

			if (!dlg.Run())
				return;
		}

		m_pState->m_pCommandQueue->ProcessCommand(new CRawCommand(dlg.GetValue()));
	}
	else if (event.GetId() == XRCID("wxID_PREFERENCES"))
	{
		OnMenuEditSettings(event);
	}
	else if (event.GetId() == XRCID("ID_MENU_EDIT_NETCONFWIZARD"))
	{
		CNetConfWizard wizard(this, COptions::Get());
		wizard.Load();
		wizard.Run();
	}
	// Debug menu
	else if (event.GetId() == XRCID("ID_CRASH"))
	{
		// Cause a crash
		int *x = 0;
		*x = 0;
	}
	else if (event.GetId() == XRCID("ID_CLEARCACHE_LAYOUT"))
	{
		CWrapEngine::ClearCache();
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_FILEEXISTS"))
	{
		CDefaultFileExistsDlg dlg;
		if (!dlg.Load(this, false))
			return;

		dlg.Run();
	}
	else if (event.GetId() == XRCID("ID_MENU_EDIT_CLEARPRIVATEDATA"))
	{
		CClearPrivateDataDialog* pDlg = CClearPrivateDataDialog::Create(this);
		if (!pDlg)
			return;

		pDlg->Show();
		pDlg->Delete();

		UpdateMenubarState();
		UpdateToolbarState();
	}
	else if (event.GetId() == XRCID("ID_MENU_SERVER_VIEWHIDDEN"))
	{
		bool showHidden = COptions::Get()->GetOptionVal(OPTION_VIEW_HIDDEN_FILES) ? 0 : 1;
		if (showHidden)
		{
			CConditionalDialog dlg(this, CConditionalDialog::viewhidden, CConditionalDialog::ok, false);
			dlg.SetTitle(_("Force showing hidden files"));

			dlg.AddText(_("Note that this feature is only supported using the FTP protocol."));
			dlg.AddText(_("A proper server always shows all files, but some broken servers hide files from the user. Use this option to force the server to show all files."));
			dlg.AddText(_("Keep in mind that not all servers support this feature and may return incorrect listings if this option is enabled. Although FileZilla performs some tests to check if the server supports this feature, the test may fail."));
			dlg.AddText(_("Disable this option again if you will not be able to see the correct directory contents anymore."));
			dlg.Run();
		}

		COptions::Get()->SetOption(OPTION_VIEW_HIDDEN_FILES, showHidden ? 1 : 0);
		CServerPath path = m_pState->GetRemotePath();
		if (!path.IsEmpty() && m_pState->m_pCommandQueue)
			m_pState->ChangeRemoteDir(path, _T(""), LIST_FLAG_REFRESH);
	}
	else if (event.GetId() == XRCID("ID_EXPORT"))
	{
		CExportDialog dlg(this, m_pQueueView);
		dlg.Show();
	}
	else if (event.GetId() == XRCID("ID_IMPORT"))
	{
		CImportDialog dlg(this, m_pQueueView);
		dlg.Show();
	}
	else if (event.GetId() == XRCID("ID_MENU_FILE_EDITED"))
	{
		CEditHandlerStatusDialog dlg(this);
		dlg.ShowModal();
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_TYPE_AUTO"))
	{
		COptions::Get()->SetOption(OPTION_ASCIIBINARY, 0);
		m_pMenuBar->FindItem(XRCID("ID_MENU_TRANSFER_TYPE_AUTO"))->Check();
		if (m_pStatusBar)
			m_pStatusBar->DisplayDataType(m_pState->GetServer());
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_TYPE_ASCII"))
	{
		COptions::Get()->SetOption(OPTION_ASCIIBINARY, 1);
		m_pMenuBar->FindItem(XRCID("ID_MENU_TRANSFER_TYPE_ASCII"))->Check();
		if (m_pStatusBar)
			m_pStatusBar->DisplayDataType(m_pState->GetServer());
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_TYPE_BINARY"))
	{
		COptions::Get()->SetOption(OPTION_ASCIIBINARY, 2);
		m_pMenuBar->FindItem(XRCID("ID_MENU_TRANSFER_TYPE_BINARY"))->Check();
		if (m_pStatusBar)
			m_pStatusBar->DisplayDataType(m_pState->GetServer());
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_PRESERVETIMES"))
	{
		if (event.IsChecked())
		{
			CConditionalDialog dlg(this, CConditionalDialog::confirm_preserve_timestamps, CConditionalDialog::ok, true);
			dlg.SetTitle(_("Preserving file timestamps"));
			dlg.AddText(_("Please note that preserving timestamps on uploads on FTP, FTPS and FTPES servers only works if they support the MFMT command."));
			dlg.Run();
		}
		COptions::Get()->SetOption(OPTION_PRESERVE_TIMESTAMPS, event.IsChecked() ? 1 : 0);
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_PROCESSQUEUE"))
	{
		if (m_pQueueView)
			m_pQueueView->SetActive(event.IsChecked());
	}
	else if (event.GetId() == XRCID("ID_MENU_HELP_GETTINGHELP") ||
			 event.GetId() == XRCID("ID_MENU_HELP_BUGREPORT"))
	{
		wxString url(_T("http://filezilla-project.org/support.php?type=client&mode="));
		if (event.GetId() == XRCID("ID_MENU_HELP_GETTINGHELP"))
			url += _T("help");
		else
			url += _T("bugreport");
		wxString version = CBuildInfo::GetVersion();
		if (version != _T("custom build"))
		{
			url += _T("&version=");
			// We need to urlencode version number

			// Unbelievable, but wxWidgets does not have any method
			// to urlencode strings.
			// Do a crude approach: Drop everything unexpected...
			for (unsigned int i = 0; i < version.Len(); i++)
			{
				wxChar& c = version[i];
				if ((version[i] >= '0' && version[i] <= '9') ||
					(version[i] >= 'a' && version[i] <= 'z') ||
					(version[i] >= 'A' && version[i] <= 'Z') ||
					version[i] == '-' || version[i] == '.' ||
					version[i] == '_')
				{
					url += c;
				}
			}
		}
		wxLaunchDefaultBrowser(url);
	}
	else if (event.GetId() == XRCID("ID_MENU_VIEW_FILELISTSTATUSBAR"))
	{
		bool show = COptions::Get()->GetOptionVal(OPTION_FILELIST_STATUSBAR) == 0;
		COptions::Get()->SetOption(OPTION_FILELIST_STATUSBAR, show ? 1 : 0);
		if (m_pLocalListViewPanel)
		{
			wxStatusBar* pStatusBar = m_pLocalListViewPanel->GetStatusBar();
			if (pStatusBar)
			{
				pStatusBar->Show(show);
				wxSizeEvent evt;
				m_pLocalListViewPanel->ProcessEvent(evt);
			}
		}
		if (m_pRemoteListViewPanel)
		{
			wxStatusBar* pStatusBar = m_pRemoteListViewPanel->GetStatusBar();
			if (pStatusBar)
			{
				pStatusBar->Show(show);
				wxSizeEvent evt;
				m_pRemoteListViewPanel->ProcessEvent(evt);
			}
		}
	}
	else if (event.GetId() == XRCID("ID_VIEW_QUICKCONNECT"))
	{
		if (!m_pQuickconnectBar)
			CreateQuickconnectBar();
		else
		{
			m_pQuickconnectBar->Destroy();
			m_pQuickconnectBar = 0;
			wxSize clientSize = GetClientSize();
			m_pTopSplitter->SetSize(0, 0, clientSize.GetWidth(), clientSize.GetHeight());
		}
		COptions::Get()->SetOption(OPTION_SHOW_QUICKCONNECT, m_pQuickconnectBar != 0);
		m_pMenuBar->Check(XRCID("ID_VIEW_QUICKCONNECT"), m_pQuickconnectBar != 0);
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_MANUAL"))
	{
		if (!m_pState || !m_pQueueView)
		{
			wxBell();
			return;
		}
		CManualTransfer dlg(m_pQueueView);
		dlg.Show(this, m_pState);
	}
	else if (event.GetId() == XRCID("ID_BOOKMARK_ADD") || event.GetId() == XRCID("ID_BOOKMARK_MANAGE"))
	{
		CServer server;
		const CServer* pServer = m_pState ? m_pState->GetServer() : 0;

		if (!pServer && !m_last_bookmark_path.empty())
		{
			// Get server from site manager
			CSiteManagerItemData_Site* data = CSiteManager::GetSiteByPath(m_last_bookmark_path);
			if (data)
			{
				server = data->m_server;
				pServer = &server;
				delete data;
			}
			else
			{
				m_last_bookmark_path.clear();
				m_bookmarks.clear();
				UpdateBookmarkMenu();
			}
		}

		// m_last_bookmark_path can get modified if it's empty now
		if (event.GetId() == XRCID("ID_BOOKMARK_ADD"))
		{
			CNewBookmarkDialog dlg(this, m_last_bookmark_path, pServer);

			if (dlg.ShowModal(m_pState->GetLocalDir().GetPath(), m_pState->GetRemotePath()) == wxID_OK)
			{
				m_bookmarks.clear();
				CSiteManager::GetBookmarks(m_last_bookmark_path, m_bookmarks);
				UpdateBookmarkMenu();
			}	
		}
		else
		{
			CBookmarksDialog dlg(this, m_last_bookmark_path, pServer);

			if (dlg.ShowModal(m_pState->GetLocalDir().GetPath(), m_pState->GetRemotePath()) == wxID_OK)
			{
				m_bookmarks.clear();
				CSiteManager::GetBookmarks(m_last_bookmark_path, m_bookmarks);
				UpdateBookmarkMenu();
			}	
		}
	}
	else
	{
		std::map<int, wxString>::const_iterator iter = m_bookmark_menu_id_map_site.find(event.GetId());
		if (iter != m_bookmark_menu_id_map_site.end())
		{
			// We hit a site-specific bookmark
			if (m_last_bookmark_path.empty())
				return;

			wxString name = iter->second;
			name.Replace(_T("\\"), _T("\\\\"));
			name.Replace(_T("/"), _T("\\/"));
			name = m_last_bookmark_path + _T("/") + name;

			CSiteManagerItemData_Site *pData = CSiteManager::GetSiteByPath(name);
			if (!pData)
				return;

			m_pState->SetSyncBrowse(false);
			if (!pData->m_remoteDir.IsEmpty() && m_pState->IsRemoteIdle())
			{
				const CServer* pServer = m_pState->GetServer();
				if (!pServer || *pServer != pData->m_server)
				{
					ConnectToSite(pData);
					pData->m_localDir.clear(); // So not to set again below
				}
				else
					m_pState->ChangeRemoteDir(pData->m_remoteDir);
			}
			if (!pData->m_localDir.empty())
			{
				bool set = m_pState->SetLocalDir(pData->m_localDir);

				if (set && pData->m_sync)
				{
					wxASSERT(!pData->m_remoteDir.IsEmpty());
					m_pState->SetSyncBrowse(true, pData->m_remoteDir);
				}
			}

			delete pData;

			return;
		}

		std::map<int, wxString>::const_iterator iter2 = m_bookmark_menu_id_map_global.find(event.GetId());
		if (iter2 != m_bookmark_menu_id_map_global.end())
		{
			// We hit a global bookmark
			wxString local_dir;
			CServerPath remote_dir;
			bool sync;
			if (!CBookmarksDialog::GetBookmark(iter2->second, local_dir, remote_dir, sync))
				return;

			m_pState->SetSyncBrowse(false);
			if (!remote_dir.IsEmpty() && m_pState->IsRemoteIdle())
			{
				const CServer* pServer = m_pState->GetServer();
				if (pServer)
				{
					CServerPath current_remote_path = m_pState->GetRemotePath();
					if (!current_remote_path.IsEmpty() && current_remote_path.GetType() != remote_dir.GetType())
					{
						wxMessageBox(_("Selected global bookmark and current server use a different server type.\nUse site-specific bookmarks for this server."), _("Bookmark"), wxICON_EXCLAMATION, this);
						return;
					}
					m_pState->ChangeRemoteDir(remote_dir);
				}
			}
			if (!local_dir.empty())
			{
				bool set = m_pState->SetLocalDir(local_dir);

				if (set && sync)
				{
					wxASSERT(!remote_dir.IsEmpty());
					m_pState->SetSyncBrowse(true, remote_dir);
				}
			}
		}

		wxString path;
		CSiteManagerItemData_Site* pData = CSiteManager::GetSiteById(event.GetId(), path);

		if (!pData)
		{
			event.Skip();
			return;
		}

		ConnectToSite(pData);
		m_last_bookmark_path = path;
		m_bookmarks.clear();
		CSiteManager::GetBookmarks(m_last_bookmark_path, m_bookmarks);
		UpdateBookmarkMenu();

		delete pData;
	}
}

void CMainFrame::OnMenuOpenHandler(wxMenuEvent& event)
{
	wxMenu* pMenu = event.GetMenu();
	if (!pMenu)
		return;
	wxMenuItem* pItem = pMenu->FindItem(XRCID("ID_MENU_TRANSFER_TYPE"));
	if (!pItem)
		return;

	const CServer* pServer = 0;
	if (m_pState)
		pServer = m_pState->GetServer();

	if (!pServer || CServer::ProtocolHasDataTypeConcept(pServer->GetProtocol()))
	{
		pItem->Enable(true);
		int mode = COptions::Get()->GetOptionVal(OPTION_ASCIIBINARY);
		switch (mode)
		{
		case 1:
			pMenu->FindItem(XRCID("ID_MENU_TRANSFER_TYPE_ASCII"))->Check();
			break;
		case 2:
			pMenu->FindItem(XRCID("ID_MENU_TRANSFER_TYPE_BINARY"))->Check();
			break;
		default:
			pMenu->FindItem(XRCID("ID_MENU_TRANSFER_TYPE_AUTO"))->Check();
			break;
		}
	}
	else
		pItem->Enable(false);

	pMenu->FindItem(XRCID("ID_MENU_TRANSFER_PRESERVETIMES"))->Check(COptions::Get()->GetOptionVal(OPTION_PRESERVE_TIMESTAMPS) ? true : false);
}

void CMainFrame::OnEngineEvent(wxEvent &event)
{
	if (!m_pState->m_pEngine)
		return;

	CNotification *pNotification = m_pState->m_pEngine->GetNextNotification();
	while (pNotification)
	{
		switch (pNotification->GetID())
		{
		case nId_logmsg:
			m_pStatusView->AddToLog(reinterpret_cast<CLogmsgNotification *>(pNotification));
			if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) == 2)
				m_pQueuePane->Highlight(3);
			delete pNotification;
			break;
		case nId_operation:
			m_pState->m_pCommandQueue->Finish(reinterpret_cast<COperationNotification*>(pNotification));
			if (m_bQuit)
			{
				Close();
				return;
			}
			break;
		case nId_listing:
			{
				const CDirectoryListingNotification* const pListingNotification = reinterpret_cast<CDirectoryListingNotification *>(pNotification);

				if (pListingNotification->GetPath().IsEmpty())
					m_pState->SetRemoteDir(0, false);
				else
				{
					CDirectoryListing* pListing = new CDirectoryListing;
					if (pListingNotification->Failed() ||
						m_pState->m_pEngine->CacheLookup(pListingNotification->GetPath(), *pListing) != FZ_REPLY_OK)
					{
						delete pListing;
						pListing = new CDirectoryListing;
						pListing->path = pListingNotification->GetPath();
						pListing->m_failed = true;
						pListing->m_firstListTime = CTimeEx::Now();
					}

					m_pState->SetRemoteDir(pListing, pListingNotification->Modified());
				}
			}
			delete pNotification;
			break;
		case nId_asyncrequest:
			{
				CAsyncRequestNotification* pAsyncRequest = reinterpret_cast<CAsyncRequestNotification *>(pNotification);
				if (pAsyncRequest->GetRequestID() == reqId_fileexists)
					m_pQueueView->ProcessNotification(pNotification);
				else
				{
					if (pAsyncRequest->GetRequestID() == reqId_certificate && m_pStatusBar)
						m_pStatusBar->SetCertificate((CCertificateNotification*)pNotification);
					m_pAsyncRequestQueue->AddRequest(m_pState->m_pEngine, pAsyncRequest);
				}
			}
			break;
		case nId_active:
			{
				CActiveNotification *pActiveNotification = reinterpret_cast<CActiveNotification *>(pNotification);
				UpdateActivityLed(pActiveNotification->GetDirection());
				delete pNotification;
			}
			break;
		case nId_transferstatus:
			{
				m_pQueueView->ProcessNotification(pNotification);
				/*
				CTransferStatusNotification *pTransferStatusNotification = reinterpret_cast<CTransferStatusNotification *>(pNotification);
				const CTransferStatus *pStatus = pTransferStatusNotification ? pTransferStatusNotification->GetStatus() : 0;
				if (pStatus && !m_transferStatusTimer.IsRunning())
					m_transferStatusTimer.Start(100);
				else if (!pStatus && m_transferStatusTimer.IsRunning())
					m_transferStatusTimer.Stop();

				SetProgress(pStatus);
				delete pNotification;*/
			}
			break;
		case nId_sftp_encryption:
			{
				if (m_pStatusBar)
					m_pStatusBar->SetSftpEncryptionInfo(reinterpret_cast<CSftpEncryptionNotification*>(pNotification));
				delete pNotification;
			}
			break;
		case nId_local_dir_created:
			if (m_pState)
			{
				CLocalDirCreatedNotification *pLocalDirCreatedNotification = reinterpret_cast<CLocalDirCreatedNotification *>(pNotification);
				m_pState->LocalDirCreated(pLocalDirCreatedNotification->dir);
			}
			delete pNotification;
			break;
		default:
			delete pNotification;
			break;
		}

		pNotification = m_pState->m_pEngine->GetNextNotification();
	}
}

#if defined(EVT_TOOL_DROPDOWN) && defined(__WXMSW__)
void CMainFrame::MakeDropdownTool(wxToolBar* pToolBar, int id)
{
	wxToolBarToolBase* pOldTool = pToolBar->FindById(id);
	if (!pOldTool)
		return;

	wxToolBarToolBase* pTool = new wxToolBarToolBase(0, id,
		pOldTool->GetLabel(), pOldTool->GetNormalBitmap(), pOldTool->GetDisabledBitmap(),
		wxITEM_DROPDOWN, NULL, pOldTool->GetShortHelp(), pOldTool->GetLongHelp());

	int pos = pToolBar->GetToolPos(id);
	wxASSERT(pos != wxNOT_FOUND);

	pToolBar->DeleteToolByPos(pos);
	pToolBar->InsertTool(pos, pTool);
	pToolBar->Realize();
}
#endif

bool CMainFrame::CreateToolBar()
{
	if (m_pToolBar)
	{
		SetToolBar(0);
		delete m_pToolBar;
	}

	{
		wxSize iconSize(16, 16);
		wxString str = COptions::Get()->GetOption(OPTION_THEME_ICONSIZE);
		int pos = str.Find('x');
		if (CThemeProvider::ThemeHasSize(COptions::Get()->GetOption(OPTION_THEME), str) && pos > 0 && pos < (int)str.Len() - 1)
		{
			long width = 0;
			long height = 0;
			if (str.Left(pos).ToLong(&width) &&
				str.Mid(pos + 1).ToLong(&height) &&
				width > 0 && height > 0)
				iconSize = wxSize(width, height);
		}

		wxToolBarXmlHandlerEx::SetIconSize(iconSize);
	}

	m_pToolBar = wxXmlResource::Get()->LoadToolBar(this, _T("ID_TOOLBAR"));
	if (!m_pToolBar)
	{
		wxLogError(_("Cannot load toolbar from resource file"));
		return false;
	}

#if defined(EVT_TOOL_DROPDOWN) && defined(__WXMSW__)
	MakeDropdownTool(m_pToolBar, XRCID("ID_TOOLBAR_SITEMANAGER"));
	//MakeDropdownTool(m_pToolBar, XRCID("ID_TOOLBAR_COMPARISON"));
#endif

#ifdef __WXMSW__
	int majorVersion, minorVersion;
	wxGetOsVersion(& majorVersion, & minorVersion);
	if (majorVersion < 6)
		m_pToolBar->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
#endif

	if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) == 2)
		m_pToolBar->DeleteTool(XRCID("ID_TOOLBAR_LOGVIEW"));

	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), CFilterManager::HasActiveFilters());
	SetToolBar(m_pToolBar);

	if (m_pQuickconnectBar)
		m_pQuickconnectBar->Refresh();

	InitToolbarState();
	UpdateToolbarState();

	return true;
}

void CMainFrame::OnDisconnect(wxCommandEvent& event)
{
	if (!m_pState->IsRemoteConnected())
		return;

	if (!m_pState->IsRemoteIdle())
		return;

	m_pState->m_pCommandQueue->ProcessCommand(new CDisconnectCommand());
}

void CMainFrame::OnCancel(wxCommandEvent& event)
{
	if (m_pState->m_pCommandQueue->Idle())
		return;

	if (wxMessageBox(_("Really cancel current operation?"), _T("FileZilla"), wxYES_NO | wxICON_QUESTION) == wxYES)
	{
		m_pState->m_pCommandQueue->Cancel();
		m_pState->GetRecursiveOperationHandler()->StopRecursiveOperation();
	}
}

#ifdef __WXMSW__

BOOL CALLBACK FzEnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
	// This function enumerates all dialogs and calls EndDialog for them
	TCHAR buffer[10];
	int c = GetClassName(hwnd, buffer, 9);
	// #32770 is the dialog window class.
	if (c && !_tcscmp(buffer, _T("#32770")))
	{
		*((bool*)lParam) = true;
		EndDialog(hwnd, IDCANCEL);
		return FALSE;
	}

	return TRUE;
}
#endif //__WXMSW__

void CMainFrame::OnClose(wxCloseEvent &event)
{
	if (!m_bQuit)
	{
		static bool quit_confirmation_displayed = false;
		if (quit_confirmation_displayed && event.CanVeto())
		{
			event.Veto();
			return;
		}
		if (event.CanVeto())
		{
			quit_confirmation_displayed = true;

			if (m_pQueueView && m_pQueueView->IsActive())
			{
				CConditionalDialog dlg(this, CConditionalDialog::confirmexit, CConditionalDialog::yesno);
				dlg.SetTitle(_("Close FileZilla"));

				dlg.AddText(_("File transfers still in progress."));
				dlg.AddText(_("Do you really want to close FileZilla?"));

				if (!dlg.Run())
				{
					event.Veto();
					quit_confirmation_displayed = false;
					return;
				}
				if (m_bQuit)
					return;
			}

			CEditHandler* pEditHandler = CEditHandler::Get();
			if (pEditHandler)
			{
				if (pEditHandler->GetFileCount(CEditHandler::remote, CEditHandler::edit) || pEditHandler->GetFileCount(CEditHandler::none, CEditHandler::upload) ||
					pEditHandler->GetFileCount(CEditHandler::none, CEditHandler::upload_and_remove) ||
					pEditHandler->GetFileCount(CEditHandler::none, CEditHandler::upload_and_remove_failed))
				{
					CConditionalDialog dlg(this, CConditionalDialog::confirmexit_edit, CConditionalDialog::yesno);
					dlg.SetTitle(_("Close FileZilla"));

					dlg.AddText(_("Some files are still being edited or need to be uploaded."));
					dlg.AddText(_("If you close FileZilla, your changes will be lost."));
					dlg.AddText(_("Do you really want to close FileZilla?"));

					if (!dlg.Run())
					{
						event.Veto();
						quit_confirmation_displayed = false;
						return;
					}
					if (m_bQuit)
						return;
				}
			}
			quit_confirmation_displayed = false;
		}

		if (!event.CanVeto())
		{
			// We need to close all other top level windows on the stack before closing the main frame.
			// In other words, all open dialogs need to be closed.
			static int prev_size = 0;

			int size = wxTopLevelWindows.size();
			static wxTopLevelWindow* pLast = 0;
			wxWindowList::reverse_iterator iter = wxTopLevelWindows.rbegin();
			wxTopLevelWindow* pTop = (wxTopLevelWindow*)(*iter);
			while (pTop != this && (size != prev_size || pLast != pTop))
			{
				wxDialog* pDialog = wxDynamicCast(pTop, wxDialog);
				if (pDialog)
					pDialog->EndModal(wxID_CANCEL);
				else
				{
					wxWindow* pParent = pTop->GetParent();
					if (m_pQueuePane && pParent == m_pQueuePane)
					{
						// It's the AUI frame manager hint window. Ignore it
						iter++;
						pTop = (wxTopLevelWindow*)(*iter);
						continue;
					}
					wxString title = pTop->GetTitle();
					pTop->Destroy();
				}

				prev_size = size;
				pLast = pTop;

				m_closeEvent = event.GetEventType();
				m_closeEventTimer.Start(1, true);

				return;
			}

#ifdef __WXMSW__
			// wxMessageBox does not use wxTopLevelWindow, close it too
			bool dialog = false;
			EnumThreadWindows(GetCurrentThreadId(), FzEnumThreadWndProc, (LPARAM)&dialog);
			if (dialog)
			{
				m_closeEvent = event.GetEventType();
				m_closeEventTimer.Start(1, true);

				return;
			}
#endif //__WXMSW__

			// At this point all other top level windows should be closed.
		}

		if (m_pWindowStateManager)
		{
			m_pWindowStateManager->Remember(OPTION_MAINWINDOW_POSITION);
			delete m_pWindowStateManager;
			m_pWindowStateManager = 0;
		}

		RememberSplitterPositions();
		m_bQuit = true;
	}

	Show(false);

	// Getting deleted by wxWidgets
	for (int i = 0; i < 2; i++)
	{
		if (!m_pActivityLed[i])
			continue;

		m_pActivityLed[i]->SetEngine(0);
		m_pActivityLed[i] = 0;
	}
	m_pStatusBar = 0;
	m_pMenuBar = 0;
	m_pToolBar = 0;

	// We're no longer interested in these events
	delete m_pStateEventHandler;
	m_pStateEventHandler = 0;

	m_transferStatusTimer.Stop();

	if (!m_pQueueView->Quit())
	{
		event.Veto();
		return;
	}

	CEditHandler* pEditHandler = CEditHandler::Get();
	if (pEditHandler)
	{
		pEditHandler->RemoveAll(true);
		pEditHandler->Release();
	}

	bool res = true;
	if (m_pState->m_pCommandQueue)
		res = m_pState->m_pCommandQueue->Cancel();

	if (!res)
	{
		event.Veto();
		return;
	}

	m_pState->DestroyEngine();

	CSiteManager::ClearIdMap();

	m_pLocalListView->SaveColumnSettings(OPTION_LOCALFILELIST_COLUMN_WIDTHS, OPTION_LOCALFILELIST_COLUMN_SHOWN, OPTION_LOCALFILELIST_COLUMN_ORDER);
	m_pRemoteListView->SaveColumnSettings(OPTION_REMOTEFILELIST_COLUMN_WIDTHS, OPTION_REMOTEFILELIST_COLUMN_SHOWN, OPTION_REMOTEFILELIST_COLUMN_ORDER);

	bool filters_toggled = CFilterManager::HasActiveFilters(true) && !CFilterManager::HasActiveFilters(false);
	COptions::Get()->SetOption(OPTION_FILTERTOGGLESTATE, filters_toggled ? 1 : 0);

	COptions::Get()->SetOption(OPTION_LAST_CONNECTED_SITE, m_last_bookmark_path);

	Destroy();
}

void CMainFrame::OnReconnect(wxCommandEvent &event)
{
	if (!m_pState)
		return;

	if (m_pState->IsRemoteConnected() || !m_pState->IsRemoteIdle())
		return;

	CServer server;
	if (!COptions::Get()->GetLastServer(server))
		return;

	if (server.GetLogonType() == ASK)
	{
		if (!CLoginManager::Get().GetPassword(server, false))
			return;
	}

	CServerPath path;
	path.SetSafePath(COptions::Get()->GetOption(OPTION_LASTSERVERPATH));
	m_pState->Connect(server, false, path);
}

void CMainFrame::OnRefresh(wxCommandEvent &event)
{
	if (!m_pState)
		return;

	if (m_pState->m_pCommandQueue && m_pState->IsRemoteConnected() && m_pState->IsRemoteIdle())
		m_pState->ChangeRemoteDir(m_pState->GetRemotePath(), _T(""), LIST_FLAG_REFRESH);

	m_pState->RefreshLocal();
}

void CMainFrame::OnTimer(wxTimerEvent& event)
{
	if (event.GetId() == TRANSFERSTATUS_TIMER_ID && m_transferStatusTimer.IsRunning())
	{
		if (!m_pState->m_pEngine)
		{
			m_transferStatusTimer.Stop();
			return;
		}

		bool changed;
		CTransferStatus status;
		if (!m_pState->m_pEngine->GetTransferStatus(status, changed))
		{
			SetProgress(0);
			m_transferStatusTimer.Stop();
		}
		else if (changed)
			SetProgress(&status);
		else
			m_transferStatusTimer.Stop();
	}
	else if (event.GetId() == m_closeEventTimer.GetId())
	{
		if (m_closeEvent == 0)
			return;

		// When we get idle event, a dialog's event loop has been left.
		// Now we can close the top level window on the stack.
		wxCloseEvent evt(m_closeEvent);
		evt.SetCanVeto(false);
		AddPendingEvent(evt);
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

void CMainFrame::OpenSiteManager(const CServer* pServer /*=0*/)
{
	CSiteManager dlg;

	if (!dlg.Create(this, m_last_bookmark_path, pServer))
		return;

	int res = dlg.ShowModal();
	if (res == wxID_YES)
	{
		CSiteManagerItemData_Site data;
		if (!dlg.GetServer(data))
			return;

		if (ConnectToSite(&data))
		{
			m_last_bookmark_path = dlg.GetSitePath();
		}
	}
	else if (res == wxID_OK)
		m_last_bookmark_path = dlg.GetChangedBookmarkPath(m_pState->GetServer());

	m_bookmarks.clear();
	dlg.GetBookmarks(m_last_bookmark_path, m_bookmarks);
	UpdateBookmarkMenu();
}

void CMainFrame::OnSiteManager(wxCommandEvent& event)
{
	OpenSiteManager();
}

void CMainFrame::UpdateActivityLed(int direction)
{
	if (m_pActivityLed[direction])
		m_pActivityLed[direction]->Ping();
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

void CMainFrame::OnMenuEditSettings(wxCommandEvent& event)
{
	CSettingsDialog dlg;
	if (!dlg.Create(this))
		return;

	COptions* pOptions = COptions::Get();

	wxString oldTheme = pOptions->GetOption(OPTION_THEME);
	wxString oldThemeSize = pOptions->GetOption(OPTION_THEME_ICONSIZE);
	wxString oldLang = pOptions->GetOption(OPTION_LANGUAGE);

	int oldShowDebugMenu = pOptions->GetOptionVal(OPTION_DEBUG_MENU) != 0;

	bool oldTimestamps = pOptions->GetOptionVal(OPTION_MESSAGELOG_TIMESTAMP) != 0;

	int res = dlg.ShowModal();
	if (res != wxID_OK)
	{
		UpdateLayout();
		return;
	}

	bool newTimestamps = pOptions->GetOptionVal(OPTION_MESSAGELOG_TIMESTAMP) != 0;

	wxString newTheme = pOptions->GetOption(OPTION_THEME);
	wxString newThemeSize = pOptions->GetOption(OPTION_THEME_ICONSIZE);
	wxString newLang = pOptions->GetOption(OPTION_LANGUAGE);

	if (oldTheme != newTheme)
	{
		wxArtProvider::Delete(m_pThemeProvider);
		m_pThemeProvider = new CThemeProvider();
	}
	if (oldTheme != newTheme ||
		oldThemeSize != newThemeSize ||
		oldLang != newLang)
		CreateToolBar();

	if (oldLang != newLang ||
		oldTimestamps != newTimestamps)
	{
		m_pStatusView->InitDefAttr();
	}

	if (oldLang != newLang ||
		oldShowDebugMenu != pOptions->GetOptionVal(OPTION_DEBUG_MENU))
	{
		CreateMenus();
	}
	if (oldLang != newLang)
	{
#ifdef __WXGTK__
		wxMessageBox(_("FileZilla needs to be restarted for the language change to take effect."), _("Language changed"), wxICON_INFORMATION, this);
#else
		CreateQuickconnectBar();
		wxGetApp().GetWrapEngine()->CheckLanguage();
#endif
	}

	CheckChangedSettings();

	if (m_pStatusBar)
	{
		m_pStatusBar->DisplayDataType(m_pState->GetServer());
		m_pStatusBar->UpdateSizeFormat();
	}
}

void CMainFrame::OnToggleLogView(wxCommandEvent& event)
{
	if (!m_pTopSplitter)
		return;

	bool shown;

	if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) == 1)
	{
		if (!m_pQueueLogSplitter)
			return;
		if (m_pQueueLogSplitter->IsSplit())
		{
			m_pQueueLogSplitter->Unsplit(m_pStatusView);
			shown = false;
		}
		else if (m_pStatusView->IsShown())
		{
			m_pStatusView->Hide();
			m_pBottomSplitter->Unsplit(m_pQueueLogSplitter);
			shown = false;
		}
		else if (!m_pQueueLogSplitter->IsShown())
		{
			m_pQueueLogSplitter->Initialize(m_pStatusView);
			m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueueLogSplitter);
			shown = true;
		}
		else
		{
			m_pQueueLogSplitter->SplitVertically(m_pQueuePane, m_pStatusView);
			shown = true;
		}
	}
	else
	{
		if (m_pTopSplitter->IsSplit())
		{
			m_pTopSplitter->Unsplit(m_pStatusView);
			shown = false;
		}
		else
		{
			m_pTopSplitter->SplitHorizontally(m_pStatusView, m_pBottomSplitter);
			shown = true;
		}
	}
	
	if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) != 2)
		COptions::Get()->SetOption(OPTION_SHOW_MESSAGELOG, shown);

	if (m_pMenuBar)
		m_pMenuBar->Check(XRCID("ID_VIEW_MESSAGELOG"), shown);
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_LOGVIEW"), shown);
}

void CMainFrame::OnToggleLocalTreeView(wxCommandEvent& event)
{
	if (!m_pTopSplitter)
		return;

	if (m_pLocalSplitter->IsSplit())
	{
		m_pLocalListViewPanel->SetHeader(m_pLocalTreeViewPanel->DetachHeader());
		m_pLocalSplitter->Unsplit(m_pLocalTreeViewPanel);
	}
	else
		ShowLocalTree();

	COptions::Get()->SetOption(OPTION_SHOW_TREE_LOCAL, m_pLocalSplitter->IsSplit());

	if (m_pMenuBar)
		m_pMenuBar->Check(XRCID("ID_VIEW_LOCALTREE"), m_pLocalSplitter->IsSplit());
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_LOCALTREEVIEW"), m_pLocalSplitter->IsSplit());
}

void CMainFrame::ShowLocalTree()
{
	if (m_pLocalSplitter->IsSplit())
		return;

	m_pLocalTreeViewPanel->SetHeader(m_pLocalListViewPanel->DetachHeader());
	wxSize size = m_pLocalSplitter->GetClientSize();
	const int layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);
	const int swap = COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP);
	if (layout == 3 && swap)
		m_pLocalSplitter->SplitVertically(m_pLocalListViewPanel, m_pLocalTreeViewPanel);
	else if (layout)
		m_pLocalSplitter->SplitVertically(m_pLocalTreeViewPanel, m_pLocalListViewPanel);
	else
		m_pLocalSplitter->SplitHorizontally(m_pLocalTreeViewPanel, m_pLocalListViewPanel);
}

void CMainFrame::OnToggleRemoteTreeView(wxCommandEvent& event)
{
	if (!m_pTopSplitter)
		return;

	if (m_pRemoteSplitter->IsSplit())
	{
		m_pRemoteListViewPanel->SetHeader(m_pRemoteTreeViewPanel->DetachHeader());
		m_pRemoteSplitter->Unsplit(m_pRemoteTreeViewPanel);
	}
	else
		ShowRemoteTree();

	COptions::Get()->SetOption(OPTION_SHOW_TREE_REMOTE, m_pRemoteSplitter->IsSplit());

	if (m_pMenuBar)
		m_pMenuBar->Check(XRCID("ID_VIEW_REMOTETREE"), m_pRemoteSplitter->IsSplit());
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_REMOTETREEVIEW"), m_pRemoteSplitter->IsSplit());
}

void CMainFrame::ShowRemoteTree()
{
	if (m_pRemoteSplitter->IsSplit())
		return;

	m_pRemoteTreeViewPanel->SetHeader(m_pRemoteListViewPanel->DetachHeader());
	wxSize size = m_pRemoteSplitter->GetClientSize();
	const int layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);
	const int swap = COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP);
	if (layout == 3 && !swap)
		m_pRemoteSplitter->SplitVertically(m_pRemoteListViewPanel, m_pRemoteTreeViewPanel);
	else if (layout)
		m_pRemoteSplitter->SplitVertically(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel);
	else
		m_pRemoteSplitter->SplitHorizontally(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel);
}

void CMainFrame::OnToggleQueueView(wxCommandEvent& event)
{
	if (!m_pBottomSplitter)
		return;

	bool shown;
	if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) == 1)
	{
		if (!m_pQueueLogSplitter)
			return;
		if (m_pQueueLogSplitter->IsSplit())
		{
			m_pQueueLogSplitter->Unsplit(m_pQueuePane);
			shown = false;
		}
		else if (m_pQueuePane->IsShown())
		{
			m_pQueuePane->Hide();
			m_pBottomSplitter->Unsplit(m_pQueueLogSplitter);
			shown = false;
		}
		else if (!m_pQueueLogSplitter->IsShown())
		{
			m_pQueueLogSplitter->Initialize(m_pQueuePane);
			m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueueLogSplitter);
			shown = true;
		}
		else
		{
			m_pQueueLogSplitter->SplitVertically(m_pQueuePane, m_pStatusView);
			shown = true;
		}
	}
	else
	{
		if (m_pBottomSplitter->IsSplit())
		{
			wxRect rect = m_pBottomSplitter->GetClientSize();
			m_pBottomSplitter->Unsplit(m_pQueueLogSplitter);
		}
		else
		{
			wxRect rect = m_pBottomSplitter->GetClientSize();
			m_pQueueLogSplitter->Initialize(m_pQueuePane);
			m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueueLogSplitter);
		}
		shown = m_pBottomSplitter->IsSplit();
	}

	COptions::Get()->SetOption(OPTION_SHOW_QUEUE, shown);

	if (m_pMenuBar)
		m_pMenuBar->Check(XRCID("ID_VIEW_QUEUE"), shown);
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_QUEUEVIEW"), shown);
}

void CMainFrame::OnMenuHelpAbout(wxCommandEvent& event)
{
	CAboutDialog dlg;
	if (!dlg.Create(this))
		return;

	dlg.ShowModal();
}

void CMainFrame::OnFilter(wxCommandEvent& event)
{
	CFilterDialog dlg;
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), dlg.HasActiveFilters());
	dlg.Create(this);
	dlg.ShowModal();
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), dlg.HasActiveFilters());
	m_pState->NotifyHandlers(STATECHANGE_APPLYFILTER);
}

#if FZ_MANUALUPDATECHECK
void CMainFrame::OnCheckForUpdates(wxCommandEvent& event)
{
	wxString version(PACKAGE_VERSION, wxConvLocal);
	if (version[0] < '0' || version[0] > '9')
	{
		wxMessageBox(_("Executable contains no version info, cannot check for updates."), _("Check for updates failed"), wxICON_ERROR, this);
		return;
	}

	CUpdateWizard dlg(this);
	if (!dlg.Load())
		return;

	dlg.Run();
}
#endif //FZ_MANUALUPDATECHECK

void CMainFrame::UpdateLayout(int layout /*=-1*/, int swap /*=-1*/, int messagelog_position /*=-1*/)
{
	if (layout == -1)
		layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);
	if (swap == -1)
		swap = COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP);

	bool final;
	if (messagelog_position == -1)
	{
		final = true;
		messagelog_position = COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION);
	}
	else
		final = false;

	// First handle changes in message log position as it can make size of the other panes change
	{
		bool shown = m_pStatusView->IsShown();
		wxWindow* parent = m_pStatusView->GetParent();
	
		bool changed;
		if (parent == m_pTopSplitter && messagelog_position != 0)
		{
			if (shown)
				m_pTopSplitter->Unsplit(m_pStatusView);
			changed = true;
		}
		else if (parent == m_pQueueLogSplitter && messagelog_position != 1)
		{
			if (shown)
			{
				if (m_pQueueLogSplitter->IsSplit())
					m_pQueueLogSplitter->Unsplit(m_pStatusView);
				else
					m_pBottomSplitter->Unsplit(m_pQueueLogSplitter);
			}
			changed = true;
		}
		else if (parent != m_pTopSplitter && parent != m_pQueueLogSplitter && messagelog_position != 2)
		{
			m_pQueuePane->RemovePage(3);
			changed = true;
			shown = true;
		}
		else
			changed = false;

		if (changed)
		{
			switch (messagelog_position)
			{
			default:
				m_pStatusView->Reparent(m_pTopSplitter);
				if (shown)
					m_pTopSplitter->SplitHorizontally(m_pStatusView, m_pBottomSplitter);
				break;
			case 1:
				m_pStatusView->Reparent(m_pQueueLogSplitter);
				if (shown)
				{
					if (m_pQueueLogSplitter->IsShown())
						m_pQueueLogSplitter->SplitVertically(m_pQueuePane, m_pStatusView);
					else
					{
						m_pQueueLogSplitter->Initialize(m_pStatusView);
						m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueueLogSplitter);
					}
				}
				break;
			case 2:
				m_pQueuePane->AddPage(m_pStatusView, _("Message log"));
				break;
			}
		}

		bool has_messagelog_button = m_pToolBar && m_pToolBar->FindById(XRCID("ID_TOOLBAR_LOGVIEW"));
		if (final && 
			((has_messagelog_button && messagelog_position == 2) ||
			 (!has_messagelog_button && messagelog_position != 2)))
		{
			CreateMenus();
			CreateToolBar();
		}
	}

	// Now the other panes
	int mode;
	if (!layout || layout == 2 || layout == 3)
		mode = wxSPLIT_VERTICAL;
	else
		mode = wxSPLIT_HORIZONTAL;

	int isMode = m_pViewSplitter->GetSplitMode();

	int isSwap = m_pViewSplitter->GetWindow1() == m_pRemoteSplitter ? 1 : 0;

	if (mode != isMode || swap != isSwap)
	{
		m_pViewSplitter->Unsplit();
		if (mode == wxSPLIT_VERTICAL)
		{
			if (swap)
				m_pViewSplitter->SplitVertically(m_pRemoteSplitter, m_pLocalSplitter);
			else
				m_pViewSplitter->SplitVertically(m_pLocalSplitter, m_pRemoteSplitter);
		}
		else
		{
			if (swap)
				m_pViewSplitter->SplitHorizontally(m_pRemoteSplitter, m_pLocalSplitter);
			else
				m_pViewSplitter->SplitHorizontally(m_pLocalSplitter, m_pRemoteSplitter);
		}
	}

	if (m_pLocalSplitter->IsSplit())
	{
		if (!layout)
			mode = wxSPLIT_HORIZONTAL;
		else
			mode = wxSPLIT_VERTICAL;

		wxWindow* pFirst;
		wxWindow* pSecond;
		if (layout == 3 && swap)
		{
			pFirst = m_pLocalListViewPanel;
			pSecond = m_pLocalTreeViewPanel;
		}
		else
		{
			pFirst = m_pLocalTreeViewPanel;
			pSecond = m_pLocalListViewPanel;
		}

		if (mode != m_pLocalSplitter->GetSplitMode() || pFirst != m_pLocalSplitter->GetWindow1())
		{
			m_pLocalSplitter->Unsplit();
			if (mode == wxSPLIT_VERTICAL)
				m_pLocalSplitter->SplitVertically(pFirst, pSecond);
			else
				m_pLocalSplitter->SplitHorizontally(pFirst, pSecond);
		}
	}

	if (m_pRemoteSplitter->IsSplit())
	{
		if (!layout)
			mode = wxSPLIT_HORIZONTAL;
		else
			mode = wxSPLIT_VERTICAL;

		wxWindow* pFirst;
		wxWindow* pSecond;
		if (layout == 3 && !swap)
		{
			pFirst = m_pRemoteListViewPanel;
			pSecond = m_pRemoteTreeViewPanel;
		}
		else
		{
			pFirst = m_pRemoteTreeViewPanel;
			pSecond = m_pRemoteListViewPanel;
		}

		if (mode != m_pRemoteSplitter->GetSplitMode() || pFirst != m_pRemoteSplitter->GetWindow1())
		{
			m_pRemoteSplitter->Unsplit();
			if (mode == wxSPLIT_VERTICAL)
				m_pRemoteSplitter->SplitVertically(pFirst, pSecond);
			else
				m_pRemoteSplitter->SplitHorizontally(pFirst, pSecond);
		}
	}

	if (layout == 3)
	{
		if (!swap)
		{
			m_pRemoteSplitter->SetSashGravity(1.0);
			m_pLocalSplitter->SetSashGravity(0.0);
		}
		else
		{
			m_pLocalSplitter->SetSashGravity(1.0);
			m_pRemoteSplitter->SetSashGravity(0.0);
		}
	}
	else
	{
		m_pLocalSplitter->SetSashGravity(0.0);
		m_pRemoteSplitter->SetSashGravity(0.0);
	}
}

void CMainFrame::OnSitemanagerDropdown(wxCommandEvent& event)
{
	wxMenu *pMenu = CSiteManager::GetSitesMenu();
	if (!pMenu)
		return;

	ShowDropdownMenu(pMenu, m_pToolBar, event);
}

bool CMainFrame::ConnectToSite(CSiteManagerItemData_Site* const pData)
{
	wxASSERT(pData);

	if (pData->m_server.GetLogonType() == ASK ||
		(pData->m_server.GetLogonType() == INTERACTIVE && pData->m_server.GetUser() == _T("")))
	{
		if (!CLoginManager::Get().GetPassword(pData->m_server, false, pData->m_server.GetName()))
			return false;
	}

	if (!m_pState->Connect(pData->m_server, true, pData->m_remoteDir))
		return false;

	if (pData->m_localDir != _T(""))
	{
		bool set = m_pState->SetLocalDir(pData->m_localDir);

		if (set && pData->m_sync)
		{
			wxASSERT(!pData->m_remoteDir.IsEmpty());

			m_pState->SetSyncBrowse(true, pData->m_remoteDir);
		}
	}

	return true;
}

void CMainFrame::CheckChangedSettings()
{
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	if (!COptions::Get()->GetDefaultVal(DEFAULT_DISABLEUPDATECHECK) && COptions::Get()->GetOptionVal(OPTION_UPDATECHECK))
	{
		if (!m_pUpdateWizard)
		{
			m_pUpdateWizard = new CUpdateWizard(this);
			m_pUpdateWizard->InitAutoUpdateCheck();
		}
	}
	else
	{
		if (m_pUpdateWizard)
		{
			delete m_pUpdateWizard;
			m_pUpdateWizard = 0;
		}
	}
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

	UpdateLayout();

	m_pAsyncRequestQueue->RecheckDefaults();

	m_pLocalListView->InitDateFormat();
	m_pRemoteListView->InitDateFormat();

	m_pQueueView->SettingsChanged();
	CAutoAsciiFiles::SettingsChanged();
}

void CMainFrame::ConnectNavigationHandler(wxEvtHandler* handler)
{
	if (!handler)
		return;

	handler->Connect(wxEVT_NAVIGATION_KEY, wxNavigationKeyEventHandler(CMainFrame::OnNavigationKeyEvent), 0, this);
}

void CMainFrame::OnNavigationKeyEvent(wxNavigationKeyEvent& event)
{
	std::list<wxWindow*> windowOrder;
	if (m_pQuickconnectBar)
		windowOrder.push_back(m_pQuickconnectBar);
	if (m_pStatusView)
		windowOrder.push_back(m_pStatusView);
	if (COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP) == 0)
	{
		windowOrder.push_back(m_pLocalViewHeader);
		windowOrder.push_back(m_pLocalTreeView);
		windowOrder.push_back(m_pLocalListView);
		windowOrder.push_back(m_pRemoteViewHeader);
		windowOrder.push_back(m_pRemoteTreeView);
		windowOrder.push_back(m_pRemoteListView);
	}
	else
	{
		windowOrder.push_back(m_pRemoteViewHeader);
		windowOrder.push_back(m_pRemoteTreeView);
		windowOrder.push_back(m_pRemoteListView);
		windowOrder.push_back(m_pLocalViewHeader);
		windowOrder.push_back(m_pLocalTreeView);
		windowOrder.push_back(m_pLocalListView);
	}
	windowOrder.push_back(m_pQueuePane);

	std::list<wxWindow*>::iterator iter;
	for (iter = windowOrder.begin(); iter != windowOrder.end(); iter++)
	{
		if (*iter == event.GetEventObject())
			break;
	}

	bool skipFirst;
	if (iter == windowOrder.end())
	{
		iter = windowOrder.begin();
		skipFirst = false;
	}
	else
		skipFirst = true;

	FocusNextEnabled(windowOrder, iter, skipFirst, event.GetDirection());
}

void CMainFrame::OnGetFocus(wxFocusEvent& event)
{
	wxNavigationKeyEvent evt;
	OnNavigationKeyEvent(evt);
}

void CMainFrame::OnChar(wxKeyEvent& event)
{
	if (event.GetKeyCode() != WXK_F6)
	{
		event.Skip();
		return;
	}

	// Jump between quickconnect bar and view headers

	std::list<wxWindow*> windowOrder;
	if (m_pQuickconnectBar)
		windowOrder.push_back(m_pQuickconnectBar);
	windowOrder.push_back(m_pLocalViewHeader);
	windowOrder.push_back(m_pRemoteViewHeader);

	wxWindow* focused = FindFocus();

	bool skipFirst = false;
	std::list<wxWindow*>::iterator iter;
	if (!focused)
	{
		iter = windowOrder.begin();
		skipFirst = false;
	}
	else
	{
		wxWindow *parent = focused->GetParent();
		for (iter = windowOrder.begin(); iter != windowOrder.end(); iter++)
		{
			if (*iter == focused || *iter == parent)
			{
				skipFirst = true;
				break;
			}
		}
		if (iter == windowOrder.end())
		{
			iter = windowOrder.begin();
			skipFirst = false;
		}
	}

	FocusNextEnabled(windowOrder, iter, skipFirst, !event.ShiftDown());
}

void CMainFrame::FocusNextEnabled(std::list<wxWindow*>& windowOrder, std::list<wxWindow*>::iterator iter, bool skipFirst, bool forward)
{
	std::list<wxWindow*>::iterator start = iter;

	while (skipFirst || !(*iter)->IsShownOnScreen() || !(*iter)->IsEnabled())
	{
		skipFirst = false;
		if (forward)
		{
			iter++;
			if (iter == windowOrder.end())
				iter = windowOrder.begin();
		}
		else
		{
			if (iter == windowOrder.begin())
				iter = windowOrder.end();
			iter--;
		}

		if (iter == start)
		{
			wxBell();
			return;
		}
	}

	(*iter)->SetFocus();
}

void CMainFrame::RememberSplitterPositions()
{
	wxString posString;

	// top_pos
	posString += wxString::Format(_T("%d "), m_pTopSplitter->GetSashPosition());

	// bottom_height
	posString += wxString::Format(_T("%d "), m_pBottomSplitter->GetSashPosition());

	// view_pos
	posString += wxString::Format(_T("%d "), (int)(m_pViewSplitter->GetRelativeSashPosition() * 1000000000));

	// local_pos
	posString += wxString::Format(_T("%d "), m_pLocalSplitter->GetSashPosition());

	// remote_pos
	posString += wxString::Format(_T("%d "), m_pRemoteSplitter->GetSashPosition());

	// queuelog splitter
	// Note that we cannot use %f, it is locale-dependent
	// m_lastQueueLogSplitterPos is a value between 0 and 1
	posString += wxString::Format(_T("%d"), (int)(m_pQueueLogSplitter->GetRelativeSashPosition() * 1000000000));

	COptions::Get()->SetOption(OPTION_MAINWINDOW_SPLITTER_POSITION, posString);
}

bool CMainFrame::RestoreSplitterPositions()
{
	if (wxGetKeyState(WXK_SHIFT) && wxGetKeyState(WXK_ALT) && wxGetKeyState(WXK_CONTROL))
		return false;

	// top_pos bottom_height view_pos view_height_width local_pos remote_pos
	wxString posString = COptions::Get()->GetOption(OPTION_MAINWINDOW_SPLITTER_POSITION);
	wxStringTokenizer tokens(posString, _T(" "));
	int count = tokens.CountTokens();
	if (count < 6)
		return false;

	long * aPosValues = new long[count];
	for (int i = 0; i < count; i++)
	{
		wxString token = tokens.GetNextToken();
		if (!token.ToLong(aPosValues + i))
		{
			delete [] aPosValues;
			return false;
		}
	}

	m_pTopSplitter->SetSashPosition(aPosValues[0]);

	m_pBottomSplitter->SetSashPosition(aPosValues[1]);

	double pos = (double)aPosValues[2] / 1000000000;
	if (pos >= 0 && pos <= 1)
		m_pViewSplitter->SetRelativeSashPosition(pos);

	m_pLocalSplitter->SetSashPosition(aPosValues[3]);
	m_pRemoteSplitter->SetSashPosition(aPosValues[4]);

	pos = (double)aPosValues[5] / 1000000000;
	if (pos >= 0 && pos <= 1)
		m_pQueueLogSplitter->SetRelativeSashPosition(pos);
	delete [] aPosValues;

	return true;
}

void CMainFrame::SetDefaultSplitterPositions()
{
	m_pTopSplitter->SetSashPosition(97);

	m_pViewSplitter->SetSashPosition(0);

	wxSize size = m_pBottomSplitter->GetClientSize();
	int h = size.GetHeight() - 135;
	if (h < 50)
		h = 50;
	m_pBottomSplitter->SetSashPosition(h);

	m_pQueueLogSplitter->SetSashPosition(0);

	m_pLocalSplitter->SetRelativeSashPosition(0.4);
	m_pRemoteSplitter->SetRelativeSashPosition(0.4);
}

void CMainFrame::OnActivate(wxActivateEvent& event)
{
	// According to the wx docs we should do this
	event.Skip();

	if (!event.GetActive())
		return;

	CEditHandler* pEditHandler = CEditHandler::Get();
	if (pEditHandler)
		pEditHandler->CheckForModifications(
#ifdef __WXMAC__
				true
#endif
			);
}

void CMainFrame::OnToolbarComparison(wxCommandEvent& event)
{
	if (m_pComparisonManager && m_pComparisonManager->IsComparing())
	{
		m_pComparisonManager->ExitComparisonMode();
		return;
	}

	if (!m_pComparisonManager)
		m_pComparisonManager = new CComparisonManager(this, m_pLocalListView, m_pRemoteListView);

	if (!COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT))
	{
		if ((m_pLocalSplitter->IsSplit() && !m_pRemoteSplitter->IsSplit()) ||
			(!m_pLocalSplitter->IsSplit() && m_pRemoteSplitter->IsSplit()))
		{
			CConditionalDialog dlg(this, CConditionalDialog::compare_treeviewmismatch, CConditionalDialog::yesno);
			dlg.SetTitle(_("Directory comparison"));
			dlg.AddText(_("To compare directories, both file lists have to be aligned."));
			dlg.AddText(_("To do this, the directory trees need to be both shown or both hidden."));
			dlg.AddText(_("Show both directory trees and continue comparing?"));
			if (!dlg.Run())
			{
				m_pComparisonManager->UpdateToolState();
				return;
			}

			ShowLocalTree();
			ShowRemoteTree();
		}
		int pos = (m_pLocalSplitter->GetSashPosition() + m_pRemoteSplitter->GetSashPosition()) / 2;
		m_pLocalSplitter->SetSashPosition(pos);
		m_pRemoteSplitter->SetSashPosition(pos);
	}

	m_pComparisonManager->CompareListings();
}

void CMainFrame::OnToolbarComparisonDropdown(wxCommandEvent& event)
{
	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_TOOLBAR_COMPARISON_DROPDOWN"));
	if (!pMenu)
		return;

	pMenu->FindItem(XRCID("ID_TOOLBAR_COMPARISON"))->Check(m_pComparisonManager && m_pComparisonManager->IsComparing());

	const int mode = COptions::Get()->GetOptionVal(OPTION_COMPARISONMODE);
	if (mode == 0)
		pMenu->FindItem(XRCID("ID_COMPARE_SIZE"))->Check();
	else
		pMenu->FindItem(XRCID("ID_COMPARE_DATE"))->Check();

	ShowDropdownMenu(pMenu, m_pToolBar, event);
}

void CMainFrame::ShowDropdownMenu(wxMenu* pMenu, wxToolBar* pToolBar, wxCommandEvent& event)
{
	#ifdef EVT_TOOL_DROPDOWN
	if (event.GetEventType() == wxEVT_COMMAND_TOOL_DROPDOWN_CLICKED)
	{
		pToolBar->SetDropdownMenu(event.GetId(), pMenu);
		event.Skip();
	}
	else
#endif
	{
#ifdef __WXMSW__
		RECT r;
        if (::SendMessage((HWND)pToolBar->GetHandle(), TB_GETITEMRECT, pToolBar->GetToolPos(event.GetId()), (LPARAM)&r))
			pToolBar->PopupMenu(pMenu, r.left, r.bottom);
		else
#endif
			pToolBar->PopupMenu(pMenu);

		delete pMenu;
	}
}

void CMainFrame::OnDropdownComparisonMode(wxCommandEvent& event)
{
	int old_mode = COptions::Get()->GetOptionVal(OPTION_COMPARISONMODE);
	int new_mode = (event.GetId() == XRCID("ID_COMPARE_SIZE")) ? 0 : 1;
	COptions::Get()->SetOption(OPTION_COMPARISONMODE, new_mode);

	if (m_pMenuBar)
	{
		if (new_mode != 1)
			m_pMenuBar->Check(XRCID("ID_COMPARE_SIZE"), true);
		else
			m_pMenuBar->Check(XRCID("ID_COMPARE_DATE"), true);
	}

	if (old_mode != new_mode && m_pComparisonManager && m_pComparisonManager->IsComparing())
		m_pComparisonManager->CompareListings();
}

void CMainFrame::InitToolbarState()
{
	if (!m_pToolBar)
		return;
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_LOGVIEW"), m_pStatusView && m_pStatusView->IsShown());
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_LOCALTREEVIEW"), m_pLocalSplitter && m_pLocalSplitter->IsSplit());
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_REMOTETREEVIEW"), m_pRemoteSplitter && m_pRemoteSplitter->IsSplit());
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_QUEUEVIEW"), m_pQueuePane && m_pQueuePane->IsShown());
}

void CMainFrame::UpdateToolbarState()
{
	if (!m_pToolBar)
		return;
	wxASSERT(m_pState);

	const CServer* pServer = m_pState->GetServer();
	const bool idle = m_pState->IsRemoteIdle();

	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_DISCONNECT"), pServer && idle);
	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_CANCEL"), pServer && !idle);
	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_COMPARISON"), pServer != 0);
	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pServer != 0);

	bool canReconnect;
	if (pServer || !idle)
		canReconnect = false;
	else
	{
		CServer tmp;
		canReconnect = COptions::Get()->GetLastServer(tmp);
	}
	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_RECONNECT"), canReconnect);
}

void CMainFrame::InitMenubarState()
{
	if (!m_pMenuBar)
		return;
	m_pMenuBar->Check(XRCID("ID_VIEW_QUICKCONNECT"), m_pQuickconnectBar != 0);
	if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) != 2)
		m_pMenuBar->Check(XRCID("ID_VIEW_MESSAGELOG"), m_pStatusView && m_pStatusView->IsShown());
	m_pMenuBar->Check(XRCID("ID_VIEW_LOCALTREE"), m_pLocalSplitter && m_pLocalSplitter->IsSplit());
	m_pMenuBar->Check(XRCID("ID_VIEW_REMOTETREE"), m_pRemoteSplitter && m_pRemoteSplitter->IsSplit());
	m_pMenuBar->Check(XRCID("ID_VIEW_QUEUE"), m_pQueuePane && m_pQueuePane->IsShown());
	m_pMenuBar->Check(XRCID("ID_MENU_VIEW_FILELISTSTATUSBAR"), COptions::Get()->GetOptionVal(OPTION_FILELIST_STATUSBAR) != 0);
}

void CMainFrame::UpdateMenubarState()
{
	if (!m_pMenuBar)
		return;
	wxASSERT(m_pState);

	const CServer* const pServer = m_pState->GetServer();
	const bool idle = m_pState->IsRemoteIdle();

	m_pMenuBar->Enable(XRCID("ID_MENU_SERVER_DISCONNECT"), pServer && idle);
	m_pMenuBar->Enable(XRCID("ID_CANCEL"), pServer && !idle);
	m_pMenuBar->Enable(XRCID("ID_MENU_SERVER_CMD"), pServer && idle);
	m_pMenuBar->Enable(XRCID("ID_MENU_FILE_COPYSITEMANAGER"), pServer != 0);
	m_pMenuBar->Enable(XRCID("ID_TOOLBAR_COMPARISON"), pServer != 0);
	m_pMenuBar->Enable(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pServer != 0);

	bool canReconnect;
	if (pServer || !idle)
		canReconnect = false;
	else
	{
		CServer tmp;
		canReconnect = COptions::Get()->GetLastServer(tmp);
	}
	m_pMenuBar->Enable(XRCID("ID_MENU_SERVER_RECONNECT"), canReconnect);
}

void CMainFrame::ProcessCommandLine()
{
	const CCommandLine* pCommandLine = wxGetApp().GetCommandLine();
	if (!pCommandLine)
		return;

	wxString site;
	if (pCommandLine->HasSwitch(CCommandLine::sitemanager))
	{
		Show();
		OpenSiteManager();
	}
	else if ((site = pCommandLine->GetOption(CCommandLine::site)) != _T(""))
	{
		CSiteManagerItemData_Site* pData = CSiteManager::GetSiteByPath(site);

		if (pData)
		{
            if (ConnectToSite(pData))
			{
				m_last_bookmark_path = site;
				m_bookmarks.clear();
				CSiteManager::GetBookmarks(m_last_bookmark_path, m_bookmarks);
				UpdateBookmarkMenu();
			}
			delete pData;
		}
	}

	wxString param = pCommandLine->GetParameter();
	if (param != _T(""))
	{
		wxString error;

		CServer server;
		CServerPath path;
		if (!server.ParseUrl(param, 0, _T(""), _T(""), error, path))
		{
			wxString str = _("Parameter not a valid URL");
			str += _T("\n") + error;
			wxMessageBox(error, _("Syntax error in command line"));
		}

		m_pState->Connect(server, true, path);
	}
}

void CMainFrame::OnFilterRightclicked(wxCommandEvent& event)
{
	const bool active = CFilterManager::HasActiveFilters();

	CFilterManager::ToggleFilters();

	if (active == CFilterManager::HasActiveFilters())
		return;

	if (m_pState)
		m_pState->NotifyHandlers(STATECHANGE_APPLYFILTER);
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), !active);
}

#ifdef __WXMSW__
WXLRESULT CMainFrame::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
	if (nMsg == WM_DEVICECHANGE)
	{
		// Let tree control handle device change message
		// They get sent by Window on adding or removing drive
		// letters

		if (m_pLocalTreeView)
			m_pLocalTreeView->OnDevicechange(wParam, lParam);
		return 0;
	}
	else if (nMsg == WM_DISPLAYCHANGE)
	{
		// wxDisplay caches the display configuration and does not
		// reset it if the display configuration changes.
		// wxDisplay uses this strange factory design pattern and uses a wxModule
		// to delete the factory on program shutdown.
		//
		// To reset the factory manually in response to WM_DISPLAYCHANGE,
		// create another instance of the module and call its Exit() member.
		// After that, the next call to a wxDisplay will create a new factory and
		// get the new display layout from Windows.
		// 
		// Note: Both the factory pattern as well as the dynamic object system
		//       are perfect example of bad design.
		//
		wxModule* pDisplayModule = (wxModule*)wxCreateDynamicObject(_T("wxDisplayModule"));
		if (pDisplayModule)
		{
			pDisplayModule->Exit();
			delete pDisplayModule;
		}
	}
	return wxFrame::MSWWindowProc(nMsg, wParam, lParam);
}
#endif

void CMainFrame::UpdateBookmarkMenu()
{
	if (!m_pMenuBar)
		return;

	wxMenu* pMenu;
	if (!m_pMenuBar->FindItem(XRCID("ID_BOOKMARK_ADD"), &pMenu))
		return;

	// Delete old bookmarks
	for (std::map<int, wxString>::const_iterator iter = m_bookmark_menu_id_map_global.begin(); iter != m_bookmark_menu_id_map_global.end(); iter++)
	{
		pMenu->Delete(iter->first);
	}
	for (std::map<int, wxString>::const_iterator iter = m_bookmark_menu_id_map_site.begin(); iter != m_bookmark_menu_id_map_site.end(); iter++)
	{
		pMenu->Delete(iter->first);
	}

	// Delete the separators
	while (pMenu->GetMenuItemCount() > 2)
	{
		wxMenuItem* pSeparator = pMenu->FindItemByPosition(2);
		if (pSeparator)
			pMenu->Delete(pSeparator);
	}

	std::list<int>::iterator ids = m_bookmark_menu_ids.begin();

	// Insert global bookmarks
	std::list<wxString> global_bookmarks;
	if (CBookmarksDialog::GetBookmarks(global_bookmarks) && !global_bookmarks.empty())
	{
		pMenu->AppendSeparator();

		for (std::list<wxString>::const_iterator iter = global_bookmarks.begin(); iter != global_bookmarks.end(); iter++)
		{
			int id;
			if (ids == m_bookmark_menu_ids.end())
			{
				id = wxNewId();
				m_bookmark_menu_ids.push_back(id);
			}
			else
			{
				id = *ids;
				ids++;
			}
			pMenu->Append(id, *iter);

			m_bookmark_menu_id_map_global[id] = *iter;
		}
	}

	// Insert site-specific bookmarks
	if (!m_bookmarks.empty())
		pMenu->AppendSeparator();

	m_bookmark_menu_id_map_site.clear();

	for (std::list<wxString>::const_iterator iter = m_bookmarks.begin(); iter != m_bookmarks.end(); iter++)
	{
		int id;
		if (ids == m_bookmark_menu_ids.end())
		{
			id = wxNewId();
			m_bookmark_menu_ids.push_back(id);
		}
		else
		{
			id = *ids;
			ids++;
		}
		pMenu->Append(id, *iter);

		m_bookmark_menu_id_map_site[id] = *iter;
	}
}

void CMainFrame::ClearBookmarks()
{
	m_last_bookmark_path.clear();
	m_bookmarks.clear();
	UpdateBookmarkMenu();
}

void CMainFrame::OnSyncBrowse(wxCommandEvent& event)
{
	if (!m_pState)
		return;

	m_pState->SetSyncBrowse(!m_pState->GetSyncBrowse());

	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), m_pState->GetSyncBrowse());
}
