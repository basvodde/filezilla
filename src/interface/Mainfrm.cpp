#include "filezilla.h"
#include "LocalTreeView.h"
#include "LocalListView.h"
#include "RemoteTreeView.h"
#include "RemoteListView.h"
#include "StatusView.h"
#include "QueueView.h"
#include "Mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(CMainFrame, wxFrame)
    EVT_SIZE(CMainFrame::OnSize)
	EVT_SPLITTER_SASH_POS_CHANGED(-1, CMainFrame::OnViewSplitterPosChanged)
	EVT_MENU(wxID_ANY, CMainFrame::OnMenuHandler)
END_EVENT_TABLE()

CMainFrame::CMainFrame() : wxFrame(NULL, -1, "FileZilla", wxDefaultPosition, wxSize(900, 750))
{
	SetSizeHints(250, 250);

	m_pStatusBar = NULL;
    m_pMenuBar = NULL;
	m_pQuickconnectBar = NULL;
	m_pTopSplitter = NULL;
	m_pBottomSplitter = NULL;
	m_pViewSplitter = NULL;
	m_pLocalSplitter = NULL;
	m_pRemoteSplitter = NULL;

	m_pStatusBar = CreateStatusBar(7);

	CreateToolBar();
	CreateMenus();
	CreateQuickconnectBar();

	m_ViewSplitterSashPos = 0.5;

	m_pTopSplitter = new wxSplitterWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER | wxSP_LIVE_UPDATE);
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
	m_pLocalListView = new CLocalListView(m_pLocalSplitter, -1);
	m_pRemoteTreeView = new CRemoteTreeView(m_pRemoteSplitter, -1);
	m_pRemoteListView = new CRemoteListView(m_pRemoteSplitter, -1);
	m_pStatusView = new CStatusView(m_pTopSplitter, -1);
	m_pQueueView = new CQueueView(m_pBottomSplitter, -1);

	m_pTopSplitter->SplitHorizontally(m_pStatusView, m_pBottomSplitter, 100);
	m_pBottomSplitter->SplitHorizontally(m_pViewSplitter, m_pQueueView);
	m_pViewSplitter->SplitVertically(m_pLocalSplitter, m_pRemoteSplitter);
	m_pLocalSplitter->SplitHorizontally(m_pLocalTreeView, m_pLocalListView);
	m_pRemoteSplitter->SplitHorizontally(m_pRemoteTreeView, m_pRemoteListView);
	wxSize size = m_pBottomSplitter->GetClientSize();
	m_pBottomSplitter->SetSashPosition(size.GetHeight() - 100);

	m_pEngine = new CFileZillaEngine();
	m_pEngine->Init(this);
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::OnSize(wxSizeEvent &event)
{
	float ViewSplitterSashPos = m_ViewSplitterSashPos;
	if (!m_pBottomSplitter)
	{
		event.Skip();
		return;
	}

	int pos = m_pBottomSplitter->GetSashPosition();
	wxSize size = m_pBottomSplitter->GetClientSize();
	
	wxFrame::OnSize(event);

	wxSize clientSize = GetClientSize();
	if (m_pQuickconnectBar)
	{
		m_pQuickconnectBar->SetSize(0, 0, clientSize.GetWidth(), -1);

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
	m_pBottomSplitter->SetSashPosition(size2.GetHeight() - size.GetHeight() + pos);

	m_ViewSplitterSashPos = ViewSplitterSashPos;

	if (m_pViewSplitter)
	{
		size = m_pViewSplitter->GetClientSize();
		int pos = size.GetWidth() * m_ViewSplitterSashPos;
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
