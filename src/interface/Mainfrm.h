#ifndef __MAINFRM_H__
#define __MAINFRM_H__

class CStatusView;
class CQueueView;
class CLocalTreeView;
class CLocalListView;
class CRemoteTreeView;
class CRemoteListView;
class CState;
class COptions;
class CCommandQueue;
class CAsyncRequestQueue;
class CLed;
class CThemeProvider;

class CMainFrame : public wxFrame
{
public:
	CMainFrame(COptions* pOptions);
	virtual ~CMainFrame();

	bool GetPassword(CServer& server, wxString name = _T(""), wxString challenge = _T(""));
	COptions* m_pOptions;

	void UpdateSendLed();
	void UpdateRecvLed();

	void AddToRequestQueue(CFileZillaEngine* pEngine, CAsyncRequestNotification* pNotification);
	CState* GetState() { return m_pState; }
	CStatusView* GetStatusView() { return m_pStatusView; }
	CLocalListView* GetLocalListView() { return m_pLocalListView; }
	CRemoteListView* GetRemoteListView() { return m_pRemoteListView; }

protected:
	bool CreateMenus();
	bool CreateQuickconnectBar();
	bool CreateToolBar();
	
	void SetProgress(const CTransferStatus* pStatus);

	wxStatusBar* m_pStatusBar;
	wxMenuBar* m_pMenuBar;
	wxToolBar* m_pToolBar;
	wxPanel* m_pQuickconnectBar;
	wxSplitterWindow* m_pTopSplitter;
	wxSplitterWindow* m_pBottomSplitter;
	wxSplitterWindow* m_pViewSplitter;
	wxSplitterWindow* m_pLocalSplitter;
	wxSplitterWindow* m_pRemoteSplitter;

	CStatusView* m_pStatusView;
	CQueueView* m_pQueueView;
	CLocalTreeView* m_pLocalTreeView;
	CLocalListView* m_pLocalListView;
	CRemoteTreeView* m_pRemoteTreeView;
	CRemoteListView* m_pRemoteListView;
	CLed* m_pRecvLed;
	CLed* m_pSendLed;
	wxTimer m_sendLedTimer, m_recvLedTimer;
	wxTimer m_transferStatusTimer;
	CThemeProvider* m_pThemeProvider;

	// Event handlers
	void OnSize(wxSizeEvent& event);
	void OnViewSplitterPosChanged(wxSplitterEvent& event);
	void OnMenuHandler(wxCommandEvent& event);
	void OnQuickconnect(wxCommandEvent& event);
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
	void OnUpdateToolbarRefresh(wxUpdateUIEvent& event);
	void OnRefresh(wxCommandEvent& event);
	void OnStatusbarSize(wxSizeEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnSiteManager(wxCommandEvent& event);
	void OnProcessQueue(wxCommandEvent& event);
	void OnUpdateToolbarProcessQueue(wxUpdateUIEvent& event);
	void OnMenuEditSettings(wxCommandEvent& event);

	float m_ViewSplitterSashPos;
	bool m_bInitDone;
	bool m_bQuit;

	CFileZillaEngine* m_pEngine;
	CCommandQueue* m_pCommandQueue;
	CAsyncRequestQueue* m_pAsyncRequestQueue;
	CState* m_pState;

#ifdef __WXMSW__
	bool m_windowIsMaximized;
#endif

	DECLARE_EVENT_TABLE()
};

#endif

