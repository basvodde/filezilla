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
#include "settings/settingsdialog.h"
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
#ifndef __WXMAC__
#include <wx/taskbar.h>
#endif
#include "search.h"
#include "power_management.h"
#include "welcome_dialog.h"
#include "context_control.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef __WXGTK__
DECLARE_EVENT_TYPE(fzEVT_TASKBAR_CLICK_DELAYED, -1);
DEFINE_EVENT_TYPE(fzEVT_TASKBAR_CLICK_DELAYED);
#endif

static int tab_hotkey_ids[10];

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
	EVT_MENU(XRCID("ID_COMPARE_HIDEIDENTICAL"), CMainFrame::OnDropdownComparisonHide)
	EVT_TOOL(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), CMainFrame::OnSyncBrowse)
#ifndef __WXMAC__
	EVT_ICONIZE(CMainFrame::OnIconize)
#endif
#ifdef __WXGTK__
	EVT_COMMAND(wxID_ANY, fzEVT_TASKBAR_CLICK_DELAYED, CMainFrame::OnTaskBarClick_Delayed)
#endif
	EVT_TOOL(XRCID("ID_TOOLBAR_FIND"), CMainFrame::OnSearch)
	EVT_MENU(XRCID("ID_MENU_SERVER_SEARCH"), CMainFrame::OnSearch)
	EVT_MENU(XRCID("ID_MENU_FILE_NEWTAB"), CMainFrame::OnMenuNewTab)
	EVT_MENU(XRCID("ID_MENU_FILE_CLOSETAB"), CMainFrame::OnMenuCloseTab)
END_EVENT_TABLE()

class CMainFrameStateEventHandler : public CStateEventHandler
{
public:
	CMainFrameStateEventHandler(CMainFrame* pMainFrame)
		: CStateEventHandler(0)
	{
		m_pMainFrame = pMainFrame;

		CContextManager::Get()->RegisterHandler(this, STATECHANGE_REMOTE_IDLE, false, true);
		CContextManager::Get()->RegisterHandler(this, STATECHANGE_SERVER, false, true);
		CContextManager::Get()->RegisterHandler(this, STATECHANGE_SYNC_BROWSE, true, true);
		CContextManager::Get()->RegisterHandler(this, STATECHANGE_COMPARISON, true, true);

		CContextManager::Get()->RegisterHandler(this, STATECHANGE_QUEUEPROCESSING, false, false);
		CContextManager::Get()->RegisterHandler(this, STATECHANGE_CHANGEDCONTEXT, false, false);
	}

protected:
	virtual void OnStateChange(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2)
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
		else if (notification == STATECHANGE_CHANGEDCONTEXT)
		{
			m_pMainFrame->UpdateMenubarState();
			m_pMainFrame->UpdateToolbarState();
			m_pMainFrame->UpdateBookmarkMenu();
	
			// Update window title
			const CServer* pServer = pState ? pState->GetServer() : 0;
			if (!pServer)
				m_pMainFrame->SetTitle(_T("FileZilla"));
			else
				m_pMainFrame->SetTitle(pState->GetTitle() + _T(" - FileZilla"));

			// Update UI state
			CStatusBar* const pStatusBar = m_pMainFrame->GetStatusBar();
			if (pStatusBar)
			{
				pStatusBar->DisplayDataType(pServer);
				pStatusBar->DisplayEncrypted(pServer);
			}

			return;
		}

		if (!pState)
			return;

		CContextControl::_context_controls* controls = m_pMainFrame->m_pContextControl->GetControlsFromState(pState);
		if (!controls)
			return;

		if (controls->tab_index == -1)
		{
			if (notification == STATECHANGE_REMOTE_IDLE || notification == STATECHANGE_SERVER)
				pState->Disconnect();

			return;
		}

		if (notification == STATECHANGE_SERVER)
		{
			const CServer* pServer = pState->GetServer();

			if (pState == CContextManager::Get()->GetCurrentContext())
			{
				if (!pServer)
					m_pMainFrame->SetTitle(_T("FileZilla"));
				else
				{
					m_pMainFrame->SetTitle(pState->GetTitle() + _T(" - FileZilla"));
					if (pServer->GetName() == _T(""))
					{
						// Can only happen through quickconnect bar
						m_pMainFrame->ClearBookmarks();
					}
				}
			}

			if (pState == CContextManager::Get()->GetCurrentContext())
			{
				CStatusBar* const pStatusBar = m_pMainFrame->GetStatusBar();
				if (pStatusBar)
				{
					pStatusBar->DisplayDataType(pServer);
					pStatusBar->DisplayEncrypted(pServer);
				}

				m_pMainFrame->UpdateMenubarState();
				m_pMainFrame->UpdateToolbarState();
			}
			return;
		}

		if (pState != CContextManager::Get()->GetCurrentContext())
			return;

		if (notification == STATECHANGE_SYNC_BROWSE)
		{
			if (m_pMainFrame->GetToolBar())
				m_pMainFrame->GetToolBar()->ToggleTool(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pState->GetSyncBrowse());
			if (m_pMainFrame->GetMenuBar())
				m_pMainFrame->GetMenuBar()->Check(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pState->GetSyncBrowse());
			return;
		}
		else if (notification == STATECHANGE_COMPARISON)
		{
			bool is_comparing = pState->GetComparisonManager()->IsComparing();
			wxToolBar* pToolBar = m_pMainFrame->GetToolBar();
			if (pToolBar)
				pToolBar->ToggleTool(XRCID("ID_TOOLBAR_COMPARISON"), is_comparing);

			wxMenuBar* pMenuBar = m_pMainFrame->GetMenuBar();
			if (pMenuBar)
				pMenuBar->Check(XRCID("ID_TOOLBAR_COMPARISON"), is_comparing);
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

#ifndef __WXMAC__
	m_taskBarIcon = 0;
#endif
#ifdef __WXGTK__
	m_taskbar_is_uniconizing = 0;
#endif


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

	m_pContextControl = 0;
	m_pStatusBar = NULL;
	m_pMenuBar = NULL;
	m_pToolBar = 0;
	m_pQuickconnectBar = NULL;
	m_pTopSplitter = NULL;
	m_pBottomSplitter = NULL;
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

	CPowerManagement::Create(this);

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

	m_transferStatusTimer.SetOwner(this);
	m_closeEventTimer.SetOwner(this);

	if (CFilterManager::HasActiveFilters(true))
	{
		if (COptions::Get()->GetOptionVal(OPTION_FILTERTOGGLESTATE))
			CFilterManager::ToggleFilters();
	}

	CreateMenus();
	CreateToolBar();
	if (COptions::Get()->GetOptionVal(OPTION_SHOW_QUICKCONNECT))
		CreateQuickconnectBar();

	m_pAsyncRequestQueue = new CAsyncRequestQueue(this);

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

	// It's important that the context control gets created before our own state handler
	// so that contextchange events can be processed in the right order.
	m_pContextControl = new CContextControl(this, m_pBottomSplitter);

	m_pStateEventHandler = new CMainFrameStateEventHandler(this);

	m_pContextControl->CreateTab();

	CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
	if (controls)
	{
		controls->site_bookmarks->path = COptions::Get()->GetOption(OPTION_LAST_CONNECTED_SITE);
		CSiteManager::GetBookmarks(controls->site_bookmarks->path,
								   controls->site_bookmarks->bookmarks);
	}
	UpdateBookmarkMenu();

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
		m_pBottomSplitter->SplitHorizontally(m_pContextControl, m_pQueueLogSplitter);
	else
	{
		m_pQueueLogSplitter->Hide();
		m_pBottomSplitter->Initialize(m_pContextControl);
	}

	m_pWindowStateManager = new CWindowStateManager(this);
	m_pWindowStateManager->Restore(OPTION_MAINWINDOW_POSITION);

	Layout();
	HandleResize();

	if (!RestoreSplitterPositions())
		SetDefaultSplitterPositions();

	wxAcceleratorEntry entries[11];
	entries[0].Set(wxACCEL_CMD | wxACCEL_SHIFT, 'I', XRCID("ID_MENU_VIEW_FILTERS"));
	for (int i = 0; i < 10; i++)
	{
		tab_hotkey_ids[i] = wxNewId();
		entries[i + 1].Set(wxACCEL_CMD, (int)'0' + i, tab_hotkey_ids[i]);
	}

	wxAcceleratorTable accel(sizeof(entries) / sizeof(wxAcceleratorEntry), entries);
	SetAcceleratorTable(accel);

	ConnectNavigationHandler(m_pStatusView);
	ConnectNavigationHandler(m_pQueuePane);

	wxNavigationKeyEvent evt;
	evt.SetDirection(true);
	AddPendingEvent(evt);

	CEditHandler::Create()->SetQueue(m_pQueueView);

	InitMenubarState();
	InitToolbarState();

	CAutoAsciiFiles::SettingsChanged();
}

CMainFrame::~CMainFrame()
{
	CPowerManagement::Destroy();

	delete m_pStateEventHandler;

	CContextManager::Get()->DestroyAllStates();
	delete m_pAsyncRequestQueue;
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	delete m_pUpdateWizard;
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

	CEditHandler* pEditHandler = CEditHandler::Get();
	if (pEditHandler)
	{
		// This might leave temporary files behind,
		// edit handler should clean them on next startup
		pEditHandler->Release();
	}

#ifndef __WXMAC__
	delete m_taskBarIcon;
#endif
}

void CMainFrame::HandleResize()
{
	wxSize clientSize = GetClientSize();
	if (clientSize.y <= 0) // Can happen if restoring from tray on XP if using ugly XP themes
		return;

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
		m_bookmark_menu_id_map_global.clear();
		m_bookmark_menu_id_map_site.clear();
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
		m_pUpdateWizard->DisplayUpdateAvailability(false);
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

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
	if (!m_pQuickconnectBar->Create(this))
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
		CState* pState = CContextManager::Get()->GetCurrentContext();
		const CServer* pServer = pState ? pState->GetServer() : 0;
		if (!pServer)
		{
			wxMessageBox(_("Not connected to any server."), _("Cannot add server to Site Manager"), wxICON_EXCLAMATION);
			return;
		}
		OpenSiteManager(pServer);
	}
	else if (event.GetId() == XRCID("ID_MENU_SERVER_CMD"))
	{
		CState* pState = CContextManager::Get()->GetCurrentContext();
		if (!pState || !pState->m_pCommandQueue || !pState->IsRemoteConnected() || !pState->IsRemoteIdle())
			return;

		CInputDialog dlg;
		dlg.Create(this, _("Enter custom command"), _("Please enter raw FTP command.\nUsing raw ftp commands will clear the directory cache."));
		if (dlg.ShowModal() != wxID_OK)
			return;

		pState = CContextManager::Get()->GetCurrentContext();
		if (!pState || !pState->m_pCommandQueue || !pState->IsRemoteConnected() || !pState->IsRemoteIdle())
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

		pState = CContextManager::Get()->GetCurrentContext();
		if (!pState || !pState->m_pCommandQueue || !pState->IsRemoteConnected() || !pState->IsRemoteIdle())
		{
			wxBell();
			return;
		}
		pState->m_pCommandQueue->ProcessCommand(new CRawCommand(dlg.GetValue()));
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
		const std::vector<CState*> *pStates = CContextManager::Get()->GetAllStates();
		for (std::vector<CState*>::const_iterator iter = pStates->begin(); iter != pStates->end(); iter++)
		{
			CState* pState = *iter;
			CServerPath path = pState->GetRemotePath();
			if (!path.IsEmpty() && pState->m_pCommandQueue)
				pState->ChangeRemoteDir(path, _T(""), LIST_FLAG_REFRESH);
		}
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
		CState* pState = CContextManager::Get()->GetCurrentContext();
		if (m_pStatusBar && pState)
			m_pStatusBar->DisplayDataType(pState->GetServer());
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_TYPE_ASCII"))
	{
		COptions::Get()->SetOption(OPTION_ASCIIBINARY, 1);
		m_pMenuBar->FindItem(XRCID("ID_MENU_TRANSFER_TYPE_ASCII"))->Check();
		CState* pState = CContextManager::Get()->GetCurrentContext();
		if (m_pStatusBar && pState)
			m_pStatusBar->DisplayDataType(pState->GetServer());
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_TYPE_BINARY"))
	{
		COptions::Get()->SetOption(OPTION_ASCIIBINARY, 2);
		m_pMenuBar->FindItem(XRCID("ID_MENU_TRANSFER_TYPE_BINARY"))->Check();
		CState* pState = CContextManager::Get()->GetCurrentContext();
		if (m_pStatusBar && pState)
			m_pStatusBar->DisplayDataType(pState->GetServer());
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
		CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
		if (controls && controls->pLocalListViewPanel)
		{
			wxStatusBar* pStatusBar = controls->pLocalListViewPanel->GetStatusBar();
			if (pStatusBar)
			{
				pStatusBar->Show(show);
				wxSizeEvent evt;
				controls->pLocalListViewPanel->ProcessEvent(evt);
			}
		}
		if (controls && controls->pRemoteListViewPanel)
		{
			wxStatusBar* pStatusBar = controls->pRemoteListViewPanel->GetStatusBar();
			if (pStatusBar)
			{
				pStatusBar->Show(show);
				wxSizeEvent evt;
				controls->pRemoteListViewPanel->ProcessEvent(evt);
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
		CState* pState = CContextManager::Get()->GetCurrentContext();
		if (!pState || !m_pQueueView)
		{
			wxBell();
			return;
		}
		CManualTransfer dlg(m_pQueueView);
		dlg.Show(this, pState);
	}
	else if (event.GetId() == XRCID("ID_BOOKMARK_ADD") || event.GetId() == XRCID("ID_BOOKMARK_MANAGE"))
	{
		CServer server;
		CState* pState = CContextManager::Get()->GetCurrentContext();
		const CServer* pServer = pState ? pState->GetServer() : 0;

		CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
		if (!controls)
			return;
		if (!pServer && !controls->site_bookmarks->path.empty())
		{
			// Get server from site manager
			CSiteManagerItemData_Site* data = CSiteManager::GetSiteByPath(controls->site_bookmarks->path);
			if (data)
			{
				server = data->m_server;
				pServer = &server;
				delete data;
			}
			else
			{
				controls->site_bookmarks->path.clear();
				controls->site_bookmarks->bookmarks.clear();
				UpdateBookmarkMenu();
			}
		}

		// controls->last_bookmark_path can get modified if it's empty now
		int res;
		if (event.GetId() == XRCID("ID_BOOKMARK_ADD"))
		{
			CNewBookmarkDialog dlg(this, controls->site_bookmarks->path, pServer);
			res = dlg.ShowModal(pState->GetLocalDir().GetPath(), pState->GetRemotePath());
		}
		else
		{
			CBookmarksDialog dlg(this, controls->site_bookmarks->path, pServer);

			res = dlg.ShowModal(pState->GetLocalDir().GetPath(), pState->GetRemotePath());
		}
		if (res == wxID_OK)
		{
			controls->site_bookmarks->bookmarks.clear();
			CSiteManager::GetBookmarks(controls->site_bookmarks->path, controls->site_bookmarks->bookmarks);
			UpdateBookmarkMenu();
		}	
	}
	else if (event.GetId() == XRCID("ID_MENU_HELP_WELCOME"))
	{
		CWelcomeDialog dlg;
		dlg.Run(this, true);
	}
	else
	{
		for (int i = 0; i < 10; i++)
		{
			if (event.GetId() != tab_hotkey_ids[i])
				continue;
			
			if (!m_pContextControl)
				return;

			int sel = i - 1;
			if (sel < 0)
				sel = 9;
			m_pContextControl->SelectTab(sel);

			return;
		}
		CState* pState = CContextManager::Get()->GetCurrentContext();

		std::map<int, wxString>::const_iterator iter = m_bookmark_menu_id_map_site.find(event.GetId());
		if (iter != m_bookmark_menu_id_map_site.end())
		{
			// We hit a site-specific bookmark
			CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
			if (!controls)
				return;
			if (controls->site_bookmarks->path.empty())
				return;

			wxString name = iter->second;
			name.Replace(_T("\\"), _T("\\\\"));
			name.Replace(_T("/"), _T("\\/"));
			name = controls->site_bookmarks->path + _T("/") + name;

			CSiteManagerItemData_Site *pData = CSiteManager::GetSiteByPath(name);
			if (!pData)
				return;

			if (!pState)
				return;

			pState->SetSyncBrowse(false);
			if (!pData->m_remoteDir.IsEmpty() && pState->IsRemoteIdle())
			{
				const CServer* pServer = pState->GetServer();
				if (!pServer || *pServer != pData->m_server)
				{
					ConnectToSite(pData);
					pData->m_localDir.clear(); // So not to set again below
				}
				else
					pState->ChangeRemoteDir(pData->m_remoteDir);
			}
			if (!pData->m_localDir.empty())
			{
				bool set = pState->SetLocalDir(pData->m_localDir);

				if (set && pData->m_sync)
				{
					wxASSERT(!pData->m_remoteDir.IsEmpty());
					pState->SetSyncBrowse(true, pData->m_remoteDir);
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

			pState->SetSyncBrowse(false);
			if (!remote_dir.IsEmpty() && pState->IsRemoteIdle())
			{
				const CServer* pServer = pState->GetServer();
				if (pServer)
				{
					CServerPath current_remote_path = pState->GetRemotePath();
					if (!current_remote_path.IsEmpty() && current_remote_path.GetType() != remote_dir.GetType())
					{
						wxMessageBox(_("Selected global bookmark and current server use a different server type.\nUse site-specific bookmarks for this server."), _("Bookmark"), wxICON_EXCLAMATION, this);
						return;
					}
					pState->ChangeRemoteDir(remote_dir);
				}
			}
			if (!local_dir.empty())
			{
				bool set = pState->SetLocalDir(local_dir);

				if (set && sync)
				{
					wxASSERT(!remote_dir.IsEmpty());
					pState->SetSyncBrowse(true, remote_dir);
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

		if (!ConnectToSite(pData))
		{
			delete pData;
			return;
		}

		SetBookmarksFromPath(path);
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
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (pState)
		pServer = pState->GetServer();

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
	CFileZillaEngine* pEngine = (CFileZillaEngine*)event.GetEventObject();
	if (!pEngine)
		return;

	const std::vector<CState*> *pStates = CContextManager::Get()->GetAllStates();
	CState* pState = 0;
	for (std::vector<CState*>::const_iterator iter = pStates->begin(); iter != pStates->end(); iter++)
	{
		if ((*iter)->m_pEngine != pEngine)
			continue;
		
		pState = *iter;
		break;
	}
	if (!pState)
		return;

	CNotification *pNotification = pState->m_pEngine->GetNextNotification();
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
			pState->m_pCommandQueue->Finish(reinterpret_cast<COperationNotification*>(pNotification));
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
					pState->SetRemoteDir(0, false);
				else
				{
					CDirectoryListing* pListing = new CDirectoryListing;
					if (pListingNotification->Failed() ||
						pState->m_pEngine->CacheLookup(pListingNotification->GetPath(), *pListing) != FZ_REPLY_OK)
					{
						delete pListing;
						pListing = new CDirectoryListing;
						pListing->path = pListingNotification->GetPath();
						pListing->m_failed = true;
						pListing->m_firstListTime = CTimeEx::Now();
					}

					pState->SetRemoteDir(pListing, pListingNotification->Modified());
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
					m_pAsyncRequestQueue->AddRequest(pState->m_pEngine, pAsyncRequest);
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
			if (pState)
			{
				CLocalDirCreatedNotification *pLocalDirCreatedNotification = reinterpret_cast<CLocalDirCreatedNotification *>(pNotification);
				pState->LocalDirCreated(pLocalDirCreatedNotification->dir);
			}
			delete pNotification;
			break;
		default:
			delete pNotification;
			break;
		}

		pNotification = pState->m_pEngine->GetNextNotification();
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
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState || !pState->IsRemoteConnected())
		return;

	if (!pState->IsRemoteIdle())
		return;

	pState->Disconnect();
}

void CMainFrame::OnCancel(wxCommandEvent& event)
{
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState || pState->m_pCommandQueue->Idle())
		return;

	if (wxMessageBox(_("Really cancel current operation?"), _T("FileZilla"), wxYES_NO | wxICON_QUESTION) == wxYES)
	{
		pState->m_pCommandQueue->Cancel();
		pState->GetRecursiveOperationHandler()->StopRecursiveOperation();
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
		m_pActivityLed[i] = 0;
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
	const std::vector<CState*> *pStates = CContextManager::Get()->GetAllStates();
	for (std::vector<CState*>::const_iterator iter = pStates->begin(); iter != pStates->end(); iter++)
	{
		CState* pState = *iter;
		if (!pState->m_pCommandQueue)
			continue;

		if (!pState->m_pCommandQueue->Cancel())
			res = false;
	}

	if (!res)
	{
		event.Veto();
		return;
	}

	CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
	if (controls)
	{
		COptions::Get()->SetLastServer(controls->pState->GetLastServer());
		COptions::Get()->SetOption(OPTION_LASTSERVERPATH, controls->pState->GetLastServerPath().GetSafePath());
		COptions::Get()->SetOption(OPTION_LAST_CONNECTED_SITE, controls->site_bookmarks->path);
	}

	for (std::vector<CState*>::const_iterator iter = pStates->begin(); iter != pStates->end(); iter++)
	{
		CState *pState = *iter;
		pState->DestroyEngine();
	}

	CSiteManager::ClearIdMap();

	if (controls)
	{
		controls->pLocalListView->SaveColumnSettings(OPTION_LOCALFILELIST_COLUMN_WIDTHS, OPTION_LOCALFILELIST_COLUMN_SHOWN, OPTION_LOCALFILELIST_COLUMN_ORDER);
		controls->pRemoteListView->SaveColumnSettings(OPTION_REMOTEFILELIST_COLUMN_WIDTHS, OPTION_REMOTEFILELIST_COLUMN_SHOWN, OPTION_REMOTEFILELIST_COLUMN_ORDER);
	}

	bool filters_toggled = CFilterManager::HasActiveFilters(true) && !CFilterManager::HasActiveFilters(false);
	COptions::Get()->SetOption(OPTION_FILTERTOGGLESTATE, filters_toggled ? 1 : 0);

	Destroy();
}

void CMainFrame::OnReconnect(wxCommandEvent &event)
{
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	if (pState->IsRemoteConnected() || !pState->IsRemoteIdle())
		return;

	CServer server = pState->GetLastServer();

	if (server.GetLogonType() == ASK)
	{
		if (!CLoginManager::Get().GetPassword(server, false))
			return;
	}

	CServerPath path = pState->GetLastServerPath();
	Connect(server, path);
}

void CMainFrame::OnRefresh(wxCommandEvent &event)
{
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	pState->RefreshRemote();
	pState->RefreshLocal();
}

void CMainFrame::OnTimer(wxTimerEvent& event)
{
	/*
	if (m_transferStatusTimer.IsRunning() && event.GetId() == m_transferStatusTimer.GetId())
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
	else */
	if (event.GetId() == m_closeEventTimer.GetId())
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
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	CSiteManager dlg;

	std::set<wxString> handled_paths;
	std::vector<CSiteManager::_connected_site> connected_sites;

	if (pServer)
	{
		CSiteManager::_connected_site connected_site;
		connected_site.server = *pServer;
		connected_sites.push_back(connected_site);
	}

	for (int i = 0; i < m_pContextControl->GetTabCount(); i++)
	{
		CContextControl::_context_controls *controls =  m_pContextControl->GetControlsFromTabIndex(i);
		if (!controls)
			continue;

		const wxString& path = controls->site_bookmarks->path;
		if (path == _T(""))
			continue;
		if (handled_paths.find(path) != handled_paths.end())
			continue;

		CSiteManager::_connected_site connected_site;
		connected_site.old_path = path;
		connected_site.server = controls->pState->GetLastServer();
		connected_sites.push_back(connected_site);
		handled_paths.insert(path);
	}

	if (!dlg.Create(this, &connected_sites, pServer))
		return;

	int res = dlg.ShowModal();
	if (res == wxID_YES || res == wxID_OK)
	{
		// Update bookmark paths
		for (size_t j = 0; j < connected_sites.size(); j++)
		{
			for (int i = 0; i < m_pContextControl->GetTabCount(); i++)
			{
				CContextControl::_context_controls *controls =  m_pContextControl->GetControlsFromTabIndex(i);
				if (!controls)
					continue;

				if (connected_sites[j].old_path != controls->site_bookmarks->path)
					continue;

				controls->site_bookmarks->path = connected_sites[j].new_path;

				controls->site_bookmarks->bookmarks.clear();
				dlg.GetBookmarks(controls->site_bookmarks->path, controls->site_bookmarks->bookmarks);

				break;
			}
		}
	}

	if (res == wxID_YES)
	{
		CSiteManagerItemData_Site data;
		if (!dlg.GetServer(data))
			return;

		if (ConnectToSite(&data))
		{
			// Get new bookmarks
			wxString path = dlg.GetSitePath();
			SetBookmarksFromPath(path);
		}
	}

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

	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (m_pStatusBar && pState)
	{
		m_pStatusBar->DisplayDataType(pState->GetServer());
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
			m_pBottomSplitter->SplitHorizontally(m_pContextControl, m_pQueueLogSplitter);
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
	if (!m_pContextControl)
		return;

	CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
	bool show = !controls || !controls->pLocalSplitter->IsSplit();

	if (!show)
	{
		for (int i = 0; i < m_pContextControl->GetTabCount(); i++)
		{
			CContextControl::_context_controls* controls = m_pContextControl->GetControlsFromTabIndex(i);
			if (!controls)
				continue;

			if (!controls->pLocalSplitter->IsSplit())
				continue;

			controls->pLocalListViewPanel->SetHeader(controls->pLocalTreeViewPanel->DetachHeader());
			controls->pLocalSplitter->Unsplit(controls->pLocalTreeViewPanel);
		}
	}
	else
		ShowLocalTree();

	COptions::Get()->SetOption(OPTION_SHOW_TREE_LOCAL, show);

	if (m_pMenuBar)
		m_pMenuBar->Check(XRCID("ID_VIEW_LOCALTREE"), show);
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_LOCALTREEVIEW"), show);
}

void CMainFrame::ShowLocalTree()
{
	if (!m_pContextControl)
		return;

	const int layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);
	const int swap = COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP);
	for (int i = 0; i < m_pContextControl->GetTabCount(); i++)
	{
		CContextControl::_context_controls* controls = m_pContextControl->GetControlsFromTabIndex(i);
		if (!controls)
			continue;

		if (controls->pLocalSplitter->IsSplit())
			continue;

		controls->pLocalTreeViewPanel->SetHeader(controls->pLocalListViewPanel->DetachHeader());
		wxSize size = controls->pLocalSplitter->GetClientSize();
		if (layout == 3 && swap)
			controls->pLocalSplitter->SplitVertically(controls->pLocalListViewPanel, controls->pLocalTreeViewPanel);
		else if (layout)
			controls->pLocalSplitter->SplitVertically(controls->pLocalTreeViewPanel, controls->pLocalListViewPanel);
		else
			controls->pLocalSplitter->SplitHorizontally(controls->pLocalTreeViewPanel, controls->pLocalListViewPanel);
	}
}

void CMainFrame::OnToggleRemoteTreeView(wxCommandEvent& event)
{
	if (!m_pContextControl)
		return;

	CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
	bool show = !controls || !controls->pRemoteSplitter->IsSplit();

	if (!show)
	{
		for (int i = 0; i < m_pContextControl->GetTabCount(); i++)
		{
			CContextControl::_context_controls* controls = m_pContextControl->GetControlsFromTabIndex(i);
			if (!controls)
				continue;

			if (!controls->pRemoteSplitter->IsSplit())
				continue;

			controls->pRemoteListViewPanel->SetHeader(controls->pRemoteTreeViewPanel->DetachHeader());
			controls->pRemoteSplitter->Unsplit(controls->pRemoteTreeViewPanel);
		}
	}
	else
		ShowRemoteTree();

	COptions::Get()->SetOption(OPTION_SHOW_TREE_REMOTE, show);

	if (m_pMenuBar)
		m_pMenuBar->Check(XRCID("ID_VIEW_REMOTETREE"), show);
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_REMOTETREEVIEW"), show);
}

void CMainFrame::ShowRemoteTree()
{
	if (!m_pContextControl)
		return;

	const int layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);
	const int swap = COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP);
	for (int i = 0; i < m_pContextControl->GetTabCount(); i++)
	{
		CContextControl::_context_controls* controls = m_pContextControl->GetControlsFromTabIndex(i);
		if (!controls)
			continue;

		if (controls->pRemoteSplitter->IsSplit())
			continue;

		controls->pRemoteTreeViewPanel->SetHeader(controls->pRemoteListViewPanel->DetachHeader());
		wxSize size = controls->pRemoteSplitter->GetClientSize();
		if (layout == 3 && !swap)
			controls->pRemoteSplitter->SplitVertically(controls->pRemoteListViewPanel, controls->pRemoteTreeViewPanel);
		else if (layout)
			controls->pRemoteSplitter->SplitVertically(controls->pRemoteTreeViewPanel, controls->pRemoteListViewPanel);
		else
			controls->pRemoteSplitter->SplitHorizontally(controls->pRemoteTreeViewPanel, controls->pRemoteListViewPanel);
	}
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
			m_pBottomSplitter->SplitHorizontally(m_pContextControl, m_pQueueLogSplitter);
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
			m_pBottomSplitter->SplitHorizontally(m_pContextControl, m_pQueueLogSplitter);
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
	if (wxGetKeyState(WXK_SHIFT))
	{
		OnFilterRightclicked(event);
		return;
	}

	CFilterDialog dlg;
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), dlg.HasActiveFilters());
	dlg.Create(this);
	dlg.ShowModal();
	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), dlg.HasActiveFilters());
	CContextManager::Get()->NotifyAllHandlers(STATECHANGE_APPLYFILTER);
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
						m_pBottomSplitter->SplitHorizontally(m_pContextControl, m_pQueueLogSplitter);
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
	for (int i = 0; i < m_pContextControl->GetTabCount(); i++)
	{
		struct CContextControl::_context_controls *controls = m_pContextControl->GetControlsFromTabIndex(i);
		if (!controls)
			continue;

		int mode;
		if (!layout || layout == 2 || layout == 3)
			mode = wxSPLIT_VERTICAL;
		else
			mode = wxSPLIT_HORIZONTAL;

		int isMode = controls->pViewSplitter->GetSplitMode();

		int isSwap = controls->pViewSplitter->GetWindow1() == controls->pRemoteSplitter ? 1 : 0;

		if (mode != isMode || swap != isSwap)
		{
			controls->pViewSplitter->Unsplit();
			if (mode == wxSPLIT_VERTICAL)
			{
				if (swap)
					controls->pViewSplitter->SplitVertically(controls->pRemoteSplitter, controls->pLocalSplitter);
				else
					controls->pViewSplitter->SplitVertically(controls->pLocalSplitter, controls->pRemoteSplitter);
			}
			else
			{
				if (swap)
					controls->pViewSplitter->SplitHorizontally(controls->pRemoteSplitter, controls->pLocalSplitter);
				else
					controls->pViewSplitter->SplitHorizontally(controls->pLocalSplitter, controls->pRemoteSplitter);
			}
		}

		if (controls->pLocalSplitter->IsSplit())
		{
			if (!layout)
				mode = wxSPLIT_HORIZONTAL;
			else
				mode = wxSPLIT_VERTICAL;

			wxWindow* pFirst;
			wxWindow* pSecond;
			if (layout == 3 && swap)
			{
				pFirst = controls->pLocalListViewPanel;
				pSecond = controls->pLocalTreeViewPanel;
			}
			else
			{
				pFirst = controls->pLocalTreeViewPanel;
				pSecond = controls->pLocalListViewPanel;
			}

			if (mode != controls->pLocalSplitter->GetSplitMode() || pFirst != controls->pLocalSplitter->GetWindow1())
			{
				controls->pLocalSplitter->Unsplit();
				if (mode == wxSPLIT_VERTICAL)
					controls->pLocalSplitter->SplitVertically(pFirst, pSecond);
				else
					controls->pLocalSplitter->SplitHorizontally(pFirst, pSecond);
			}
		}

		if (controls->pRemoteSplitter->IsSplit())
		{
			if (!layout)
				mode = wxSPLIT_HORIZONTAL;
			else
				mode = wxSPLIT_VERTICAL;

			wxWindow* pFirst;
			wxWindow* pSecond;
			if (layout == 3 && !swap)
			{
				pFirst = controls->pRemoteListViewPanel;
				pSecond = controls->pRemoteTreeViewPanel;
			}
			else
			{
				pFirst = controls->pRemoteTreeViewPanel;
				pSecond = controls->pRemoteListViewPanel;
			}

			if (mode != controls->pRemoteSplitter->GetSplitMode() || pFirst != controls->pRemoteSplitter->GetWindow1())
			{
				controls->pRemoteSplitter->Unsplit();
				if (mode == wxSPLIT_VERTICAL)
					controls->pRemoteSplitter->SplitVertically(pFirst, pSecond);
				else
					controls->pRemoteSplitter->SplitHorizontally(pFirst, pSecond);
			}
		}

		if (layout == 3)
		{
			if (!swap)
			{
				controls->pRemoteSplitter->SetSashGravity(1.0);
				controls->pLocalSplitter->SetSashGravity(0.0);
			}
			else
			{
				controls->pLocalSplitter->SetSashGravity(1.0);
				controls->pRemoteSplitter->SetSashGravity(0.0);
			}
		}
		else
		{
			controls->pLocalSplitter->SetSashGravity(0.0);
			controls->pRemoteSplitter->SetSashGravity(0.0);
		}
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

	if (!Connect(pData->m_server, pData->m_remoteDir))
		return false;

	if (pData->m_localDir != _T(""))
	{
		CState *pState = CContextManager::Get()->GetCurrentContext();
		bool set = pState && pState->SetLocalDir(pData->m_localDir);

		if (set && pData->m_sync)
		{
			wxASSERT(!pData->m_remoteDir.IsEmpty());

			pState->SetSyncBrowse(true, pData->m_remoteDir);
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

	CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
	if (controls)
	{
		controls->pLocalListView->InitDateFormat();
		controls->pRemoteListView->InitDateFormat();
	}

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

	CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
	if (controls)
	{
		if (COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP) == 0)
		{
			windowOrder.push_back(controls->pLocalViewHeader);
			windowOrder.push_back(controls->pLocalTreeView);
			windowOrder.push_back(controls->pLocalListView);
			windowOrder.push_back(controls->pRemoteViewHeader);
			windowOrder.push_back(controls->pRemoteTreeView);
			windowOrder.push_back(controls->pRemoteListView);
		}
		else
		{
			windowOrder.push_back(controls->pRemoteViewHeader);
			windowOrder.push_back(controls->pRemoteTreeView);
			windowOrder.push_back(controls->pRemoteListView);
			windowOrder.push_back(controls->pLocalViewHeader);
			windowOrder.push_back(controls->pLocalTreeView);
			windowOrder.push_back(controls->pLocalListView);
		}
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
	CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
	if (controls)
	{
		windowOrder.push_back(controls->pLocalViewHeader);
		windowOrder.push_back(controls->pRemoteViewHeader);
	}

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
	CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
	if (!controls)
		return;
	
	wxString posString;

	// top_pos
	posString += wxString::Format(_T("%d "), m_pTopSplitter->GetSashPosition());

	// bottom_height
	posString += wxString::Format(_T("%d "), m_pBottomSplitter->GetSashPosition());

	// view_pos
	posString += wxString::Format(_T("%d "), (int)(controls->pViewSplitter->GetRelativeSashPosition() * 1000000000));

	// local_pos
	posString += wxString::Format(_T("%d "), controls->pLocalSplitter->GetSashPosition());

	// remote_pos
	posString += wxString::Format(_T("%d "), controls->pRemoteSplitter->GetSashPosition());

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

	CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
	if (!controls)
		return false;

	double pos = (double)aPosValues[2] / 1000000000;
	if (pos >= 0 && pos <= 1)
		controls->pViewSplitter->SetRelativeSashPosition(pos);

	controls->pLocalSplitter->SetSashPosition(aPosValues[3]);
	controls->pRemoteSplitter->SetSashPosition(aPosValues[4]);

	pos = (double)aPosValues[5] / 1000000000;
	if (pos >= 0 && pos <= 1)
		m_pQueueLogSplitter->SetRelativeSashPosition(pos);
	delete [] aPosValues;

	return true;
}

void CMainFrame::SetDefaultSplitterPositions()
{
	m_pTopSplitter->SetSashPosition(97);

	wxSize size = m_pBottomSplitter->GetClientSize();
	int h = size.GetHeight() - 135;
	if (h < 50)
		h = 50;
	m_pBottomSplitter->SetSashPosition(h);

	m_pQueueLogSplitter->SetSashPosition(0);

	CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
	if (controls)
	{
		controls->pViewSplitter->SetSashPosition(0);
		controls->pLocalSplitter->SetRelativeSashPosition(0.4);
		controls->pRemoteSplitter->SetRelativeSashPosition(0.4);
	}
}

void CMainFrame::OnActivate(wxActivateEvent& event)
{
	// According to the wx docs we should do this
	event.Skip();

	if (!event.GetActive())
		return;

	CEditHandler* pEditHandler = CEditHandler::Get();
	if (pEditHandler)
		pEditHandler->CheckForModifications(true);

	if (m_pAsyncRequestQueue)
		m_pAsyncRequestQueue->TriggerProcessing();
}

void CMainFrame::OnToolbarComparison(wxCommandEvent& event)
{
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	CComparisonManager* pComparisonManager = pState->GetComparisonManager();
	if (pComparisonManager->IsComparing())
	{
		pComparisonManager->ExitComparisonMode();
		return;
	}

	if (!COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT))
	{
		CContextControl::_context_controls* controls = m_pContextControl->GetCurrentControls();
		if (!controls)
			return;

		if ((controls->pLocalSplitter->IsSplit() && !controls->pRemoteSplitter->IsSplit()) ||
			(!controls->pLocalSplitter->IsSplit() && controls->pRemoteSplitter->IsSplit()))
		{
			CConditionalDialog dlg(this, CConditionalDialog::compare_treeviewmismatch, CConditionalDialog::yesno);
			dlg.SetTitle(_("Directory comparison"));
			dlg.AddText(_("To compare directories, both file lists have to be aligned."));
			dlg.AddText(_("To do this, the directory trees need to be both shown or both hidden."));
			dlg.AddText(_("Show both directory trees and continue comparing?"));
			if (!dlg.Run())
			{
				// Needed to restore non-toggle state of button
				pState->NotifyHandlers(STATECHANGE_COMPARISON);
				return;
			}

			ShowLocalTree();
			ShowRemoteTree();
		}

		int pos = (controls->pLocalSplitter->GetSashPosition() + controls->pRemoteSplitter->GetSashPosition()) / 2;
		controls->pLocalSplitter->SetSashPosition(pos);
		controls->pRemoteSplitter->SetSashPosition(pos);
	}

	pComparisonManager->CompareListings();
}

void CMainFrame::OnToolbarComparisonDropdown(wxCommandEvent& event)
{
	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_TOOLBAR_COMPARISON_DROPDOWN"));
	if (!pMenu)
		return;

	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	CComparisonManager* pComparisonManager = pState->GetComparisonManager();
	pMenu->FindItem(XRCID("ID_TOOLBAR_COMPARISON"))->Check(pComparisonManager->IsComparing());

	const int mode = COptions::Get()->GetOptionVal(OPTION_COMPARISONMODE);
	if (mode == 0)
		pMenu->FindItem(XRCID("ID_COMPARE_SIZE"))->Check();
	else
		pMenu->FindItem(XRCID("ID_COMPARE_DATE"))->Check();

	pMenu->Check(XRCID("ID_COMPARE_HIDEIDENTICAL"), COptions::Get()->GetOptionVal(OPTION_COMPARE_HIDEIDENTICAL) != 0);

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
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

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

	CComparisonManager* pComparisonManager = pState->GetComparisonManager();
	if (old_mode != new_mode && pComparisonManager && pComparisonManager->IsComparing())
		pComparisonManager->CompareListings();
}

void CMainFrame::OnDropdownComparisonHide(wxCommandEvent& event)
{
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	bool old_mode = COptions::Get()->GetOptionVal(OPTION_COMPARE_HIDEIDENTICAL) != 0;
	bool new_mode = event.IsChecked();

	COptions::Get()->SetOption(OPTION_COMPARE_HIDEIDENTICAL, new_mode ? 1 : 0);

	if (m_pMenuBar)
	{
		m_pMenuBar->Check(XRCID("ID_COMPARE_HIDEIDENTICAL"), new_mode);
	}

	CComparisonManager* pComparisonManager = pState->GetComparisonManager();
	if (old_mode != new_mode && pComparisonManager && pComparisonManager->IsComparing())
		pComparisonManager->CompareListings();
}

void CMainFrame::InitToolbarState()
{
	if (!m_pToolBar)
		return;
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_LOGVIEW"), m_pStatusView && m_pStatusView->IsShown());
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_QUEUEVIEW"), m_pQueuePane && m_pQueuePane->IsShown());
	CContextControl::_context_controls* controls = m_pContextControl ? m_pContextControl->GetCurrentControls() : 0;
	if (controls)
	{
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_LOCALTREEVIEW"), controls->pLocalSplitter && controls->pLocalSplitter->IsSplit());
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_REMOTETREEVIEW"), controls->pRemoteSplitter && controls->pRemoteSplitter->IsSplit());
	}
}

void CMainFrame::UpdateToolbarState()
{
	if (!m_pToolBar)
		return;

	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	const CServer* pServer = pState->GetServer();
	const bool idle = pState->IsRemoteIdle();

	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_DISCONNECT"), pServer && idle);
	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_CANCEL"), pServer && !idle);
	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_COMPARISON"), pServer != 0);
	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pServer != 0);
	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_FIND"), pServer && idle);

	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_COMPARISON"), pState->GetComparisonManager()->IsComparing());
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pState->GetSyncBrowse());

	bool canReconnect;
	if (pServer || !idle)
		canReconnect = false;
	else
	{
		CServer tmp;
		canReconnect = pState->GetLastServer().GetHost() != _T("");
	}
	m_pToolBar->EnableTool(XRCID("ID_TOOLBAR_RECONNECT"), canReconnect);
}

void CMainFrame::InitMenubarState()
{
	if (!m_pMenuBar)
		return;

	m_pMenuBar->Check(XRCID("ID_MENU_SERVER_VIEWHIDDEN"), COptions::Get()->GetOptionVal(OPTION_VIEW_HIDDEN_FILES) ? true : false);

	int mode = COptions::Get()->GetOptionVal(OPTION_COMPARISONMODE);
	if (mode != 1)
		m_pMenuBar->Check(XRCID("ID_COMPARE_SIZE"), true);
	else
		m_pMenuBar->Check(XRCID("ID_COMPARE_DATE"), true);

	m_pMenuBar->Check(XRCID("ID_COMPARE_HIDEIDENTICAL"), COptions::Get()->GetOptionVal(OPTION_COMPARE_HIDEIDENTICAL) != 0);

	m_pMenuBar->Check(XRCID("ID_VIEW_QUICKCONNECT"), m_pQuickconnectBar != 0);
	if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) != 2)
		m_pMenuBar->Check(XRCID("ID_VIEW_MESSAGELOG"), m_pStatusView && m_pStatusView->IsShown());
	CContextControl::_context_controls* controls = m_pContextControl ? m_pContextControl->GetCurrentControls() : 0;
	if (controls)
	{
		m_pMenuBar->Check(XRCID("ID_VIEW_LOCALTREE"), controls->pLocalSplitter && controls->pLocalSplitter->IsSplit());
		m_pMenuBar->Check(XRCID("ID_VIEW_REMOTETREE"), controls->pRemoteSplitter && controls->pRemoteSplitter->IsSplit());
		m_pMenuBar->Check(XRCID("ID_VIEW_QUEUE"), m_pQueuePane && m_pQueuePane->IsShown());
		m_pMenuBar->Check(XRCID("ID_MENU_VIEW_FILELISTSTATUSBAR"), COptions::Get()->GetOptionVal(OPTION_FILELIST_STATUSBAR) != 0);
	}
}

void CMainFrame::UpdateMenubarState()
{
	if (!m_pMenuBar)
		return;

	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	const CServer* const pServer = pState->GetServer();
	const bool idle = pState->IsRemoteIdle();

	m_pMenuBar->Enable(XRCID("ID_MENU_SERVER_DISCONNECT"), pServer && idle);
	m_pMenuBar->Enable(XRCID("ID_CANCEL"), pServer && !idle);
	m_pMenuBar->Enable(XRCID("ID_MENU_SERVER_CMD"), pServer && idle);
	m_pMenuBar->Enable(XRCID("ID_MENU_FILE_COPYSITEMANAGER"), pServer != 0);
	m_pMenuBar->Enable(XRCID("ID_TOOLBAR_COMPARISON"), pServer != 0);
	m_pMenuBar->Enable(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pServer != 0);
	m_pMenuBar->Enable(XRCID("ID_MENU_SERVER_SEARCH"), pServer && idle);

	m_pMenuBar->Check(XRCID("ID_TOOLBAR_COMPARISON"), pState->GetComparisonManager()->IsComparing());
	m_pMenuBar->Check(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pState->GetSyncBrowse());

	bool canReconnect;
	if (pServer || !idle)
		canReconnect = false;
	else
	{
		CServer tmp;
		canReconnect = pState->GetLastServer().GetHost() != _T("");
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
				SetBookmarksFromPath(site);
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

		wxString logontype = pCommandLine->GetOption(CCommandLine::logontype);
		if (logontype == _T("ask"))
			server.SetLogonType(ASK);
		else if (logontype == _T("interactive"))
			server.SetLogonType(INTERACTIVE);

		CServerPath path;
		if (!server.ParseUrl(param, 0, _T(""), _T(""), error, path))
		{
			wxString str = _("Parameter not a valid URL");
			str += _T("\n") + error;
			wxMessageBox(error, _("Syntax error in command line"));
		}

		if (server.GetLogonType() == ASK ||
			(server.GetLogonType() == INTERACTIVE && server.GetUser() == _T("")))
		{
			if (!CLoginManager::Get().GetPassword(server, false))
				return;
		}

		CState* pState = CContextManager::Get()->GetCurrentContext();
		if (!pState)
			return;

		Connect(server, path);
	}
}

void CMainFrame::OnFilterRightclicked(wxCommandEvent& event)
{
	const bool active = CFilterManager::HasActiveFilters();

	CFilterManager::ToggleFilters();

	if (active == CFilterManager::HasActiveFilters())
		return;

	CContextManager::Get()->NotifyAllHandlers(STATECHANGE_APPLYFILTER);
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

		if (!m_pContextControl)
			return 0;

		for (int i = 0; i < m_pContextControl->GetTabCount(); i++)
		{
			CContextControl::_context_controls* controls = m_pContextControl->GetControlsFromTabIndex(i);
			if (controls && controls->pLocalTreeView)
				controls->pLocalTreeView->OnDevicechange(wParam, lParam);
		}
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
	m_bookmark_menu_id_map_global.clear();

	for (std::map<int, wxString>::const_iterator iter = m_bookmark_menu_id_map_site.begin(); iter != m_bookmark_menu_id_map_site.end(); iter++)
	{
		pMenu->Delete(iter->first);
	}
	m_bookmark_menu_id_map_site.clear();

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
	CContextControl::_context_controls* controls = m_pContextControl ? m_pContextControl->GetCurrentControls() : 0;
	if (!controls)
		return;

	if (!controls->site_bookmarks || controls->site_bookmarks->bookmarks.empty())
		return;

	pMenu->AppendSeparator();

	for (std::list<wxString>::const_iterator iter = controls->site_bookmarks->bookmarks.begin(); iter != controls->site_bookmarks->bookmarks.end(); iter++)
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
	CContextControl::_context_controls* controls = m_pContextControl ? m_pContextControl->GetCurrentControls() : 0;
	if (!controls)
		controls->site_bookmarks = new CContextControl::_context_controls::_site_bookmarks;
	UpdateBookmarkMenu();
}

void CMainFrame::OnSyncBrowse(wxCommandEvent& event)
{
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	pState->SetSyncBrowse(!pState->GetSyncBrowse());

	if (m_pToolBar)
		m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pState->GetSyncBrowse());
}

#ifndef __WXMAC__
void CMainFrame::OnIconize(wxIconizeEvent& event)
{
#ifdef __WXGTK__
	if (m_taskbar_is_uniconizing)
		return;
#endif
	if (!event.Iconized())
	{
		if (m_taskBarIcon)
			m_taskBarIcon->RemoveIcon();
		Show(true);

		if (m_pAsyncRequestQueue)
			m_pAsyncRequestQueue->TriggerProcessing();

		return;
	}


	if (!COptions::Get()->GetOptionVal(OPTION_MINIMIZE_TRAY))
		return;

	if (!m_taskBarIcon)
	{
		m_taskBarIcon = new wxTaskBarIcon();
		m_taskBarIcon->Connect(wxEVT_TASKBAR_LEFT_DCLICK, wxTaskBarIconEventHandler(CMainFrame::OnTaskBarClick), 0, this);
		m_taskBarIcon->Connect(wxEVT_TASKBAR_LEFT_UP, wxTaskBarIconEventHandler(CMainFrame::OnTaskBarClick), 0, this);
		m_taskBarIcon->Connect(wxEVT_TASKBAR_RIGHT_UP, wxTaskBarIconEventHandler(CMainFrame::OnTaskBarClick), 0, this);
	}

	bool installed;
	if (!m_taskBarIcon->IsIconInstalled())
		installed = m_taskBarIcon->SetIcon(CThemeProvider::GetIcon(_T("ART_FILEZILLA")), GetTitle());
	else
		installed = true;

	if (installed)
		Show(false);
}

void CMainFrame::OnTaskBarClick(wxTaskBarIconEvent& event)
{
#ifdef __WXGTK__
	if (m_taskbar_is_uniconizing)
		return;
	m_taskbar_is_uniconizing = true;
#endif

	if (m_taskBarIcon)
		m_taskBarIcon->RemoveIcon();

	Show(true);
	Iconize(false);

	if (m_pAsyncRequestQueue)
		m_pAsyncRequestQueue->TriggerProcessing();

#ifdef __WXGTK__
	wxCommandEvent evt(fzEVT_TASKBAR_CLICK_DELAYED);
	AddPendingEvent(evt);
#endif
}

#ifdef __WXGTK__
void CMainFrame::OnTaskBarClick_Delayed(wxCommandEvent& event)
{
	m_taskbar_is_uniconizing = false;
}
#endif

#endif

void CMainFrame::OnSearch(wxCommandEvent& event)
{
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	CSearchDialog dlg(this, pState, m_pQueueView);
	if (!dlg.Load())
		return;

	dlg.Run();
}

void CMainFrame::PostInitialize()
{
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	// Need to do this after welcome screen to avoid simultaneous display of multiple dialogs
	if (!COptions::Get()->GetDefaultVal(DEFAULT_DISABLEUPDATECHECK) && COptions::Get()->GetOptionVal(OPTION_UPDATECHECK))
	{
		m_pUpdateWizard = new CUpdateWizard(this);
		m_pUpdateWizard->InitAutoUpdateCheck();
	}
	else
		m_pUpdateWizard = 0;
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
}

void CMainFrame::OnMenuNewTab(wxCommandEvent& event)
{
	if (m_pContextControl)
		m_pContextControl->CreateTab();
}

void CMainFrame::OnMenuCloseTab(wxCommandEvent& event)
{
	if (!m_pContextControl)
		return;

	m_pContextControl->CloseTab(m_pContextControl->GetCurrentTab());
}

void CMainFrame::SetBookmarksFromPath(const wxString& path)
{
	if (!m_pContextControl)
		return;

	CSharedPointer<CContextControl::_context_controls::_site_bookmarks> site_bookmarks;
	for (int i = 0; i < m_pContextControl->GetTabCount(); i++)
	{
		CContextControl::_context_controls *controls = m_pContextControl->GetControlsFromTabIndex(i);
		if (i == m_pContextControl->GetCurrentTab())
			continue;
		if (controls->site_bookmarks->path != path)
			continue;

		site_bookmarks = controls->site_bookmarks;
		site_bookmarks->bookmarks.clear();
	}
	if (!site_bookmarks)
	{
		site_bookmarks = new CContextControl::_context_controls::_site_bookmarks;
		site_bookmarks->path = path;
	}

	CContextControl::_context_controls *controls = m_pContextControl->GetCurrentControls();
	if (controls)
	{
		controls->site_bookmarks = site_bookmarks;
		CSiteManager::GetBookmarks(controls->site_bookmarks->path, controls->site_bookmarks->bookmarks);
	}
}

bool CMainFrame::Connect(const CServer &server, const CServerPath &path /*=CServerPath()*/)
{
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return false;

	if (pState->IsRemoteConnected() || !pState->IsRemoteIdle())
	{
		wxDialogEx dlg;
		if (dlg.Load(this, _T("ID_ALREADYCONNECTED")))
		{
			if (COptions::Get()->GetOptionVal(OPTION_ALREADYCONNECTED_CHOICE) != 0)
				XRCCTRL(dlg, "ID_OLDTAB", wxRadioButton)->SetValue(true);
			else
				XRCCTRL(dlg, "ID_NEWTAB", wxRadioButton)->SetValue(true);

			if (dlg.ShowModal() != wxID_OK)
				return false;

			if (XRCCTRL(dlg, "ID_NEWTAB", wxRadioButton)->GetValue())
			{
				m_pContextControl->CreateTab();
				pState = CContextManager::Get()->GetCurrentContext();
				COptions::Get()->SetOption(OPTION_ALREADYCONNECTED_CHOICE, 0);
			}
			else
				COptions::Get()->SetOption(OPTION_ALREADYCONNECTED_CHOICE, 1);
		}
	}

	return pState->Connect(server, path);
}
