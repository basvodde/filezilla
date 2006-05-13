#include "FileZilla.h"
#include "state.h"
#include "commandqueue.h"
#include "FileZillaEngine.h"
#include "Options.h"
#include "Mainfrm.h"

CState::CState(CMainFrame* pMainFrame)
{
	m_pMainFrame = pMainFrame;

	m_pDirectoryListing = 0;
	m_pServer = 0;

	m_pEngine = 0;
	m_pCommandQueue = 0;
}

CState::~CState()
{
	delete m_pDirectoryListing;
	delete m_pServer;

	delete m_pCommandQueue;
	delete m_pEngine;

	// Unregister all handlers
	for (std::list<CStateEventHandler*>::iterator iter = m_handlers.begin(); iter != m_handlers.end(); iter++)
		(*iter)->m_pState = 0;
}

wxString CState::GetLocalDir() const
{
	return m_localDir;
}

bool CState::SetLocalDir(wxString dir)
{
	dir.Replace(_T("\\"), _T("/"));
	if (dir != _T(".."))
	{
#ifdef __WXMSW__
		if (dir == _T("") || dir == _T("/") || dir.c_str()[1] == ':' || m_localDir == _T("\\"))
			m_localDir = dir;
		else
#else
		if (dir.IsEmpty() || dir.c_str()[0] == '/')
			m_localDir = dir;
		else
#endif
			m_localDir += dir;

		if (m_localDir.Right(1) != _T("/"))
			m_localDir += _T("/");
	}
	else
	{
#ifdef __WXMSW__
		m_localDir.Replace(_T("\\"), _T("/"));
#endif

		if (m_localDir == _T("/"))
			m_localDir.Clear();
		else if (!m_localDir.IsEmpty())
		{
			m_localDir.Truncate(m_localDir.Length() - 1);
			int pos = m_localDir.Find('/', true);
			if (pos == -1)
				m_localDir = _T("/");
			else
				m_localDir = m_localDir.Left(pos + 1);
		}
	}

#ifdef __WXMSW__

	//Todo: Desktop and homedir
	if (m_localDir == _T(""))
		m_localDir = _T("\\");

	m_localDir.Replace(_T("/"), _T("\\"));
#else
	if (m_localDir == _T(""))
		m_localDir = _T("/");
#endif

	NotifyHandlers(STATECHANGE_LOCAL_DIR);

	return true;
}

bool CState::SetRemoteDir(const CDirectoryListing *pDirectoryListing, bool modified /*=false*/)
{
	if (!pDirectoryListing)
	{
		if (modified)
		{
			delete pDirectoryListing;
			return false;
		}

		const CDirectoryListing* pOldListing = m_pDirectoryListing;
		m_pDirectoryListing = 0;
		NotifyHandlers(STATECHANGE_REMOTE_DIR);
		delete pOldListing;
		return true;
	}

	if (modified)
	{
		if (!m_pDirectoryListing || m_pDirectoryListing->path != pDirectoryListing->path)
		{
			// We aren't interested in these listings
			delete pDirectoryListing;
			return true;
		}
	}
	else
		COptions::Get()->SetOption(OPTION_LASTSERVERPATH, pDirectoryListing->path.GetSafePath());
	
	if (m_pDirectoryListing && m_pDirectoryListing->path == pDirectoryListing->path &&
        pDirectoryListing->m_failed)
	{
		// We still got an old listing, no need to display the new one
		delete pDirectoryListing;
		return true;
	}

	const CDirectoryListing *pOldListing = m_pDirectoryListing;
	m_pDirectoryListing = pDirectoryListing;
	
	if (!modified)
		NotifyHandlers(STATECHANGE_REMOTE_DIR);
	else
		NotifyHandlers(STATECHANGE_REMOTE_DIR_MODIFIED);

	delete pOldListing;

	return true;
}

const CDirectoryListing *CState::GetRemoteDir() const
{
	return m_pDirectoryListing;
}

const CServerPath CState::GetRemotePath() const
{
	if (!m_pDirectoryListing)
		return CServerPath();
	
	return m_pDirectoryListing->path;
}

void CState::RefreshLocal()
{
	NotifyHandlers(STATECHANGE_LOCAL_DIR);
}

void CState::SetServer(const CServer* server)
{
	delete m_pServer;
	if (server)
		m_pServer = new CServer(*server);
	else
		m_pServer = 0;
}

const CServer* CState::GetServer() const
{
	return m_pServer;
}

void CState::ApplyCurrentFilter()
{
	NotifyHandlers(STATECHANGE_APPLYFILTER);
}

bool CState::Connect(const CServer& server, bool askBreak, const CServerPath& path /*=CServerPath()*/)
{
	if (!m_pEngine)
		return false;
	if (m_pEngine->IsConnected() || m_pEngine->IsBusy() || !m_pCommandQueue->Idle())
	{
		if (askBreak)
			if (wxMessageBox(_("Break current connection?"), _T("FileZilla"), wxYES_NO | wxICON_QUESTION) != wxYES)
				return false;
		m_pCommandQueue->Cancel();
	}

	SetServer(&server);
	m_pCommandQueue->ProcessCommand(new CConnectCommand(server));
	m_pCommandQueue->ProcessCommand(new CListCommand(path));
	
	COptions::Get()->SetLastServer(server);
	COptions::Get()->SetOption(OPTION_LASTSERVERPATH, path.GetSafePath());

	return true;
}

bool CState::CreateEngine()
{
	wxASSERT(!m_pEngine);
	if (m_pEngine)
		return true;

	m_pEngine = new CFileZillaEngine();
	m_pEngine->Init(m_pMainFrame, COptions::Get());

	m_pCommandQueue = new CCommandQueue(m_pEngine, m_pMainFrame);
	
	return true;
}

void CState::DestroyEngine()
{
	delete m_pCommandQueue;
	m_pCommandQueue = 0;
	delete m_pEngine;
	m_pEngine = 0;
}

void CState::RegisterHandler(CStateEventHandler* pHandler)
{
	wxASSERT(pHandler);

	std::list<CStateEventHandler*>::const_iterator iter;
	for (iter = m_handlers.begin(); iter != m_handlers.end(); iter++)
		if (*iter == pHandler)
			break;

	if (iter != m_handlers.end())
		return;

	m_handlers.push_back(pHandler);
}

void CState::UnregisterHandler(CStateEventHandler* pHandler)
{
	for (std::list<CStateEventHandler*>::iterator iter = m_handlers.begin(); iter != m_handlers.end(); iter++)
	{
		if (*iter == pHandler)
		{
			m_handlers.erase(iter);
			return;
		}
	}
}

void CState::NotifyHandlers(unsigned int event)
{
	for (std::list<CStateEventHandler*>::iterator iter = m_handlers.begin(); iter != m_handlers.end(); iter++)
	{
		if ((*iter)->m_eventMask & event)
			(*iter)->OnStateChange(event);
	}
}

CStateEventHandler::CStateEventHandler(CState* pState, unsigned int eventMask)
{
	wxASSERT(pState);
	wxASSERT(eventMask);

	if (!pState)
		return;

	m_pState = pState;
	m_eventMask = eventMask;

	m_pState->RegisterHandler(this);
}

CStateEventHandler::~CStateEventHandler()
{
	if (!m_pState)
		return;
	m_pState->UnregisterHandler(this);
}
