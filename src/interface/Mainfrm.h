#pragma once

class CStatusView;
class CQueueView;
class CLocalTreeView;
class CLocalListView;
class CRemoteTreeView;
class CRemoteListView;

class CMainFrame : public wxFrame
{
public:
    CMainFrame();
    virtual ~CMainFrame();
protected:
    wxStatusBar *m_pStatusBar;
    wxMenuBar	*m_pMenuBar;
	wxSplitterWindow *m_pTopSplitter;
	wxSplitterWindow *m_pBottomSplitter;
	wxSplitterWindow *m_pViewSplitter;
	wxSplitterWindow *m_pLocalSplitter;
	wxSplitterWindow *m_pRemoteSplitter;

	CStatusView *m_pStatusView;
	CQueueView *m_pQueueView;
	CLocalTreeView *m_pLocalTreeView;
	CLocalListView *m_pLocalListView;
	CRemoteTreeView *m_pRemoteTreeView;
	CRemoteListView *m_pRemoteListView;

    void OnSize(wxSizeEvent& event);
	void OnViewSplitterPosChanged(wxSplitterEvent& event);

	float m_ViewSplitterSashPos;

	CFileZillaEngine *m_pEngine;

	DECLARE_EVENT_TABLE()
};