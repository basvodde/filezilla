#include "FileZilla.h"
#include "state.h"
#include "commandqueue.h"
#include "FileZillaEngine.h"
#include "Options.h"
#include "Mainfrm.h"
#include "QueueView.h"

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

bool CState::SetLocalDir(wxString dir, wxString *error /*=0*/)
{
#ifdef __WXMSW__
	if (dir == _T("\\") || dir == _T("/") || dir == _T(""))
	{
		m_localDir = _T("\\");
		NotifyHandlers(STATECHANGE_LOCAL_DIR);
		return true;
	}

	// "Go up one level" is a little bit difficult under Windows due to 
	// things like "My Computer" and "Desktop"
	if (dir == _T(".."))
	{
		dir = m_localDir;
		if (dir != _T("\\"))
		{
			dir.Truncate(dir.Length() - 1);
			int pos = dir.Find('\\', true);
			if (pos == -1)
				dir = _T("\\");
			else
				dir = dir.Left(pos + 1);
		}
	}
	else
#endif
	{
		wxFileName newDir(dir, _T(""));
		{
			wxLogNull noLog;
			if (!newDir.MakeAbsolute(m_localDir))
				return false;
		}
		dir = newDir.GetFullPath();
		if (dir.Right(1) != wxFileName::GetPathSeparator())
			dir += wxFileName::GetPathSeparator();
	}

	// Check for partial UNC paths
	if (dir.Left(2) == _T("\\\\"))
	{
		int pos = dir.Mid(2).Find('\\');
		if (pos == -1)
		{
			// Partial UNC path, no full server yet, skip further processing
			return false;
		}

		pos = dir.Mid(pos + 3).Find('\\');
		if (pos == -1)
		{
			// Partial UNC path, no full share yet, skip further processing
			return false;
		}
	}

	if (!wxDir::Exists(dir))
	{
		if (!error)
			return false;

		if (error)
			*error = wxString::Format(_("'%s' does not exist or cannot be accessed."), dir.c_str());

#ifdef __WXMSW__
		if (dir[0] == '\\')
			return false;

		// Check for removable drive, display a more specific error message in that case
		if (::GetLastError() != ERROR_NOT_READY)
			return false;
		int type = GetDriveType(dir.Left(3));
		if (type == DRIVE_REMOVABLE || type == DRIVE_CDROM)

			*error = wxString::Format(_("Cannot access '%s', no media inserted or drive not ready."), dir.c_str());
#endif
			
		return false;
	}
	
	m_localDir = dir;

	NotifyHandlers(STATECHANGE_LOCAL_DIR);

	return true;
}

bool CState::SetRemoteDir(const CDirectoryListing *pDirectoryListing, bool modified /*=false*/)
{
	if (!pDirectoryListing)
	{
		if (modified)
			return false;

		if (m_pDirectoryListing)
		{
			const CDirectoryListing* pOldListing = m_pDirectoryListing;
			m_pDirectoryListing = 0;
			NotifyHandlers(STATECHANGE_REMOTE_DIR);
			delete pOldListing;
		}
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
	if (m_pServer)
	{
		SetRemoteDir(0);
		delete m_pServer;
	}

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

void CState::UploadDroppedFiles(const wxFileDataObject* pFileDataObject, const wxString& subdir)
{
	if (!m_pServer || !m_pDirectoryListing)
		return;

	CServerPath path = m_pDirectoryListing->path;
	if (subdir == _T("..") && path.HasParent())
		path = path.GetParent();
	else if (subdir != _T(""))
		path.AddSegment(subdir);

	const wxArrayString& files = pFileDataObject->GetFilenames();
	
	for (unsigned int i = 0; i < files.Count(); i++)
	{
		if (wxFile::Exists(files[i]))
		{
			const wxFileName name(files[i]);
			const wxLongLong size = name.GetSize().GetValue();
			m_pMainFrame->GetQueue()->QueueFile(false, false, files[i], name.GetFullName(), path, *m_pServer, size);
		}
		else if (wxDir::Exists(files[i]))
		{
			wxString name = files[i];
			int pos = name.Find(wxFileName::GetPathSeparator(), true);
			if (pos != -1)
			{
				name = name.Mid(pos + 1);
				path.AddSegment(name);
				m_pMainFrame->GetQueue()->QueueFolder(false, false, files[i], path, *m_pServer);
			}
		}
	}
}
