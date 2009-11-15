#ifndef __MAINFRM_H__
#define __MAINFRM_H__

#ifndef __WXMAC__
#include <wx/taskbar.h>
#endif
#include <wx/aui/auibook.h>

class CStatusView;
class CQueueView;
class CLocalTreeView;
class CLocalListView;
class CRemoteTreeView;
class CRemoteListView;
class CState;
class CAsyncRequestQueue;
class CLed;
class CThemeProvider;
class CView;
class CQuickconnectBar;
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
class CUpdateWizard;
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
class CSiteManagerItemData_Site;
class CQueue;
class CViewHeader;
class CWindowStateManager;
class CStatusBar;
class CMainFrameStateEventHandler;
class CSplitterWindowEx;
class wxAuiNotebookEx;

class CMainFrame : public wxFrame
{
	friend class CMainFrameStateEventHandler;
public:
	CMainFrame();
	virtual ~CMainFrame();

	void UpdateActivityLed(int direction);

	void AddToRequestQueue(CFileZillaEngine* pEngine, CAsyncRequestNotification* pNotification);
	CStatusView* GetStatusView() { return m_pStatusView; }
	CLocalListView* GetLocalListView() { return m_context_controls[m_current_context_controls].pLocalListView; }
	CRemoteListView* GetRemoteListView() { return m_context_controls[m_current_context_controls].pRemoteListView; }
	CQueueView* GetQueue() { return m_pQueueView; }
	CQuickconnectBar* GetQuickconnectBar() { return m_pQuickconnectBar; }

	void UpdateLayout(int layout = -1, int swap = -1, int messagelog_position = -1);

	// Window size and position as well as pane sizes
	void RememberSplitterPositions();
	bool RestoreSplitterPositions();
	void SetDefaultSplitterPositions();

	void CheckChangedSettings();

	void ConnectNavigationHandler(wxEvtHandler* handler);

	CStatusBar* GetStatusBar() { return m_pStatusBar; }

	void UpdateToolbarState();
	void UpdateMenubarState();

	void ProcessCommandLine();

	void ClearBookmarks();

	void PostInitialize();
	
	bool Connect(const CServer& server, const CServerPath& path = CServerPath());

protected:
	bool CreateMenus();
	bool CreateQuickconnectBar();
	bool CreateToolBar();
	void SetProgress(const CTransferStatus* pStatus);
	bool ConnectToSite(CSiteManagerItemData_Site* const pData);
	void OpenSiteManager(const CServer* pServer = 0);
	void InitToolbarState();
	void InitMenubarState();
	bool CloseTab(int tab);
	void CreateTab();
	void CreateContextControls(CState* pState);

	void FocusNextEnabled(std::list<wxWindow*>& windowOrder, std::list<wxWindow*>::iterator iter, bool skipFirst, bool forward);

	void SetBookmarksFromPath(const wxString& path);
	void UpdateBookmarkMenu();

	struct _context_controls
	{
		// List of all windows and controls assorted with a context
		CView* pLocalTreeViewPanel;
		CView* pLocalListViewPanel;
		CLocalTreeView* pLocalTreeView;
		CLocalListView* pLocalListView;
		CView* pRemoteTreeViewPanel;
		CView* pRemoteListViewPanel;
		CRemoteTreeView* pRemoteTreeView;
		CRemoteListView* pRemoteListView;
		CViewHeader* pLocalViewHeader;
		CViewHeader* pRemoteViewHeader;
	
		CSplitterWindowEx* pViewSplitter; // Contains local and remote splitters
		CSplitterWindowEx* pLocalSplitter;
		CSplitterWindowEx* pRemoteSplitter;

		CState* pState;

		wxString title;

		int tab_index;

		struct _site_bookmarks
		{
			wxString path;
			std::list<wxString> bookmarks;
		};
		CSharedPointer<struct _site_bookmarks> site_bookmarks;
	};

	std::vector<struct _context_controls> m_context_controls;
	size_t m_current_context_controls;

	std::list<int> m_bookmark_menu_ids;
	std::map<int, wxString> m_bookmark_menu_id_map_global;
	std::map<int, wxString> m_bookmark_menu_id_map_site;

	CStatusBar* m_pStatusBar;
	wxMenuBar* m_pMenuBar;
	wxToolBar* m_pToolBar;
	CQuickconnectBar* m_pQuickconnectBar;

	CSplitterWindowEx* m_pTopSplitter; // If log position is 0, splits message log from rest of panes
	CSplitterWindowEx* m_pBottomSplitter; // Top contains view splitter, bottom queue (or queuelog splitter if in position 1)
	CSplitterWindowEx* m_pQueueLogSplitter;

	CStatusView* m_pStatusView;
	CQueueView* m_pQueueView;
	CLed* m_pActivityLed[2];
	wxTimer m_transferStatusTimer;
	CThemeProvider* m_pThemeProvider;
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	CUpdateWizard* m_pUpdateWizard;
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

	void ShowLocalTree();
	void ShowRemoteTree();

#if defined(EVT_TOOL_DROPDOWN) && defined(__WXMSW__)
	void MakeDropdownTool(wxToolBar* pToolBar, int id);
#endif
	void ShowDropdownMenu(wxMenu* pMenu, wxToolBar* pToolBar, wxCommandEvent& event);

#ifdef __WXMSW__
	virtual WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);
#endif

	void HandleResize();

	// Event handlers
	DECLARE_EVENT_TABLE()
	void OnSize(wxSizeEvent& event);
	void OnMenuHandler(wxCommandEvent& event);
	void OnMenuOpenHandler(wxMenuEvent& event);
	void OnEngineEvent(wxEvent& event);
	void OnDisconnect(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnReconnect(wxCommandEvent& event);
	void OnRefresh(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnSiteManager(wxCommandEvent& event);
	void OnProcessQueue(wxCommandEvent& event);
	void OnMenuEditSettings(wxCommandEvent& event);
	void OnToggleLogView(wxCommandEvent& event);
	void OnToggleLocalTreeView(wxCommandEvent& event);
	void OnToggleRemoteTreeView(wxCommandEvent& event);
	void OnToggleQueueView(wxCommandEvent& event);
	void OnMenuHelpAbout(wxCommandEvent& event);
	void OnFilter(wxCommandEvent& event);
	void OnFilterRightclicked(wxCommandEvent& event);
#if FZ_MANUALUPDATECHECK
	void OnCheckForUpdates(wxCommandEvent& event);
#endif //FZ_MANUALUPDATECHECK
	void OnSitemanagerDropdown(wxCommandEvent& event);
	void OnNavigationKeyEvent(wxNavigationKeyEvent& event);
	void OnGetFocus(wxFocusEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnActivate(wxActivateEvent& event);
	void OnToolbarComparison(wxCommandEvent& event);
	void OnToolbarComparisonDropdown(wxCommandEvent& event);
	void OnDropdownComparisonMode(wxCommandEvent& event);
	void OnDropdownComparisonHide(wxCommandEvent& event);
	void OnSyncBrowse(wxCommandEvent& event);
#ifndef __WXMAC__
	void OnIconize(wxIconizeEvent& event);
	void OnTaskBarClick(wxTaskBarIconEvent& event);
#endif
#ifdef __WXGTK__
	void OnTaskBarClick_Delayed(wxCommandEvent& event);
#endif
	void OnSearch(wxCommandEvent& event);
	void OnMenuNewTab(wxCommandEvent& event);
	void OnMenuCloseTab(wxCommandEvent& event);
	void OnTabChanged(wxAuiNotebookEvent& event);
	void OnTabClosing(wxAuiNotebookEvent& event);
	void OnTabClosing_Deferred(wxCommandEvent& event);
	void OnTabBgDoubleclick(wxAuiNotebookEvent& event);

	bool m_bInitDone;
	bool m_bQuit;
	wxEventType m_closeEvent;
	wxTimer m_closeEventTimer;

	CAsyncRequestQueue* m_pAsyncRequestQueue;
	CMainFrameStateEventHandler* m_pStateEventHandler;

	CWindowStateManager* m_pWindowStateManager;

	CQueue* m_pQueuePane;

#ifndef __WXMAC__
	wxTaskBarIcon* m_taskBarIcon;
#endif
#ifdef __WXGTK__
	// There is a bug in KDE, causing the window to toggle iconized state
	// several times a second after uniconizing it from taskbar icon.
	// Set m_taskbar_is_uniconizing in OnTaskBarClick and unset the
	// next time the pending event processing runs and calls OnTaskBarClick_Delayed.
	// While set, ignore iconize events.
	bool m_taskbar_is_uniconizing;
#endif

	wxAuiNotebookEx* m_tabs;
};

#endif
