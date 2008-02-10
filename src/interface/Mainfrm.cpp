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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TRANSFERSTATUS_TIMER_ID wxID_HIGHEST + 3

static const int statbarWidths[6] = {
#ifdef __WXMSW__
	-2, 90, -1, 150, -1, 41
#else
	-2, 90, -1, 150, -1, 50
#endif
};

#ifdef __WXMSW__
DECLARE_EVENT_TYPE(fzEVT_ONSIZE_POST, -1)
DEFINE_EVENT_TYPE(fzEVT_ONSIZE_POST)
#endif

BEGIN_EVENT_TABLE(CMainFrame, wxFrame)
	EVT_SIZE(CMainFrame::OnSize)
#ifdef __WXMSW__
	EVT_COMMAND(wxID_ANY, fzEVT_ONSIZE_POST, CMainFrame::OnSizePost)
#endif
	EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, CMainFrame::OnViewSplitterPosChanged)
	EVT_MENU(wxID_ANY, CMainFrame::OnMenuHandler)
	EVT_MENU_OPEN(CMainFrame::OnMenuOpenHandler)
	EVT_FZ_NOTIFICATION(wxID_ANY, CMainFrame::OnEngineEvent)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_DISCONNECT"), CMainFrame::OnUpdateToolbarDisconnect)
	EVT_TOOL(XRCID("ID_TOOLBAR_DISCONNECT"), CMainFrame::OnDisconnect)
	EVT_UPDATE_UI(XRCID("ID_MENU_SERVER_DISCONNECT"), CMainFrame::OnUpdateToolbarDisconnect)
	EVT_MENU(XRCID("ID_MENU_SERVER_DISCONNECT"), CMainFrame::OnDisconnect)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_CANCEL"), CMainFrame::OnUpdateToolbarCancel)
	EVT_TOOL(XRCID("ID_TOOLBAR_CANCEL"), CMainFrame::OnCancel)
	EVT_SPLITTER_SASH_POS_CHANGING(wxID_ANY, CMainFrame::OnSplitterSashPosChanging)
	EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, CMainFrame::OnSplitterSashPosChanged)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_RECONNECT"), CMainFrame::OnUpdateToolbarReconnect)
	EVT_TOOL(XRCID("ID_TOOLBAR_RECONNECT"), CMainFrame::OnReconnect)
	EVT_UPDATE_UI(XRCID("ID_MENU_SERVER_RECONNECT"), CMainFrame::OnUpdateToolbarReconnect)
	EVT_TOOL(XRCID("ID_MENU_SERVER_RECONNECT"), CMainFrame::OnReconnect)
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
	EVT_TOOL(XRCID("ID_TOOLBAR_FILTER"), CMainFrame::OnFilter)
#if FZ_MANUALUPDATECHECK
	EVT_MENU(XRCID("ID_CHECKFORUPDATES"), CMainFrame::OnCheckForUpdates)
#endif //FZ_MANUALUPDATECHECK
	EVT_TOOL_RCLICKED(XRCID("ID_TOOLBAR_SITEMANAGER"), CMainFrame::OnSitemanagerDropdown)
#ifdef EVT_TOOL_DROPDOWN
	EVT_TOOL_DROPDOWN(XRCID("ID_TOOLBAR_SITEMANAGER"), CMainFrame::OnSitemanagerDropdown)
#endif
	EVT_UPDATE_UI(XRCID("ID_MENU_SERVER_CMD"), CMainFrame::OnUpdateMenuCustomcommand)
	EVT_UPDATE_UI(XRCID("ID_MENU_SERVER_VIEWHIDDEN"), CMainFrame::OnUpdateMenuShowHidden)
	EVT_NAVIGATION_KEY(CMainFrame::OnNavigationKeyEvent)
	EVT_SET_FOCUS(CMainFrame::OnGetFocus)
	EVT_CHAR_HOOK(CMainFrame::OnChar)
	EVT_MENU(XRCID("ID_MENU_EDIT_FILTERS"), CMainFrame::OnFilter)
	EVT_ACTIVATE(CMainFrame::OnActivate)
	EVT_TOOL(XRCID("ID_TOOLBAR_COMPARISON"), CMainFrame::OnToolbarComparison)
	EVT_UPDATE_UI(XRCID("ID_TOOLBAR_COMPARISON"), CMainFrame::OnUpdateToolbarComparison)
	EVT_TOOL_RCLICKED(XRCID("ID_TOOLBAR_COMPARISON"), CMainFrame::OnToolbarComparisonDropdown)
#ifdef EVT_TOOL_DROPDOWN
	EVT_TOOL_DROPDOWN(XRCID("ID_TOOLBAR_COMPARISON"), CMainFrame::OnToolbarComparisonDropdown)
#endif
	EVT_MENU(XRCID("ID_COMPARE_SIZE"), CMainFrame::OnDropdownComparisonMode)
	EVT_MENU(XRCID("ID_COMPARE_DATE"), CMainFrame::OnDropdownComparisonMode)
END_EVENT_TABLE()

CMainFrame::CMainFrame() : wxFrame(NULL, -1, _T("FileZilla"), wxDefaultPosition, wxSize(900, 750))
{
	SetSizeHints(250, 250);

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
	m_bInitDone = false;
	m_bQuit = false;
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	m_pUpdateWizard = 0;
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

	m_lastLogViewSplitterPos = 0;
	m_lastLocalTreeSplitterPos = 0;
	m_lastRemoteTreeSplitterPos = 0;
	m_lastQueueSplitterPos = 0;

#ifdef __WXMSW__
	m_windowIsMaximized = false;
	m_pendingPostSizing = false;
#endif

	m_pThemeProvider = new CThemeProvider();
	m_pState = new CState(this);

	m_pStatusBar = new wxStatusBar(this, wxID_ANY, wxST_SIZEGRIP);
	if (m_pStatusBar)
	{
		m_pStatusBar->SetFieldsCount(6);

		m_pStatusBar->Connect(wxID_ANY, wxEVT_SIZE, (wxObjectEventFunction)(wxEventFunction)(wxSizeEventFunction)&CMainFrame::OnStatusbarSize, 0, this);
		int array[6];
		for (int i = 1; i < 5; i++)
			array[i] = wxSB_NORMAL;
		array[0] = wxSB_FLAT;
		array[5] = wxSB_FLAT;
		m_pStatusBar->SetStatusStyles(6, array);

		m_pStatusBar->SetStatusWidths(6, statbarWidths);

		m_pRecvLed = new CLed(m_pStatusBar, 1, m_pState);
		m_pSendLed = new CLed(m_pStatusBar, 0, m_pState);

		SetStatusBar(m_pStatusBar);
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

	m_pAsyncRequestQueue = new CAsyncRequestQueue(this);

	if (!m_pState->CreateEngine())
	{
		wxMessageBox(_("Failed to initialize FTP engine"));
	}

#ifdef __WXMSW__
	long style = wxSP_NOBORDER | wxSP_LIVE_UPDATE;
#else
	long style = wxSP_3DBORDER | wxSP_LIVE_UPDATE;
#endif

	wxSize clientSize = GetClientSize();

	m_pTopSplitter = new wxSplitterWindow(this, -1, wxDefaultPosition, clientSize, style);
	m_pTopSplitter->SetMinimumPaneSize(50);

	m_pBottomSplitter = new wxSplitterWindow(m_pTopSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pBottomSplitter->SetMinimumPaneSize(50);
	m_pBottomSplitter->SetSashGravity(1.0);

	m_pViewSplitter = new wxSplitterWindow(m_pBottomSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pViewSplitter->SetMinimumPaneSize(50);

	m_pLocalSplitter = new wxSplitterWindow(m_pViewSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pLocalSplitter->SetMinimumPaneSize(50);

	m_pRemoteSplitter = new wxSplitterWindow(m_pViewSplitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER  | wxSP_LIVE_UPDATE);
	m_pRemoteSplitter->SetMinimumPaneSize(50);

	m_pStatusView = new CStatusView(m_pTopSplitter, -1);
	m_pQueuePane = new CQueue(m_pBottomSplitter, this, m_pAsyncRequestQueue);
	m_pQueueView = m_pQueuePane->GetQueueView();

	m_pLocalTreeViewPanel = new CView(m_pLocalSplitter);
	m_pLocalListViewPanel = new CView(m_pLocalSplitter);
	m_pLocalTreeView = new CLocalTreeView(m_pLocalTreeViewPanel, -1, m_pState, m_pQueueView);
	m_pLocalListView = new CLocalListView(m_pLocalListViewPanel, m_pState, m_pQueueView);
	m_pLocalTreeViewPanel->SetWindow(m_pLocalTreeView);
	m_pLocalListViewPanel->SetWindow(m_pLocalListView);

	m_pRemoteTreeViewPanel = new CView(m_pRemoteSplitter);
	m_pRemoteListViewPanel = new CView(m_pRemoteSplitter);
	m_pRemoteTreeView = new CRemoteTreeView(m_pRemoteTreeViewPanel, -1, m_pState, m_pQueueView);
	m_pRemoteListView = new CRemoteListView(m_pRemoteListViewPanel, m_pState, m_pQueueView);
	m_pRemoteTreeViewPanel->SetWindow(m_pRemoteTreeView);
	m_pRemoteListViewPanel->SetWindow(m_pRemoteListView);

	if (COptions::Get()->GetOptionVal(OPTION_SHOW_MESSAGELOG))
		m_pTopSplitter->SplitHorizontally(m_pStatusView, m_pBottomSplitter, 100);
	else
	{
		m_pStatusView->Hide();
		m_pTopSplitter->Initialize(m_pBottomSplitter);
	}
	if (COptions::Get()->GetOptionVal(OPTION_SHOW_QUEUE))
		m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueuePane);
	else
	{
		m_pQueuePane->Hide();
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

	wxSize size = m_pBottomSplitter->GetClientSize();
	m_pBottomSplitter->SetSashPosition(size.GetHeight() - 170);

	Layout();

	m_pWindowStateManager = new CWindowStateManager(this);
	m_pWindowStateManager->Restore(OPTION_MAINWINDOW_POSITION);
	RestoreSplitterPositions();

	wxString localDir = COptions::Get()->GetOption(OPTION_LASTLOCALDIR);
	if (!m_pState->SetLocalDir(localDir))
		m_pState->SetLocalDir(_T("/"));

	wxAcceleratorEntry entries[1];
	entries[0].Set(wxACCEL_NORMAL, WXK_F5, XRCID("ID_TOOLBAR_REFRESH"));
	wxAcceleratorTable accel(1, entries);
	SetAcceleratorTable(accel);

#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	if (COptions::Get()->GetOptionVal(OPTION_UPDATECHECK))
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
}

CMainFrame::~CMainFrame()
{
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

void CMainFrame::OnSize(wxSizeEvent &event)
{
	if (!m_pBottomSplitter)
		return;

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

#ifdef __WXMSW__
	if (!m_pendingPostSizing)
	{
		m_pendingPostSizing = true;
		wxCommandEvent evt(fzEVT_ONSIZE_POST);
		AddPendingEvent(evt);
	}
#else
	if (m_pViewSplitter)
	{
		Layout();
		wxSize size = m_pViewSplitter->GetClientSize();

		const int layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);

		int pos;
		if (layout == 1)
		{
			pos = static_cast<int>(size.GetHeight() * m_ViewSplitterSashPos);
			if (pos < 20)
				pos = 20;
			else if (pos > size.GetHeight() - 20)
				pos = size.GetHeight() - 20;
		}
		else
		{
			pos = static_cast<int>(size.GetWidth() * m_ViewSplitterSashPos);
			if (pos < 20)
				pos = 20;
			else if (pos > size.GetWidth() - 20)
				pos = size.GetWidth() - 20;
		}
		m_pViewSplitter->SetSashPosition(pos);
	}

	ApplySplitterConstraints();
#endif
}

#ifdef __WXMSW__
void CMainFrame::OnSizePost(wxCommandEvent& event)
{
	m_pendingPostSizing = false;
	if (m_pViewSplitter)
	{
		Layout();
		wxSize size = m_pViewSplitter->GetClientSize();

		const int layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);

		int pos;
		if (layout == 1)
		{
			pos = static_cast<int>(size.GetHeight() * m_ViewSplitterSashPos);
			if (pos < 20)
				pos = 20;
			else if (pos > size.GetHeight() - 20)
				pos = size.GetHeight() - 20;
		}
		else
		{
			pos = static_cast<int>(size.GetWidth() * m_ViewSplitterSashPos);
			if (pos < 20)
				pos = 20;
			else if (pos > size.GetWidth() - 20)
				pos = size.GetWidth() - 20;
		}
		m_pViewSplitter->SetSashPosition(pos);
	}

	ApplySplitterConstraints();
}
#endif

void CMainFrame::OnViewSplitterPosChanged(wxSplitterEvent &event)
{
	if (event.GetEventObject() != m_pViewSplitter)
	{
		event.Skip();
		return;
	}
#ifdef __WXMSW__
	if (m_pendingPostSizing)
		return;
#endif

	wxSize size = m_pViewSplitter->GetClientSize();
	int pos = m_pViewSplitter->GetSashPosition();

	const int layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);
	if (layout != 1)
		m_ViewSplitterSashPos = pos / (float)size.GetWidth();
	else
		m_ViewSplitterSashPos = pos / (float)size.GetHeight();
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

#if !FZ_MANUALUPDATECHECK
	wxMenu *helpMenu;
	wxMenuItem* pUpdateItem = m_pMenuBar->FindItem(XRCID("ID_CHECKFORUPDATES"), &helpMenu);
	if (pUpdateItem)
		helpMenu->Delete(pUpdateItem);
#endif //!FZ_MANUALUPDATECHECK

	if (COptions::Get()->GetOptionVal(OPTION_DEBUG_MENU))
	{
		wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_DEBUG"));
		if (pMenu)
			m_pMenuBar->Append(pMenu, _("&Debug"));
	}

	SetMenuBar(m_pMenuBar);
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	if (m_pUpdateWizard)
		m_pUpdateWizard->DisplayUpdateAvailability(false, true);
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

	m_pMenuBar->FindItem(XRCID("ID_MENU_SERVER_VIEWHIDDEN"), 0)->Check(COptions::Get()->GetOptionVal(OPTION_VIEW_HIDDEN_FILES) ? true : false);

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
		if (!m_pState->m_pEngine || !m_pState->m_pEngine->IsConnected() || !m_pState->m_pCommandQueue->Idle())
			return;

		CInputDialog dlg;
		dlg.Create(this, _("Enter custom command"), _("Please enter raw FTP command.\nUsing raw ftp commands will clear the directory cache."));
		if (dlg.ShowModal() != wxID_OK)
			return;

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
			m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(path, _T(""), true));
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
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_TYPE_ASCII"))
	{
		COptions::Get()->SetOption(OPTION_ASCIIBINARY, 1);
		m_pMenuBar->FindItem(XRCID("ID_MENU_TRANSFER_TYPE_ASCII"))->Check();
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_TYPE_BINARY"))
	{
		COptions::Get()->SetOption(OPTION_ASCIIBINARY, 2);
		m_pMenuBar->FindItem(XRCID("ID_MENU_TRANSFER_TYPE_BINARY"))->Check();
	}
	else if (event.GetId() == XRCID("ID_MENU_TRANSFER_PRESERVETIMES"))
	{
		if (event.IsChecked())
		{
			CConditionalDialog dlg(this, CConditionalDialog::confirm_preserve_timestamps, CConditionalDialog::ok, true);
			dlg.SetTitle(_("Preserving file timestamps"));
			dlg.AddText(_("Please note that preserving timestamps on uploads only works on FTP, FTPS and FTPES servers (but not SFTP) supporting the MFMT command."));
			dlg.Run();
		}
		COptions::Get()->SetOption(OPTION_PRESERVE_TIMESTAMPS, event.IsChecked() ? 1 : 0);
	}
	else
	{
		CSiteManagerItemData* pData = CSiteManager::GetSiteById(event.GetId());

		if (!pData)
		{
			event.Skip();
			return;
		}

		ConnectToSite(pData);
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

	if (!pServer || pServer->GetProtocol() == FTP || pServer->GetProtocol() == FTPS || pServer->GetProtocol() == FTPES)
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
					m_pAsyncRequestQueue->AddRequest(m_pState->m_pEngine, pAsyncRequest);
			}
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
	m_pToolBar->SetExtraStyle(wxWS_EX_PROCESS_UI_UPDATES);

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

	CFilterDialog dlg;
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), dlg.HasActiveFilters());
	SetToolBar(m_pToolBar);

	if (m_pQuickconnectBar)
		m_pQuickconnectBar->Refresh();

	return true;
}

void CMainFrame::OnUpdateToolbarDisconnect(wxUpdateUIEvent& event)
{
	event.Enable(m_pState->IsRemoteConnected() && m_pState->IsRemoteIdle());
}

void CMainFrame::OnDisconnect(wxCommandEvent& event)
{
	if (!m_pState->IsRemoteConnected())
		return;

	if (!m_pState->IsRemoteIdle())
		return;

	m_pState->m_pCommandQueue->ProcessCommand(new CDisconnectCommand());
}

void CMainFrame::OnUpdateToolbarCancel(wxUpdateUIEvent& event)
{
	event.Enable(m_pState && m_pState->m_pCommandQueue && !m_pState->m_pCommandQueue->Idle());
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
	if (!m_bQuit)
	{
		static bool quit_confirmation_displayed = false;
		if (quit_confirmation_displayed)
		{
			event.Veto();
			return;
		}
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
		}

		CEditHandler* pEditHandler = CEditHandler::Get();
		if (pEditHandler)
		{
			if (pEditHandler->GetFileCount(CEditHandler::remote, CEditHandler::edit) || pEditHandler->GetFileCount(CEditHandler::none, CEditHandler::upload) ||
				pEditHandler->GetFileCount(CEditHandler::none, CEditHandler::upload_and_remove))
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
			}
		}
		quit_confirmation_displayed = false;

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

	delete m_pSendLed;
	delete m_pRecvLed;
	m_pSendLed = 0;
	m_pRecvLed = 0;

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

	COptions::Get()->SaveColumnWidths(m_pLocalListView, OPTION_LOCALFILELIST_COLUMN_WIDTHS);
	COptions::Get()->SaveColumnWidths(m_pRemoteListView, OPTION_REMOTEFILELIST_COLUMN_WIDTHS);

	Destroy();
}

void CMainFrame::OnUpdateToolbarReconnect(wxUpdateUIEvent &event)
{
	if (!m_pState->m_pEngine || m_pState->m_pEngine->IsConnected() || !m_pState->m_pCommandQueue->Idle())
	{
		event.Enable(false);
		return;
	}

	CServer server;
	event.Enable(COptions::Get()->GetLastServer(server));
}

void CMainFrame::OnReconnect(wxCommandEvent &event)
{
	if (!m_pState->m_pEngine || m_pState->m_pEngine->IsConnected() || !m_pState->m_pCommandQueue->Idle())
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
	if (m_pState->m_pEngine && m_pState->m_pEngine->IsConnected() && m_pState->m_pCommandQueue->Idle())
		m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(m_pState->GetRemotePath(), _T(""), true));

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
	CSiteManager dlg;
	if (!dlg.Create(this))
		return;

	int res = dlg.ShowModal();
	if (res == wxID_YES)
	{
		CSiteManagerItemData data;
		if (!dlg.GetServer(data))
			return;

		ConnectToSite(&data);
	}
}

void CMainFrame::UpdateSendLed()
{
	if (m_pSendLed)
		m_pSendLed->Ping();
}

void CMainFrame::UpdateRecvLed()
{
	if (m_pRecvLed)
		m_pRecvLed->Ping();
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
	COptions::Get()->SetOption(OPTION_SHOW_MESSAGELOG, m_pTopSplitter->IsSplit());
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

	if (m_pLocalSplitter->IsSplit() && m_pLocalSplitter->GetSashPosition() < 50)
		m_pLocalSplitter->SetSashPosition(50);

	if (m_pRemoteSplitter->IsSplit() && m_pRemoteSplitter->GetSashPosition() < 50)
		m_pRemoteSplitter->SetSashPosition(50);
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
		ShowLocalTree();

	COptions::Get()->SetOption(OPTION_SHOW_TREE_LOCAL, m_pLocalSplitter->IsSplit());
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
		m_pLocalSplitter->SplitVertically(m_pLocalListViewPanel, m_pLocalTreeViewPanel, m_lastLocalTreeSplitterPos);
	else if (layout)
		m_pLocalSplitter->SplitVertically(m_pLocalTreeViewPanel, m_pLocalListViewPanel, m_lastLocalTreeSplitterPos);
	else
		m_pLocalSplitter->SplitHorizontally(m_pLocalTreeViewPanel, m_pLocalListViewPanel, m_lastLocalTreeSplitterPos);
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
		ShowRemoteTree();

	COptions::Get()->SetOption(OPTION_SHOW_TREE_REMOTE, m_pRemoteSplitter->IsSplit());
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
		m_pRemoteSplitter->SplitVertically(m_pRemoteListViewPanel, m_pRemoteTreeViewPanel, m_lastRemoteTreeSplitterPos);
	else if (layout)
		m_pRemoteSplitter->SplitVertically(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel, m_lastRemoteTreeSplitterPos);
	else
		m_pRemoteSplitter->SplitHorizontally(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel, m_lastRemoteTreeSplitterPos);
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
		m_pBottomSplitter->Unsplit(m_pQueuePane);
	}
	else
	{
		wxRect rect = m_pBottomSplitter->GetClientSize();
		m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueuePane, rect.GetHeight() - m_lastQueueSplitterPos);
		ApplySplitterConstraints();
	}
	COptions::Get()->SetOption(OPTION_SHOW_QUEUE, m_pBottomSplitter->IsSplit());
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

void CMainFrame::OnFilter(wxCommandEvent& event)
{
	CFilterDialog dlg;
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), dlg.HasActiveFilters());
	dlg.Create(this);
	dlg.ShowModal();
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), dlg.HasActiveFilters());
	m_pState->ApplyCurrentFilter();
}

#if FZ_MANUALUPDATECHECK
void CMainFrame::OnCheckForUpdates(wxCommandEvent& event)
{
	wxString version(PACKAGE_VERSION, wxConvLocal);
	if (version[0] < '0' || version[0] > '9')
	{
		wxMessageBox(_("Executable contains no version info, cannot check for updates."), _("Updatecheck failed"), wxICON_ERROR, this);
		return;
	}

	CUpdateWizard dlg(this);
	if (!dlg.Load())
		return;

	dlg.Run();
}
#endif //FZ_MANUALUPDATECHECK

void CMainFrame::UpdateLayout(int layout /*=-1*/, int swap /*=-1*/)
{
	if (layout == -1)
		layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);
	if (swap == -1)
		swap = COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP);

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
				m_pLocalSplitter->SplitVertically(pFirst, pSecond, m_lastLocalTreeSplitterPos);
			else
				m_pLocalSplitter->SplitHorizontally(pFirst, pSecond, m_lastLocalTreeSplitterPos);
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
				m_pRemoteSplitter->SplitVertically(pFirst, pSecond, m_lastRemoteTreeSplitterPos);
			else
				m_pRemoteSplitter->SplitHorizontally(pFirst, pSecond, m_lastRemoteTreeSplitterPos);
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

void CMainFrame::ConnectToSite(CSiteManagerItemData* const pData)
{
	wxASSERT(pData);

	if (pData->m_server.GetLogonType() == ASK)
	{
		if (!CLoginManager::Get().GetPassword(pData->m_server, false, pData->m_name))
			return;
	}

	m_pState->Connect(pData->m_server, true, pData->m_remoteDir);

	if (pData->m_localDir != _T(""))
		m_pState->SetLocalDir(pData->m_localDir);
}

void CMainFrame::OnUpdateMenuCustomcommand(wxUpdateUIEvent& event)
{
	if (!m_pMenuBar)
		return;

	event.Enable(m_pState->m_pEngine && m_pState->m_pEngine->IsConnected() && m_pState->m_pCommandQueue->Idle());
}

void CMainFrame::OnUpdateMenuShowHidden(wxUpdateUIEvent& event)
{
	bool enable = true;

	const CServer* pServer;
	if (m_pState && (pServer = m_pState->GetServer()))
	{
		switch (pServer->GetProtocol())
		{
		case FTP:
		case FTPS:
		case FTPES:
			break;
		default:
			enable = false;
		}
	}
	event.Enable(enable);

	event.Check(COptions::Get()->GetOptionVal(OPTION_VIEW_HIDDEN_FILES) != 0);
}

void CMainFrame::CheckChangedSettings()
{
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	if (COptions::Get()->GetOptionVal(OPTION_UPDATECHECK))
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
	windowOrder.push_back(m_pQuickconnectBar);
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
	posString += wxString::Format(_T("%d "), m_pTopSplitter->IsSplit() ? m_pTopSplitter->GetSashPosition() : m_lastLogViewSplitterPos);

	// bottom_height
	int x, y;
	m_pBottomSplitter->GetClientSize(&x, &y);
	if (m_pBottomSplitter->IsSplit())
		y -= m_pBottomSplitter->GetSashPosition();
	else
		y = m_lastQueueSplitterPos;
	posString += wxString::Format(_T("%d "), y);

	// view_pos
	posString += wxString::Format(_T("%d "), m_pViewSplitter->GetSashPosition());

	// view_height_width
	m_pViewSplitter->GetClientSize(&x, &y);
	if (COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT) != 1)
		posString += wxString::Format(_T("%d "), x);
	else
		posString += wxString::Format(_T("%d "), y);

	// local_pos
	posString += wxString::Format(_T("%d "), m_pLocalSplitter->IsSplit() ? m_pLocalSplitter->GetSashPosition() : m_lastLocalTreeSplitterPos);

	// remote_pos
	posString += wxString::Format(_T("%d"), m_pRemoteSplitter->IsSplit() ? m_pRemoteSplitter->GetSashPosition() : m_lastRemoteTreeSplitterPos);

	COptions::Get()->SetOption(OPTION_MAINWINDOW_SPLITTER_POSITION, posString);
}

void CMainFrame::RestoreSplitterPositions()
{
	if (wxGetKeyState(WXK_SHIFT) && wxGetKeyState(WXK_ALT) && wxGetKeyState(WXK_CONTROL))
		return;

	// top_pos bottom_height view_pos view_height_width local_pos remote_pos
	wxString posString = COptions::Get()->GetOption(OPTION_MAINWINDOW_SPLITTER_POSITION);
	wxStringTokenizer tokens(posString, _T(" "));
	int count = tokens.CountTokens();
	if (count != 6)
		return;


	long * aPosValues = new long[count];
	for (int i = 0; i < count; i++)
	{
		wxString token = tokens.GetNextToken();
		if (!token.ToLong(aPosValues + i))
		{
			delete [] aPosValues;
			return;
		}
	}

	m_lastLogViewSplitterPos = aPosValues[0];
	m_pTopSplitter->SetSashPosition(m_lastLogViewSplitterPos);

#ifdef __WXMSW__
	if (IsMaximized())
		Show();
#endif

	m_lastQueueSplitterPos = aPosValues[1];

	int x, y;
	m_pBottomSplitter->GetClientSize(&x, &y);

	if (m_lastQueueSplitterPos > y - 100)
		m_lastQueueSplitterPos = y - 100;
	else if (m_lastQueueSplitterPos < 50)
		m_lastQueueSplitterPos = 100;

	y -= m_lastQueueSplitterPos;

	m_pBottomSplitter->SetSashPosition(y);

	m_ViewSplitterSashPos = (float)aPosValues[2] / aPosValues[3];
	m_pViewSplitter->SetSashPosition(aPosValues[2]);

	m_lastLocalTreeSplitterPos = aPosValues[4];
	m_lastRemoteTreeSplitterPos = aPosValues[5];
	m_pLocalSplitter->SetSashPosition(m_lastLocalTreeSplitterPos);
	m_pRemoteSplitter->SetSashPosition(m_lastRemoteTreeSplitterPos);
	delete [] aPosValues;
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
				return;

			ShowLocalTree();
			ShowRemoteTree();
		}
		int pos = (m_pLocalSplitter->GetSashPosition() + m_pRemoteSplitter->GetSashPosition()) / 2;
		m_pLocalSplitter->SetSashPosition(pos);
		m_pRemoteSplitter->SetSashPosition(pos);
	}

	if (!m_pComparisonManager)
		m_pComparisonManager = new CComparisonManager(m_pLocalListView, m_pRemoteListView);

	m_pComparisonManager->CompareListings();
}

void CMainFrame::OnUpdateToolbarComparison(wxUpdateUIEvent& event)
{
	bool enable;
	if (!m_pState->m_pEngine || !m_pState->m_pEngine->IsConnected())
		enable = false;
	else
		enable = true;

	event.Enable(enable);

	event.Check(m_pComparisonManager && m_pComparisonManager->IsComparing());
	if (m_pMenuBar)
	{
		m_pMenuBar->Enable(XRCID("ID_TOOLBAR_COMPARISON"), enable);
		m_pMenuBar->Check(XRCID("ID_TOOLBAR_COMPARISON"), m_pComparisonManager && m_pComparisonManager->IsComparing());

		int mode = COptions::Get()->GetOptionVal(OPTION_COMPARISONMODE);
		if (mode != 1)
			m_pMenuBar->Check(XRCID("ID_COMPARE_SIZE"), true);
		else
			m_pMenuBar->Check(XRCID("ID_COMPARE_DATE"), true);
	}
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

	if (old_mode != new_mode && m_pComparisonManager && m_pComparisonManager->IsComparing())
		m_pComparisonManager->CompareListings();
}
