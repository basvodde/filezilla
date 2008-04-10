#include "FileZilla.h"
#include "state.h"
#include "commandqueue.h"
#include "FileZillaEngine.h"
#include "Options.h"
#include "Mainfrm.h"
#include "queue.h"
#include "filezillaapp.h"
#include "RemoteListView.h"
#include "recursive_operation.h"
#include "statusbar.h"

CState::CState(CMainFrame* pMainFrame)
{
	m_pMainFrame = pMainFrame;

	m_pDirectoryListing = 0;
	m_pServer = 0;

	m_pEngine = 0;
	m_pCommandQueue = 0;

	m_pRecursiveOperation = new CRecursiveOperation(this);
}

CState::~CState()
{
	delete m_pDirectoryListing;
	delete m_pServer;

	delete m_pCommandQueue;
	delete m_pEngine;

	// Unregister all handlers
	for (int i = 0; i < STATECHANGE_MAX; i++)
	{
		for (std::list<CStateEventHandler*>::iterator iter = m_handlers[i].begin(); iter != m_handlers[i].end(); iter++)
			(*iter)->m_pState = 0;
	}

	delete m_pRecursiveOperation;
}

wxString CState::GetLocalDir() const
{
	return m_localDir;
}

wxString CState::Canonicalize(wxString oldDir, wxString newDir, wxString *error /*=0*/)
{
#ifdef __WXMSW__
	if (newDir == _T("\\") || newDir == _T("/") || newDir == _T(""))
		return _T("\\");

	// "Go up one level" is a little bit difficult under Windows due to 
	// things like "My Computer" and "Desktop"
	if (newDir == _T(".."))
	{
		newDir = oldDir;
		if (newDir != _T("\\"))
		{
			newDir.RemoveLast();
			int pos = newDir.Find('\\', true);
			if (pos == -1)
				return _T("\\");
			else
				newDir = newDir.Left(pos + 1);
		}
	}
	else
#endif
	{
		wxFileName dir(newDir, _T(""));
		{
			wxLogNull noLog;
			if (!dir.MakeAbsolute(oldDir))
				return _T("");
		}
		newDir = dir.GetFullPath();
		if (newDir.Right(1) != wxFileName::GetPathSeparator())
			newDir += wxFileName::GetPathSeparator();
	}

	// Check for partial UNC paths
	if (newDir.Left(2) == _T("\\\\"))
	{
		int pos = newDir.Mid(2).Find('\\');
		if (pos == -1 || pos + 3 == (int)newDir.Len())
		{
			// Partial UNC path, no share given
			return newDir;
		}

		pos = newDir.Mid(pos + 3).Find('\\');
		if (pos == -1)
		{
			// Partial UNC path, no full share yet, skip further processing
			return _T("");
		}
	}

	if (!wxDir::Exists(newDir))
	{
		if (!error)
			return _T("");

		*error = wxString::Format(_("'%s' does not exist or cannot be accessed."), newDir.c_str());

#ifdef __WXMSW__
		if (newDir[0] == '\\')
			return _T("");

		// Check for removable drive, display a more specific error message in that case
		if (::GetLastError() != ERROR_NOT_READY)
			return _T("");
		int type = GetDriveType(newDir.Left(3));
		if (type == DRIVE_REMOVABLE || type == DRIVE_CDROM)

			*error = wxString::Format(_("Cannot access '%s', no media inserted or drive not ready."), newDir.c_str());
#endif
			
		return _T("");
	}

	return newDir;
}

bool CState::SetLocalDir(wxString dir, wxString *error /*=0*/)
{
	dir = Canonicalize(m_localDir, dir, error);
	if (dir == _T(""))
		return false;

	m_localDir = dir;

	COptions::Get()->SetOption(OPTION_LASTLOCALDIR, dir);

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

void CState::RefreshLocalFile(wxString file)
{
	wxFileName fn(file);
	if (!fn.IsOk())
		return;
	if (!fn.IsAbsolute())
		return;

	wxString path = fn.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	if (path == _T(""))
		return;

	if (path == m_localDir)
	{
		file = fn.GetFullName();
		if (file == _T(""))
			return;
	}
	else if (path.Left(m_localDir.Len()) == m_localDir)
	{
		path = path.Mid(m_localDir.Len());
		int pos = path.Find(wxFileName::GetPathSeparator());
		if (pos < 1)
			return;

		file = path.Left(pos);
	}

	NotifyHandlers(STATECHANGE_LOCAL_REFRESH_FILE, file);
}

void CState::SetServer(const CServer* server)
{
	if (m_pServer)
	{
		SetRemoteDir(0);
		delete m_pServer;
	}

	CStatusBar* const pStatusBar = m_pMainFrame->GetStatusBar();
	if (pStatusBar)
	{
		pStatusBar->DisplayDataType(server);
		pStatusBar->DisplayEncrypted(server);
	}

	if (server)
		m_pServer = new CServer(*server);
	else
	{
		if (m_pServer)
			m_pMainFrame->SetTitle(_T("FileZilla"));
		m_pServer = 0;
	}

	NotifyHandlers(STATECHANGE_SERVER);
}

const CServer* CState::GetServer() const
{
	return m_pServer;
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

	const wxString& name = server.GetName();
	if (!name.IsEmpty())
		m_pMainFrame->SetTitle(name + _T(" - ") + server.FormatServer() + _T(" - FileZilla"));
	else
		m_pMainFrame->SetTitle(server.FormatServer() + _T(" - FileZilla"));

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

void CState::RegisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification)
{
	wxASSERT(pHandler);
	wxASSERT(notification != STATECHANGE_MAX && notification != STATECHANGE_NONE);

	std::list<CStateEventHandler*> &handlers = m_handlers[notification];
	std::list<CStateEventHandler*>::const_iterator iter;
	for (iter = handlers.begin(); iter != handlers.end(); iter++)
	{
		if (*iter == pHandler)
			return;
	}

	handlers.push_back(pHandler);
}

void CState::UnregisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification)
{
	wxASSERT(pHandler);
	wxASSERT(notification != STATECHANGE_MAX);

	if (notification == STATECHANGE_NONE)
	{
		for (int i = 0; i < STATECHANGE_MAX; i++)
		{
			std::list<CStateEventHandler*> &handlers = m_handlers[notification];
			for (std::list<CStateEventHandler*>::iterator iter = handlers.begin(); iter != handlers.end(); iter++)
			{
				if (*iter == pHandler)
				{
					handlers.erase(iter);
					break;
				}
			}
		}
	}
	else
	{
		std::list<CStateEventHandler*> &handlers = m_handlers[notification];
		for (std::list<CStateEventHandler*>::iterator iter = handlers.begin(); iter != handlers.end(); iter++)
		{
			if (*iter == pHandler)
			{
				handlers.erase(iter);
				return;
			}
		}
	}
}

void CState::NotifyHandlers(enum t_statechange_notifications notification, const wxString& data /*=_T("")*/)
{
	wxASSERT(notification != STATECHANGE_NONE && notification != STATECHANGE_MAX);

	const std::list<CStateEventHandler*> &handlers = m_handlers[notification];
	for (std::list<CStateEventHandler*>::const_iterator iter = handlers.begin(); iter != handlers.end(); iter++)
	{
		(*iter)->OnStateChange(notification, data);
	}
}

CStateEventHandler::CStateEventHandler(CState* pState)
{
	wxASSERT(pState);

	if (!pState)
		return;

	m_pState = pState;
}

CStateEventHandler::~CStateEventHandler()
{
	if (!m_pState)
		return;
	m_pState->UnregisterHandler(this, STATECHANGE_NONE);
}

void CState::UploadDroppedFiles(const wxFileDataObject* pFileDataObject, const wxString& subdir, bool queueOnly)
{
	if (!m_pServer || !m_pDirectoryListing)
		return;

	CServerPath path = m_pDirectoryListing->path;
	if (subdir == _T("..") && path.HasParent())
		path = path.GetParent();
	else if (subdir != _T(""))
		path.AddSegment(subdir);

	UploadDroppedFiles(pFileDataObject, path, queueOnly);
}

void CState::UploadDroppedFiles(const wxFileDataObject* pFileDataObject, const CServerPath& path, bool queueOnly)
{
	if (!m_pServer)
		return;

	const wxArrayString& files = pFileDataObject->GetFilenames();
	
	for (unsigned int i = 0; i < files.Count(); i++)
	{
		if (wxFile::Exists(files[i]))
		{
			const wxFileName name(files[i]);
			const wxLongLong size = name.GetSize().GetValue();
			m_pMainFrame->GetQueue()->QueueFile(queueOnly, false, files[i], name.GetFullName(), path, *m_pServer, size);
		}
		else if (wxDir::Exists(files[i]))
		{
			wxString name = files[i];
			int pos = name.Find(wxFileName::GetPathSeparator(), true);
			if (pos != -1)
			{
				name = name.Mid(pos + 1);
				CServerPath target = path;
				target.AddSegment(name);
				m_pMainFrame->GetQueue()->QueueFolder(queueOnly, false, files[i], target, *m_pServer);
			}
		}
	}
}

void CState::HandleDroppedFiles(const wxFileDataObject* pFileDataObject, wxString path, bool copy)
{
	if (path.Last() != wxFileName::GetPathSeparator())
		path += wxFileName::GetPathSeparator();

	const wxArrayString &files = pFileDataObject->GetFilenames();
	if (!files.Count())
		return;

#ifdef __WXMSW__
	int len = 1;

	for (unsigned int i = 0; i < files.Count(); i++)
		len += files[i].Len() + 1;

	wxChar* from = new wxChar[len];
	wxChar* p = from;
	for (unsigned int i = 0; i < files.Count(); i++)
	{
		wxStrcpy(p, files[i]);
		p += files[i].Len() + 1;
	}
	*p = 0;

	wxChar* to = new wxChar[path.Len() + 2];
	wxStrcpy(to, path);
	to[path.Len() + 1] = 0;

	SHFILEOPSTRUCT op = {0};
	op.pFrom = from;
	op.pTo = to;
	op.wFunc = copy ? FO_COPY : FO_MOVE;
	op.hwnd = (HWND)m_pMainFrame->GetHandle();
	SHFileOperation(&op);

	delete [] to;
	delete [] from;
#else
	for (unsigned int i = 0; i < files.Count(); i++)
	{
		const wxString& file = files[i];
		if (wxFile::Exists(file))
		{
			int pos = file.Find(wxFileName::GetPathSeparator(), true);
			if (pos == -1 || pos == (int)file.Len() - 1)
				continue;
			const wxString& name = file.Mid(pos + 1);
			if (copy)
				wxCopyFile(file, path + name);
			else
				wxRenameFile(file, path + name);
		}
		else if (wxDir::Exists(file))
		{
			if (copy)
				RecursiveCopy(file, path);
			else
			{
				int pos = file.Find(wxFileName::GetPathSeparator(), true);
				if (pos == -1 || pos == (int)file.Len() - 1)
					continue;
				const wxString& name = file.Mid(pos + 1);
				wxRenameFile(file, path + name);
			}
		}
	}
#endif

	RefreshLocal();
}

bool CState::RecursiveCopy(wxString source, wxString target)
{
	if (source == _T(""))
		return false;

	if (target == _T(""))
		return false;

	if (target.Last() != wxFileName::GetPathSeparator())
		target += wxFileName::GetPathSeparator();

	if (source.Last() == wxFileName::GetPathSeparator())
		source.RemoveLast();

	if (source + wxFileName::GetPathSeparator() == target)
		return false;

	if (target.Len() > source.Len() && source == target.Left(source.Len()) && target[source.Len()] == wxFileName::GetPathSeparator())
		return false;
	
	int pos = source.Find(wxFileName::GetPathSeparator(), true);
	if (pos == -1 || pos == (int)source.Len() - 1)
		return false;

	std::list<wxString> dirsToVisit;
	dirsToVisit.push_back(source.Mid(pos + 1) + wxFileName::GetPathSeparator());
	source = source.Left(pos + 1);

	// Process any subdirs which still have to be visited
	while (!dirsToVisit.empty())
	{
		wxString dirname = dirsToVisit.front();
		dirsToVisit.pop_front();
		wxMkdir(target + dirname);
		wxDir dir;
		if (!dir.Open(source + dirname))
			continue;

		wxString file;
		for (bool found = dir.GetFirst(&file); found; found = dir.GetNext(&file))
		{
			if (file == _T(""))
			{
				wxGetApp().DisplayEncodingWarning();
				continue;
			}
			if (wxFileName::DirExists(source + dirname + file))
			{
				const wxString subDir = dirname + file + wxFileName::GetPathSeparator();
				dirsToVisit.push_back(subDir);
			}
			else
				wxCopyFile(source + dirname + file, target + dirname + file);
		}
	}

	return true;
}

bool CState::DownloadDroppedFiles(const CRemoteDataObject* pRemoteDataObject, wxString path, bool queueOnly /*=false*/)
{
	bool hasDirs = false;
	bool hasFiles = false;
	const std::list<CRemoteDataObject::t_fileInfo>& files = pRemoteDataObject->GetFiles();
	for (std::list<CRemoteDataObject::t_fileInfo>::const_iterator iter = files.begin(); iter != files.end(); iter++)
	{
		if (iter->dir)
			hasDirs = true;
		else
			hasFiles = true;
	}

	if (hasDirs)
	{
		if (!m_pEngine->IsConnected() || m_pEngine->IsBusy() || !m_pCommandQueue->Idle())
			return false;
	}

	if (hasFiles)
		m_pMainFrame->GetQueue()->QueueFiles(queueOnly, path, *pRemoteDataObject);

	if (!hasDirs)
		return true;

	return m_pMainFrame->GetRemoteListView()->DownloadDroppedFiles(pRemoteDataObject, path, queueOnly);
}

bool CState::IsRemoteConnected() const
{
	if (!m_pEngine)
		return false;

	return m_pEngine->IsConnected();
}

bool CState::IsRemoteIdle() const
{
	if (m_pRecursiveOperation->GetOperationMode() != CRecursiveOperation::recursive_none)
		return false;

	if (!m_pCommandQueue)
		return true;

	return m_pCommandQueue->Idle();
}

bool CState::LocalDirHasParent(const wxString& dir)
{
#ifdef __WXMSW__
	if (dir.Left(2) == _T("\\\\"))
	{
		int pos = dir.Mid(2).Find('\\');
		if (pos == -1 || pos + 3 == (int)dir.Len())
			return false;
	}
	if (dir == _T("\\") || dir == _T("//"))
		return false;
#endif

	if (dir == _T("/"))
		return false;

	return true;
}

bool CState::LocalDirIsWriteable(const wxString& dir)
{
#ifdef __WXMSW__
	if (dir == _T("\\") || dir == _T("//"))
		return false;

	if (dir.Left(2) == _T("\\\\"))
	{
		int pos = dir.Mid(2).Find('\\');
		if (pos == -1 || pos + 3 == (int)dir.Len())
			return false;
	}
#endif

	return true;
}
