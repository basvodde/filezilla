#include "FileZilla.h"
#include "state.h"
#include "LocalListView.h"
#include "LocalTreeView.h"
#include "RemoteListView.h"
#include "viewheader.h"

CState::CState()
{
	m_pLocalListView = 0;
	m_pLocalTreeView = 0;
	m_pRemoteListView = 0;

	m_pDirectoryListing = 0;
	m_pServer = 0;
	m_pLocalViewHeader = 0;
	m_pRemoteViewHeader = 0;
}

CState::~CState()
{
	delete m_pDirectoryListing;
	delete m_pServer;
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

	if (m_pLocalListView)
		m_pLocalListView->DisplayDir(m_localDir);

	if (m_pLocalTreeView)
		m_pLocalTreeView->SetDir(m_localDir);

	if (m_pLocalViewHeader)
		m_pLocalViewHeader->SetDir(m_localDir);

	return true;
}

void CState::SetLocalListView(CLocalListView *pLocalListView)
{
	m_pLocalListView = pLocalListView;
}

void CState::SetLocalTreeView(CLocalTreeView *pLocalTreeView)
{
	m_pLocalTreeView = pLocalTreeView;
}

void CState::SetRemoteListView(CRemoteListView *pRemoteListView)
{
	m_pRemoteListView = pRemoteListView;
}

bool CState::SetRemoteDir(const CDirectoryListing *pDirectoryListing, bool modified /*=false*/)
{
    if (!pDirectoryListing)
	{
		if (m_pRemoteListView)
			m_pRemoteListView->SetDirectoryListing(0);
		delete m_pDirectoryListing;
		m_pDirectoryListing = 0;
		return true;
	}

	CDirectoryListing *newListing = new CDirectoryListing;
	*newListing = *pDirectoryListing;
	
	if (m_pRemoteListView)
		m_pRemoteListView->SetDirectoryListing(newListing, modified);

	if (m_pRemoteViewHeader)
		m_pRemoteViewHeader->SetDir(newListing ? newListing->path : CServerPath());

	delete m_pDirectoryListing;
	m_pDirectoryListing = newListing;

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
	if (m_pLocalListView)
		m_pLocalListView->DisplayDir(m_localDir);
	if (m_pLocalTreeView)
		m_pLocalTreeView->Refresh();
}

void CState::SetServer(CServer* server)
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
