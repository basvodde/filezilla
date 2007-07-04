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

#ifndef __WXMSW__
#include "resources/filezilla.xpm"
#endif

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

BEGIN_EVENT_TABLE(CMainFrame, wxFrame)
	EVT_SIZE(CMainFrame::OnSize)
	EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, CMainFrame::OnViewSplitterPosChanged)
	EVT_MENU(wxID_ANY, CMainFrame::OnMenuHandler)
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
	m_pQueuePane = new CQueue(m_pBottomSplitter, this, m_pAsyncRequestQueue);
	m_pQueueView = m_pQueuePane->GetQueueView();
	
	m_pLocalTreeViewPanel = new CView(m_pLocalSplitter);
	m_pLocalListViewPanel = new CView(m_pLocalSplitter);
	m_pLocalTreeView = new CLocalTreeView(m_pLocalTreeViewPanel, -1, m_pState, m_pQueueView);
	m_pLocalListView = new CLocalListView(m_pLocalListViewPanel, -1, m_pState, m_pQueueView);
	m_pLocalTreeViewPanel->SetWindow(m_pLocalTreeView);
	m_pLocalListViewPanel->SetWindow(m_pLocalListView);

	m_pRemoteTreeViewPanel = new CView(m_pRemoteSplitter);
	m_pRemoteListViewPanel = new CView(m_pRemoteSplitter);
	m_pRemoteTreeView = new CRemoteTreeView(m_pRemoteTreeViewPanel, -1, m_pState, m_pQueueView);
	m_pRemoteListView = new CRemoteListView(m_pRemoteListViewPanel, -1, m_pState, m_pQueueView);
	m_pRemoteTreeViewPanel->SetWindow(m_pRemoteTreeView);
	m_pRemoteListViewPanel->SetWindow(m_pRemoteListView);

	m_pTopSplitter->SplitHorizontally(m_pStatusView, m_pBottomSplitter, 100);
	m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueuePane);

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

	if (COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_LOCAL))
	{
		m_pLocalTreeViewPanel->SetHeader(new CLocalViewHeader(m_pLocalSplitter, m_pState));
		if (layout == 3 && swap)
			m_pLocalSplitter->SplitVertically(m_pLocalListViewPanel, m_pLocalTreeViewPanel);
		else if (layout)
			m_pLocalSplitter->SplitVertically(m_pLocalTreeViewPanel, m_pLocalListViewPanel);
		else
			m_pLocalSplitter->SplitHorizontally(m_pLocalTreeViewPanel, m_pLocalListViewPanel);
	}
	else
	{
		m_pLocalListViewPanel->SetHeader(new CLocalViewHeader(m_pLocalSplitter, m_pState));
		m_pLocalSplitter->Initialize(m_pLocalListViewPanel);
	}
	if (COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_REMOTE))
	{
		m_pRemoteTreeViewPanel->SetHeader(new CRemoteViewHeader(m_pRemoteSplitter, m_pState));
		if (layout == 3 && !swap)
			m_pRemoteSplitter->SplitVertically(m_pRemoteListViewPanel, m_pRemoteTreeViewPanel);
		else if (layout)
			m_pRemoteSplitter->SplitVertically(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel);
		else
			m_pRemoteSplitter->SplitHorizontally(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel);
	}
	else
	{
		m_pRemoteListViewPanel->SetHeader(new CRemoteViewHeader(m_pRemoteSplitter, m_pState));
		m_pRemoteSplitter->Initialize(m_pRemoteListViewPanel);
	}
	wxSize size = m_pBottomSplitter->GetClientSize();
	m_pBottomSplitter->SetSashPosition(size.GetHeight() - 170);

	Layout();

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

}

CMainFrame::~CMainFrame()
{
	delete m_pState;
	delete m_pAsyncRequestQueue;
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	delete m_pUpdateWizard;
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
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

void CMainFrame::OnViewSplitterPosChanged(wxSplitterEvent &event)
{
	if (event.GetEventObject() != m_pViewSplitter)
	{
		event.Skip();
		return;
	}

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

		wxTextEntryDialog dlg(this, _("Please enter raw FTP command.\nUsing raw ftp commands will clear the directory cache."), _("Enter custom command"));
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
			CConditionalDialog dlg(this, CConditionalDialog::viewhidden, CConditionalDialog::ok, true);
			dlg.SetTitle(_("View hidden files"));

			dlg.AddText(_("Note that this feature is only supported using the FTP protocol."));
			dlg.AddText(_("Also, not all servers support this feature and may return incorrect listings if 'View hidden files' is enabled. Although FileZilla performs some tests to see if the server supports this feature, the test may fail."));
			dlg.AddText(_("Disable this option if you cannot see your files anymore."));
			dlg.Run();
		}

		COptions::Get()->SetOption(OPTION_VIEW_HIDDEN_FILES, showHidden ? 1 : 0);
	}
	else if (event.GetId() == XRCID("ID_EXPORT"))
	{
		CExportDialog dlg(this, m_pQueueView);
		dlg.Show();
	}
	else if (event.GetId() == XRCID("ID_IMPORT"))
	{
		CImportDialog dlg(this);
		dlg.Show();
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
			delete pNotification;
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
						pListing->path = pListingNotification->GetPath();
						pListing->m_failed = true;
					}

					m_pState->SetRemoteDir(pListing, pListingNotification->Modified());
				}
			}
			delete pNotification;
			break;
		case nId_asyncrequest:
			m_pAsyncRequestQueue->AddRequest(m_pState->m_pEngine, reinterpret_cast<CAsyncRequestNotification *>(pNotification));
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
				const CTransferStatus *pStatus = pTransferStatusNotification ? pTransferStatusNotification->GetStatus() : 0;
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

		pNotification = m_pState->m_pEngine->GetNextNotification();
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
		return false;
	}
	m_pToolBar->SetExtraStyle(wxWS_EX_PROCESS_UI_UPDATES);

#if defined(EVT_TOOL_DROPDOWN) && defined(__WXMSW__)
	wxToolBarToolBase* pOldTool = m_pToolBar->FindById(XRCID("ID_TOOLBAR_SITEMANAGER"));
	if (pOldTool)
	{
		wxToolBarToolBase* pTool = new wxToolBarToolBase(0, XRCID("ID_TOOLBAR_SITEMANAGER"),
			pOldTool->GetLabel(), pOldTool->GetNormalBitmap(), pOldTool->GetDisabledBitmap(),
			wxITEM_DROPDOWN, NULL, pOldTool->GetShortHelp(), pOldTool->GetLongHelp());

		int pos = m_pToolBar->GetToolPos(XRCID("ID_TOOLBAR_SITEMANAGER"));
		wxASSERT(pos != wxNOT_FOUND);

		m_pToolBar->DeleteToolByPos(pos);
		m_pToolBar->InsertTool(pos, pTool);
		m_pToolBar->Realize();
	}
#endif

	CFilterDialog dlg;
	m_pToolBar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), dlg.HasActiveFilters());
	SetToolBar(m_pToolBar);

	return true;
}

void CMainFrame::OnUpdateToolbarDisconnect(wxUpdateUIEvent& event)
{
	event.Enable(m_pState->m_pEngine && m_pState->m_pEngine->IsConnected() && m_pState->m_pCommandQueue->Idle());
}

void CMainFrame::OnDisconnect(wxCommandEvent& event)
{
	if (!m_pState->m_pEngine)
		return;

	if (!m_pState->m_pCommandQueue->Idle())
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
	delete m_pSendLed;
	delete m_pRecvLed;
	m_pSendLed = 0;
	m_pRecvLed = 0;

	m_transferStatusTimer.Stop();

	bool res = true;
	if (m_pState->m_pCommandQueue)
		res = m_pState->m_pCommandQueue->Cancel();

	if (!res)
	{
		event.Veto();
		return;
	}
	m_pState->DestroyEngine();

	if (!m_pQueueView->Quit())
	{
		event.Veto();
		return;
	}

	CSiteManager::ClearIdMap();

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
	int oldLang = wxGetApp().GetCurrentLanguage();

	int oldShowDebugMenu = pOptions->GetOptionVal(OPTION_DEBUG_MENU) != 0;

	int res = dlg.ShowModal();
	if (res != wxID_OK)
	{
		UpdateLayout();
		return;
	}

	wxString newTheme = pOptions->GetOption(OPTION_THEME);
	int newLang = wxGetApp().GetCurrentLanguage();

	if (oldTheme != newTheme)
	{
		wxArtProvider::Delete(m_pThemeProvider);
		m_pThemeProvider = new CThemeProvider();
	}
	if (oldTheme != newTheme || oldLang != newLang)
		CreateToolBar();
	if (oldLang != newLang ||
		oldShowDebugMenu != pOptions->GetOptionVal(OPTION_DEBUG_MENU))
	{
		m_pStatusView->InitDefAttr();
		CreateMenus();
	}
	if (oldLang != newLang)
	{
		CreateQuickconnectBar();
		wxGetApp().GetWrapEngine()->CheckLanguage();
	}

#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	if (pOptions->GetOptionVal(OPTION_UPDATECHECK))
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
		const int layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);
		const int swap = COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP);
		if (layout == 3 && swap)
			m_pLocalSplitter->SplitVertically(m_pLocalListViewPanel, m_pLocalTreeViewPanel, m_lastLocalTreeSplitterPos);
		else if (layout)
			m_pLocalSplitter->SplitVertically(m_pLocalTreeViewPanel, m_pLocalListViewPanel, m_lastLocalTreeSplitterPos);
		else
			m_pLocalSplitter->SplitHorizontally(m_pLocalTreeViewPanel, m_pLocalListViewPanel, m_lastLocalTreeSplitterPos);
	}
	COptions::Get()->SetOption(OPTION_SHOW_TREE_LOCAL, m_pLocalSplitter->IsSplit());
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
		const int layout = COptions::Get()->GetOptionVal(OPTION_FILEPANE_LAYOUT);
		const int swap = COptions::Get()->GetOptionVal(OPTION_FILEPANE_SWAP);
		if (layout == 3 && !swap)
			m_pRemoteSplitter->SplitVertically(m_pRemoteListViewPanel, m_pRemoteTreeViewPanel, m_lastRemoteTreeSplitterPos);
		else if (layout)
			m_pRemoteSplitter->SplitVertically(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel, m_lastRemoteTreeSplitterPos);
		else
			m_pRemoteSplitter->SplitHorizontally(m_pRemoteTreeViewPanel, m_pRemoteListViewPanel, m_lastRemoteTreeSplitterPos);
	}
	COptions::Get()->SetOption(OPTION_SHOW_TREE_REMOTE, m_pRemoteSplitter->IsSplit());
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
#ifdef EVT_TOOL_DROPDOWN
	if (event.GetEventType() == wxEVT_COMMAND_TOOL_DROPDOWN_CLICKED)
	{
		m_pToolBar->SetDropdownMenu(XRCID("ID_TOOLBAR_SITEMANAGER"), pMenu);
		event.Skip();
	}
	else
#endif
	{
		if (!pMenu)
			return;
#ifdef __WXMSW__
		RECT r;
        if (::SendMessage((HWND)m_pToolBar->GetHandle(), TB_GETITEMRECT, m_pToolBar->GetToolPos(event.GetId()), (LPARAM)&r))
			m_pToolBar->PopupMenu(pMenu, r.left, r.bottom);
		else
#endif
		m_pToolBar->PopupMenu(pMenu);

		delete pMenu;
	}
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
