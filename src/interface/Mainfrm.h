#ifndef __MAINFRM_H__
#define __MAINFRM_H__

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
class CSiteManagerItemData;
class CQueue;
class CViewHeader;

class CMainFrame : public wxFrame
{
public:
	CMainFrame();
	virtual ~CMainFrame();

	void UpdateSendLed();
	void UpdateRecvLed();

	void AddToRequestQueue(CFileZillaEngine* pEngine, CAsyncRequestNotification* pNotification);
	CState* GetState() { return m_pState; }
	CStatusView* GetStatusView() { return m_pStatusView; }
	CLocalListView* GetLocalListView() { return m_pLocalListView; }
	CRemoteListView* GetRemoteListView() { return m_pRemoteListView; }
	CQueueView* GetQueue() { return m_pQueueView; }
	CQuickconnectBar* GetQuickconnectBar() { return m_pQuickconnectBar; }

	void UpdateLayout(int layout = -1, int swap = -1);

	// Window size and position as well as pane sizes
	void RememberSizes();
	void RestoreSizes();

	void CheckChangedSettings();

	void ConnectNavigationHandler(wxEvtHandler* handler);

protected:
	bool CreateMenus();
	bool CreateQuickconnectBar();
	bool CreateToolBar();
	void SetProgress(const CTransferStatus* pStatus);
	void ConnectToSite(CSiteManagerItemData* const pData);

	// If resizing the window, make sure the individual splitter windows don't get too small
	void ApplySplitterConstraints();

	void FocusNextEnabled(std::list<wxWindow*>& windowOrder, std::list<wxWindow*>::iterator iter, bool skipFirst, bool forward);

	wxStatusBar* m_pStatusBar;
	wxMenuBar* m_pMenuBar;
	wxToolBar* m_pToolBar;
	CQuickconnectBar* m_pQuickconnectBar;
	wxSplitterWindow* m_pTopSplitter;
	wxSplitterWindow* m_pBottomSplitter;
	wxSplitterWindow* m_pViewSplitter;
	wxSplitterWindow* m_pLocalSplitter;
	wxSplitterWindow* m_pRemoteSplitter;

	CStatusView* m_pStatusView;
	CQueueView* m_pQueueView;
	CView* m_pLocalTreeViewPanel;
	CView* m_pLocalListViewPanel;
	CLocalTreeView* m_pLocalTreeView;
	CLocalListView* m_pLocalListView;
	CView* m_pRemoteTreeViewPanel;
	CView* m_pRemoteListViewPanel;
	CRemoteTreeView* m_pRemoteTreeView;
	CRemoteListView* m_pRemoteListView;
	CViewHeader* m_pLocalViewHeader;
	CViewHeader* m_pRemoteViewHeader;
	CLed* m_pRecvLed;
	CLed* m_pSendLed;
	wxTimer m_transferStatusTimer;
	CThemeProvider* m_pThemeProvider;
#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
	CUpdateWizard* m_pUpdateWizard;
#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

	// Event handlers
	DECLARE_EVENT_TABLE()
	void OnSize(wxSizeEvent& event);
	void OnViewSplitterPosChanged(wxSplitterEvent& event);
	void OnMenuHandler(wxCommandEvent& event);
	void OnMenuOpenHandler(wxMenuEvent& event);
	void OnEngineEvent(wxEvent& event);
	void OnUpdateToolbarDisconnect(wxUpdateUIEvent& event);
	void OnDisconnect(wxCommandEvent& event);
	void OnUpdateToolbarCancel(wxUpdateUIEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnSplitterSashPosChanging(wxSplitterEvent& event);
	void OnSplitterSashPosChanged(wxSplitterEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnUpdateToolbarReconnect(wxUpdateUIEvent& event);
	void OnReconnect(wxCommandEvent& event);
	void OnRefresh(wxCommandEvent& event);
	void OnStatusbarSize(wxSizeEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnSiteManager(wxCommandEvent& event);
	void OnProcessQueue(wxCommandEvent& event);
	void OnUpdateToolbarProcessQueue(wxUpdateUIEvent& event);
	void OnMenuEditSettings(wxCommandEvent& event);
	void OnToggleLogView(wxCommandEvent& event);
	void OnUpdateToggleLogView(wxUpdateUIEvent& event);
	void OnToggleLocalTreeView(wxCommandEvent& event);
	void OnUpdateToggleLocalTreeView(wxUpdateUIEvent& event);
	void OnToggleRemoteTreeView(wxCommandEvent& event);
	void OnUpdateToggleRemoteTreeView(wxUpdateUIEvent& event);
	void OnToggleQueueView(wxCommandEvent& event);
	void OnUpdateToggleQueueView(wxUpdateUIEvent& event);
	void OnMenuHelpAbout(wxCommandEvent& event);
	void OnFilter(wxCommandEvent& event);
	void OnUpdateMenuCustomcommand(wxUpdateUIEvent& event);
#if FZ_MANUALUPDATECHECK
	void OnCheckForUpdates(wxCommandEvent& event);
#endif //FZ_MANUALUPDATECHECK
	void OnSitemanagerDropdown(wxCommandEvent& event);
	void OnUpdateMenuShowHidden(wxUpdateUIEvent& event);
	void OnNavigationKeyEvent(wxNavigationKeyEvent& event);
	void OnGetFocus(wxFocusEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnMoveEvent(wxMoveEvent& event);
	void OnActivate(wxActivateEvent& event);
#ifdef __WXMSW__
	void OnSizePost(wxCommandEvent& event);

	bool m_pendingPostSizing;
#endif

	float m_ViewSplitterSashPos;
	bool m_bInitDone;
	bool m_bQuit;

	CAsyncRequestQueue* m_pAsyncRequestQueue;
	CState* m_pState;

	// Variables to remember the splitter position on unsplit
	int m_lastLogViewSplitterPos;
	int m_lastLocalTreeSplitterPos;
	int m_lastRemoteTreeSplitterPos;
	int m_lastQueueSplitterPos;

#ifdef __WXMSW__
	bool m_windowIsMaximized;
#endif

	CQueue* m_pQueuePane;

	bool m_lastMaximized;
	wxPoint m_lastWindowPosition;
	wxSize m_lastWindowSize;
};

#endif

