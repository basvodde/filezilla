#include "FileZilla.h"
#include "state.h"
#include "commandqueue.h"
#include "FileZillaEngine.h"
#include "Options.h"
#include "Mainfrm.h"
#include "queue.h"
#include "filezillaapp.h"
#include "recursive_operation.h"
#include "statusbar.h"
#include "local_filesys.h"
#include "listingcomparison.h"

CContextManager the_context_manager;

CContextManager* CContextManager::Get()
{
	return &the_context_manager;
}

CState* CContextManager::CreateState(CMainFrame* pMainFrame)
{
	wxASSERT(pMainFrame);

	CState* pState = new CState(pMainFrame);

	m_contexts.push_back(pState);

	return pState;
}

void CContextManager::DestroyState(CState* pState)
{
	for (unsigned int i = 0; i < m_contexts.size(); i++)
	{
		if (m_contexts[i] != pState)
			continue;

		m_contexts.erase(m_contexts.begin() + i);
		if ((int)i < m_current_context)
			m_current_context--;
		else if (i == m_current_context)
		{
			m_current_context--;
			// TODO context change
		}

		break;
	}

	delete pState;
}

void CContextManager::RegisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification, bool current_only, bool blockable)
{
	wxASSERT(pHandler);
	wxASSERT(notification != STATECHANGE_MAX && notification != STATECHANGE_NONE);

	std::list<t_handler> &handlers = m_handlers[notification];
	std::list<t_handler>::const_iterator iter;
	for (iter = handlers.begin(); iter != handlers.end(); iter++)
	{
		if (iter->pHandler == pHandler)
			return;
	}

	t_handler handler;
	handler.pHandler = pHandler;
	handler.blockable = blockable;
	handler.current_only = current_only;
	handlers.push_back(handler);
}

void CContextManager::UnregisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification)
{
	wxASSERT(pHandler);
	wxASSERT(notification != STATECHANGE_MAX);

	if (notification == STATECHANGE_NONE)
	{
		for (int i = 0; i < STATECHANGE_MAX; i++)
		{
			std::list<t_handler> &handlers = m_handlers[i];
			for (std::list<t_handler>::iterator iter = handlers.begin(); iter != handlers.end(); iter++)
			{
				if (iter->pHandler == pHandler)
				{
					handlers.erase(iter);
					break;
				}
			}
		}
	}
	else
	{
		std::list<t_handler> &handlers = m_handlers[notification];
		for (std::list<t_handler>::iterator iter = handlers.begin(); iter != handlers.end(); iter++)
		{
			if (iter->pHandler == pHandler)
			{
				handlers.erase(iter);
				return;
			}
		}
	}
}

void CContextManager::NotifyHandlers(CState* pState, t_statechange_notifications notification, const wxString& data, const void* data2, bool blocked)
{
	wxASSERT(notification != STATECHANGE_NONE && notification != STATECHANGE_MAX);

	const std::list<t_handler> &handlers = m_handlers[notification];
	for (std::list<t_handler>::const_iterator iter = handlers.begin(); iter != handlers.end(); iter++)
	{
		if (blocked && iter->blockable)
			continue;

		iter->pHandler->OnStateChange(pState, notification, data, data2);
	}
}

CState::CState(CMainFrame* pMainFrame)
{
	memset(m_blocked, 0, sizeof(m_blocked));

	m_pMainFrame = pMainFrame;

	m_pDirectoryListing = 0;
	m_pServer = 0;
	m_successful_connect = 0;

	m_pEngine = 0;
	m_pCommandQueue = 0;
	m_pComparisonManager = new CComparisonManager(this);

	m_pRecursiveOperation = new CRecursiveOperation(this);

	m_sync_browse.is_changing = false;
	m_sync_browse.compare = false;
}

CState::~CState()
{
	delete m_pServer;

	delete m_pComparisonManager;
	delete m_pCommandQueue;
	delete m_pEngine;

	// Unregister all handlers
	for (int i = 0; i < STATECHANGE_MAX; i++)
	{
		for (std::list<t_handler>::iterator iter = m_handlers[i].begin(); iter != m_handlers[i].end(); iter++)
		{
			iter->pHandler->m_pState = 0;
		}
	}

	delete m_pRecursiveOperation;
}

CLocalPath CState::GetLocalDir() const
{
	return m_localDir;
}

bool CState::SetLocalDir(const wxString& dir, wxString *error /*=0*/)
{
	if (m_sync_browse.is_changing)
	{
		wxMessageBox(_T("Cannot change directory, there already is a synchronized browsing operation in progress."), _("Synchronized browsing"));
		return false;
	}

	CLocalPath p(m_localDir);
#ifdef __WXMSW__
	if (dir == _T("..") && !p.HasParent() && p.HasLogicalParent())
	{
		// Parent of C:\ is drive list
		if (!p.MakeParent())
			return false;
	}
	else
#endif
	if (!p.ChangePath(dir))
		return false;

	if (!p.Exists(error))
		return false;

	if (!m_sync_browse.local_root.empty())
	{
		wxASSERT(m_pServer);
		
		if (p != m_sync_browse.local_root && !p.IsSubdirOf(m_sync_browse.local_root))
		{
			wxString msg = wxString::Format(_("The local directory '%s' is not below the synchronization root (%s).\nDisable synchronized browsing and continue changing the local directory?"),
					p.GetPath().c_str(),
					m_sync_browse.local_root.GetPath().c_str());
			if (wxMessageBox(msg, _("Synchronized browsing"), wxICON_QUESTION | wxYES_NO) != wxYES)
				return false;
			SetSyncBrowse(false);
		}
		else if (!IsRemoteIdle())
		{
			wxString msg(_("A remote operation is in progress and synchronized browsing is enabled.\nDisable synchronized browsing and continue changing the local directory?"));
			if (wxMessageBox(msg, _("Synchronized browsing"), wxICON_QUESTION | wxYES_NO) != wxYES)
				return false;
			SetSyncBrowse(false);
		}
		else
		{
			CServerPath remote_path = GetSynchronizedDirectory(p);
			if (remote_path.IsEmpty())
			{
				SetSyncBrowse(false);
				wxString msg = wxString::Format(_("Could not obtain corresponding remote directory for the local directory '%s'.\nSynchronized browsing has been disabled."),
					p.GetPath().c_str());
				wxMessageBox(msg, _("Synchronized browsing"));
				return false;
			}

			m_sync_browse.is_changing = true;
			m_sync_browse.compare = m_pComparisonManager->IsComparing();
			CListCommand *pCommand = new CListCommand(remote_path);
			m_pCommandQueue->ProcessCommand(pCommand);

			return true;
		}
	}
	
	m_localDir = p;

	COptions::Get()->SetOption(OPTION_LASTLOCALDIR, m_localDir.GetPath());

	NotifyHandlers(STATECHANGE_LOCAL_DIR);

	return true;
}

bool CState::SetRemoteDir(const CDirectoryListing *pDirectoryListing, bool modified /*=false*/)
{
	if (!pDirectoryListing)
	{
		SetSyncBrowse(false);
		if (modified)
			return false;

		if (m_pDirectoryListing)
		{
			m_pDirectoryListing = 0;
			NotifyHandlers(STATECHANGE_REMOTE_DIR);
		}
		return true;
	}

	wxASSERT(pDirectoryListing->m_firstListTime.IsValid());

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

	m_pDirectoryListing = pDirectoryListing;

	if (!modified)
		NotifyHandlers(STATECHANGE_REMOTE_DIR);
	else
		NotifyHandlers(STATECHANGE_REMOTE_DIR_MODIFIED);

	if (m_sync_browse.is_changing)
	{
		m_sync_browse.is_changing = false;
		if (m_pDirectoryListing->path != m_sync_browse.remote_root && !m_pDirectoryListing->path.IsSubdirOf(m_sync_browse.remote_root, false))
		{
			SetSyncBrowse(false);
			wxString msg = wxString::Format(_("Current remote directory (%s) is not below the synchronization root (%s).\nSynchronized browsing has been disabled."),
					m_pDirectoryListing->path.GetPath().c_str(),
					m_sync_browse.remote_root.GetPath().c_str());
			wxMessageBox(msg, _("Synchronized browsing"));
		}
		else
		{
			CLocalPath local_path = GetSynchronizedDirectory(m_pDirectoryListing->path);
			if (local_path.empty())
			{
				SetSyncBrowse(false);
				wxString msg = wxString::Format(_("Could not obtain corresponding local directory for the remote directory '%s'.\nSynchronized browsing has been disabled."),
					m_pDirectoryListing->path.GetPath().c_str());
				wxMessageBox(msg, _("Synchronized browsing"));
				return true;
			}

			wxString error;
			if (!local_path.Exists(&error))
			{
				SetSyncBrowse(false);
				wxString msg = error + _T("\n") + _("Synchronized browsing has been disabled.");
				wxMessageBox(msg, _("Synchronized browsing"));
				return true;
			}

			m_localDir = local_path;

			COptions::Get()->SetOption(OPTION_LASTLOCALDIR, m_localDir.GetPath());

			NotifyHandlers(STATECHANGE_LOCAL_DIR);

			if (m_sync_browse.compare)
				m_pComparisonManager->CompareListings();
		}
	}
	return true;
}

CSharedPointer<const CDirectoryListing> CState::GetRemoteDir() const
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
	wxString file_name;
	CLocalPath path(file, &file_name);
	if (path.empty())
		return;

	if (file_name.empty())
	{
		if (!path.HasParent())
			return;
		path.MakeParent(&file_name);
		wxASSERT(!file_name.empty());
	}

	if (path != m_localDir)
		return;

	NotifyHandlers(STATECHANGE_LOCAL_REFRESH_FILE, file_name);
}

void CState::LocalDirCreated(const CLocalPath& path)
{
	if (!path.IsSubdirOf(m_localDir))
		return;

	wxString next_segment = path.GetPath().Mid(m_localDir.GetPath().Len());
	int pos = next_segment.Find(CLocalPath::path_separator);
	if (pos <= 0)
	{
		// Shouldn't ever come true
		return;
	}

	// Current local path is /foo/
	// Called with /foo/bar/baz/
	// -> Refresh /foo/bar/
	next_segment = next_segment.Left(pos);
	NotifyHandlers(STATECHANGE_LOCAL_REFRESH_FILE, next_segment);
}

void CState::SetServer(const CServer* server)
{
	if (m_pServer)
	{
		if (server && *server == *m_pServer)
		{
			// Nothing changes
			return;
		}

		SetRemoteDir(0);
		delete m_pServer;
	}
	if (server)
		m_pServer = new CServer(*server);
	else
		m_pServer = 0;

	m_successful_connect = false;

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
	m_pCommandQueue->ProcessCommand(new CListCommand(path, _T(""), LIST_FLAG_FALLBACK_CURRENT));

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

void CState::RegisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification, bool blockable /*=true*/)
{
	wxASSERT(pHandler);
	wxASSERT(pHandler->m_pState == this);
	wxASSERT(notification != STATECHANGE_MAX && notification != STATECHANGE_NONE);

	std::list<t_handler> &handlers = m_handlers[notification];
	std::list<t_handler>::const_iterator iter;
	for (iter = handlers.begin(); iter != handlers.end(); iter++)
	{
		if (iter->pHandler == pHandler)
			return;
	}

	t_handler handler;
	handler.pHandler = pHandler;
	handler.blockable = blockable;
	handlers.push_back(handler);
}

void CState::UnregisterHandler(CStateEventHandler* pHandler, enum t_statechange_notifications notification)
{
	wxASSERT(pHandler);
	wxASSERT(notification != STATECHANGE_MAX);

	if (notification == STATECHANGE_NONE)
	{
		for (int i = 0; i < STATECHANGE_MAX; i++)
		{
			std::list<t_handler> &handlers = m_handlers[i];
			for (std::list<t_handler>::iterator iter = handlers.begin(); iter != handlers.end(); iter++)
			{
				if (iter->pHandler == pHandler)
				{
					handlers.erase(iter);
					break;
				}
			}
		}
	}
	else
	{
		std::list<t_handler> &handlers = m_handlers[notification];
		for (std::list<t_handler>::iterator iter = handlers.begin(); iter != handlers.end(); iter++)
		{
			if (iter->pHandler == pHandler)
			{
				handlers.erase(iter);
				return;
			}
		}
	}
}

void CState::BlockHandlers(enum t_statechange_notifications notification)
{
	wxASSERT(notification != STATECHANGE_MAX);

	if (notification == STATECHANGE_NONE)
	{
		for (int i = 0; i < STATECHANGE_MAX; i++)
			m_blocked[i] = true;
	}
	else
		m_blocked[notification] = true;
}

void CState::UnblockHandlers(enum t_statechange_notifications notification)
{
	wxASSERT(notification != STATECHANGE_MAX);

	if (notification == STATECHANGE_NONE)
	{
		for (int i = 0; i < STATECHANGE_MAX; i++)
			m_blocked[i] = false;
	}
	else
		m_blocked[notification] = false;

}

void CState::NotifyHandlers(enum t_statechange_notifications notification, const wxString& data /*=_T("")*/, const void* data2 /*=0*/)
{
	wxASSERT(notification != STATECHANGE_NONE && notification != STATECHANGE_MAX);

	const std::list<t_handler> &handlers = m_handlers[notification];
	for (std::list<t_handler>::const_iterator iter = handlers.begin(); iter != handlers.end(); iter++)
	{
		if (m_blocked[notification] && iter->blockable)
			continue;

		iter->pHandler->OnStateChange(this, notification, data, data2);
	}

	CContextManager::Get()->NotifyHandlers(this, notification, data, data2, m_blocked[notification]);
}

CStateEventHandler::CStateEventHandler(CState* pState)
{
	if (!pState)
		return;

	m_pState = pState;
}

CStateEventHandler::~CStateEventHandler()
{
	CContextManager::Get()->UnregisterHandler(this, STATECHANGE_NONE);

	const std::vector<CState*> *states = CContextManager::Get()->GetAllStates();
	for (std::vector<CState*>::const_iterator iter = states->begin(); iter != states->end(); iter++)
		(*iter)->UnregisterHandler(this, STATECHANGE_NONE);
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
			m_pMainFrame->GetQueue()->QueueFile_Finish(!queueOnly);
		}
		else if (wxDir::Exists(files[i]))
		{
			wxString dir = files[i];
			if (dir.Last() == CLocalPath::path_separator && dir.Len() > 1)
				dir.RemoveLast();
			int pos = dir.Find(CLocalPath::path_separator, true);
			if (pos != -1 && pos != (int)dir.Len() - 1)
			{
				wxString lastSegment = dir.Mid(pos + 1);
				CServerPath target = path;
				target.AddSegment(lastSegment);
				m_pMainFrame->GetQueue()->QueueFolder(queueOnly, false, dir, target, *m_pServer);
			}
		}
	}
}

void CState::HandleDroppedFiles(const wxFileDataObject* pFileDataObject, const CLocalPath& path, bool copy)
{
	const wxArrayString &files = pFileDataObject->GetFilenames();
	if (!files.Count())
		return;

#ifdef __WXMSW__
	int len = 1;

	for (unsigned int i = 0; i < files.Count(); i++)
		len += files[i].Len() + 1;

	// SHFILEOPSTRUCT's pTo and pFrom accept null-terminated lists
	// of null-terminated filenames.
	wxChar* from = new wxChar[len];
	wxChar* p = from;
	for (unsigned int i = 0; i < files.Count(); i++)
	{
		wxStrcpy(p, files[i]);
		p += files[i].Len() + 1;
	}
	*p = 0; // End of list

	wxChar* to = new wxChar[path.GetPath().Len() + 2];
	wxStrcpy(to, path.GetPath());
	to[path.GetPath().Len() + 1] = 0; // End of list

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
			int pos = file.Find(CLocalPath::path_separator, true);
			if (pos == -1 || pos == (int)file.Len() - 1)
				continue;
			const wxString& name = file.Mid(pos + 1);
			if (copy)
				wxCopyFile(file, path.GetPath() + name);
			else
				wxRenameFile(file, path.GetPath() + name);
		}
		else if (wxDir::Exists(file))
		{
			if (copy)
				RecursiveCopy(file, path.GetPath());
			else
			{
				int pos = file.Find(CLocalPath::path_separator, true);
				if (pos == -1 || pos == (int)file.Len() - 1)
					continue;
				const wxString& name = file.Mid(pos + 1);
				wxRenameFile(file, path.GetPath() + name);
			}
		}
	}
#endif

	RefreshLocal();
}

bool CState::RecursiveCopy(CLocalPath source, const CLocalPath& target)
{
	if (source.empty() || target.empty())
		return false;

	if (source == target)
		return false;

	if (source.IsParentOf(target))
		return false;

	if (!source.HasParent())
		return false;

	wxString last_segment;
	if (!source.MakeParent(&last_segment))
		return false;

	std::list<wxString> dirsToVisit;
	dirsToVisit.push_back(last_segment + CLocalPath::path_separator);

	// Process any subdirs which still have to be visited
	while (!dirsToVisit.empty())
	{
		wxString dirname = dirsToVisit.front();
		dirsToVisit.pop_front();
		wxMkdir(target.GetPath() + dirname);

		CLocalFileSystem fs;
		if (!fs.BeginFindFiles(source.GetPath() + dirname, false))
			continue;

		bool is_dir, is_link;
		wxString file;
		while (fs.GetNextFile(file, is_link, is_dir, 0, 0, 0))
		{
			if (file == _T(""))
			{
				wxGetApp().DisplayEncodingWarning();
				continue;
			}

			if (is_dir)
			{
				if (is_link)
					continue;

				const wxString subDir = dirname + file + CLocalPath::path_separator;
				dirsToVisit.push_back(subDir);
			}
			else
				wxCopyFile(source.GetPath() + dirname + file, target.GetPath() + dirname + file);
		}
	}

	return true;
}

bool CState::DownloadDroppedFiles(const CRemoteDataObject* pRemoteDataObject, const CLocalPath& path, bool queueOnly /*=false*/)
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
		if (!IsRemoteConnected() || !IsRemoteIdle())
			return false;
	}

	if (hasFiles)
		m_pMainFrame->GetQueue()->QueueFiles(queueOnly, path.GetPath(), *pRemoteDataObject);

	if (!hasDirs)
		return true;

	for (std::list<CRemoteDataObject::t_fileInfo>::const_iterator iter = files.begin(); iter != files.end(); iter++)
	{
		if (!iter->dir)
			continue;

		m_pRecursiveOperation->AddDirectoryToVisit(pRemoteDataObject->GetServerPath(), iter->name, path.GetPath() + iter->name, iter->link);
	}

	if (m_pComparisonManager->IsComparing())
		m_pComparisonManager->ExitComparisonMode();

	CFilterManager filter;
	m_pRecursiveOperation->StartRecursiveOperation(queueOnly ? CRecursiveOperation::recursive_addtoqueue : CRecursiveOperation::recursive_download, pRemoteDataObject->GetServerPath(), filter.GetActiveFilters(false));

	return true;
}

bool CState::IsRemoteConnected() const
{
	if (!m_pEngine)
		return false;

	return m_pServer != 0;
}

bool CState::IsRemoteIdle() const
{
	if (m_pRecursiveOperation->GetOperationMode() != CRecursiveOperation::recursive_none)
		return false;

	if (!m_pCommandQueue)
		return true;

	return m_pCommandQueue->Idle();
}

wxString CState::GetAsURL(const wxString& dir)
{
	// Cheap URL encode
	wxString encoded;
	const wxWX2MBbuf utf8 = dir.mb_str();

	const char* p = utf8;
	while (*p)
	{
		// List of characters that don't need to be escaped taken
		// from the BNF grammar in RFC 1738
		// Again attention seeking Windows wants special treatment...
		const unsigned char c = (unsigned char)*p++;
		if ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			c == '$' ||
			c == '_' ||
			c == '-' ||
			c == '.' ||
			c == '+' ||
			c == '!' ||
			c == '*' ||
#ifndef __WXMSW__
			c == '\'' ||
#endif
			c == '(' ||
			c == ')' ||
			c == ',' ||
			c == '?' ||
			c == ':' ||
			c == '@' ||
			c == '&' ||
			c == '=' ||
			c == '/')
		{
			encoded += (wxChar)c;
		}
#ifdef __WXMSW__
		else if (c == '\\')
			encoded += '/';
#endif
		else
			encoded += wxString::Format(_T("%%%x"), (unsigned int)c);
	}
#ifdef __WXMSW__
	if (encoded.Left(2) == _T("//"))
	{
		// UNC path
		encoded = encoded.Mid(2);
	}
	else
		encoded = _T("/") + encoded;
#endif

	return _T("file://") + encoded;
}

void CState::ListingFailed(int error)
{
	m_sync_browse.is_changing = false;

	// Let the recursive operation handler know if a LIST command failed,
	// so that it may issue the next command in recursive operations.
	m_pRecursiveOperation->ListingFailed(error);
}

void CState::LinkIsNotDir(const CServerPath& path, const wxString& subdir)
{
	m_sync_browse.is_changing = false;

	NotifyHandlers(STATECHANGE_REMOTE_LINKNOTDIR, subdir, &path);
}

bool CState::ChangeRemoteDir(const CServerPath& path, const wxString& subdir /*=_T("")*/, int flags /*=0*/, bool ignore_busy /*=false*/)
{
	if (!m_pServer || !m_pCommandQueue)
		return false;

	if (!m_sync_browse.local_root.empty())
	{
		CServerPath p(path);
		if (!subdir.empty() && !p.ChangePath(subdir))
		{
			wxString msg = wxString::Format(_("Could not get full remote path."));
			wxMessageBox(msg, _("Synchronized browsing"));
			return false;
		}

		if (p != m_sync_browse.remote_root && !p.IsSubdirOf(m_sync_browse.remote_root, false))
		{
			wxString msg = wxString::Format(_("The remote directory '%s' is not below the synchronization root (%s).\nDisable synchronized browsing and continue changing the remote directory?"),
					p.GetPath().c_str(),
					m_sync_browse.remote_root.GetPath().c_str());
			if (wxMessageBox(msg, _("Synchronized browsing"), wxICON_QUESTION | wxYES_NO) != wxYES)
				return false;
			SetSyncBrowse(false);
		}
		else if (!IsRemoteIdle() && !ignore_busy)
		{
			wxString msg(_("Another remote operation is already in progress, cannot change directory now."));
			wxMessageBox(msg, _("Synchronized browsing"), wxICON_EXCLAMATION);
			return false;
		}
		else
		{
			wxString error;
			CLocalPath local_path = GetSynchronizedDirectory(p);
			if (local_path.empty())
			{
				wxString msg = wxString::Format(_("Could not obtain corresponding local directory for the remote directory '%s'.\nDisable synchronized browsing and continue changing the remote directory?"),
					p.GetPath().c_str());
				if (wxMessageBox(msg, _("Synchronized browsing"), wxICON_QUESTION | wxYES_NO) != wxYES)
					return false;
				SetSyncBrowse(false);
			}
			else if (!local_path.Exists(&error))
			{
				wxString msg = error + _T("\n") + _("Disable synchronized browsing and continue changing the remote directory?");
				if (wxMessageBox(msg, _("Synchronized browsing"), wxICON_QUESTION | wxYES_NO) != wxYES)
					return false;
				SetSyncBrowse(false);
			}
			else
			{
				m_sync_browse.is_changing = true;
				m_sync_browse.compare = m_pComparisonManager->IsComparing();
			}
		}
	}

	CListCommand *pCommand = new CListCommand(path, subdir, flags);
	m_pCommandQueue->ProcessCommand(pCommand);

	return true;
}

bool CState::SetSyncBrowse(bool enable, const CServerPath& assumed_remote_root /*=CServerPath()*/)
{
	if (enable != m_sync_browse.local_root.empty())
		return enable;

	if (!enable)
	{
		wxASSERT(assumed_remote_root.IsEmpty());
		m_sync_browse.local_root.clear();
		m_sync_browse.remote_root.Clear();
		m_sync_browse.is_changing = false;

		NotifyHandlers(STATECHANGE_SYNC_BROWSE);
		return false;
	}

	if (!m_pDirectoryListing && assumed_remote_root.IsEmpty())
		return false;

	m_sync_browse.is_changing = false;
	m_sync_browse.local_root = m_localDir;

	if (assumed_remote_root.IsEmpty())
		m_sync_browse.remote_root = m_pDirectoryListing->path;
	else
	{
		m_sync_browse.remote_root = assumed_remote_root;
		m_sync_browse.is_changing = true;
		m_sync_browse.compare = false;
	}

	while (m_sync_browse.local_root.HasParent() && m_sync_browse.remote_root.HasParent() &&
		m_sync_browse.local_root.GetLastSegment() == m_sync_browse.remote_root.GetLastSegment())
	{
		m_sync_browse.local_root.MakeParent();
		m_sync_browse.remote_root = m_sync_browse.remote_root.GetParent();
	}

	NotifyHandlers(STATECHANGE_SYNC_BROWSE);
	return true;
}

CLocalPath CState::GetSynchronizedDirectory(CServerPath remote_path)
{
	std::list<wxString> segments;
	while (remote_path.HasParent() && remote_path != m_sync_browse.remote_root)
	{
		segments.push_front(remote_path.GetLastSegment());
		remote_path = remote_path.GetParent();
	}
	if (remote_path != m_sync_browse.remote_root)
		return CLocalPath();

	CLocalPath local_path = m_sync_browse.local_root;
	for (std::list<wxString>::const_iterator iter = segments.begin(); iter != segments.end(); iter++)
		local_path.AddSegment(*iter);

	return local_path;
}


CServerPath CState::GetSynchronizedDirectory(CLocalPath local_path)
{
	std::list<wxString> segments;
	while (local_path.HasParent() && local_path != m_sync_browse.local_root)
	{
		wxString last;
		local_path.MakeParent(&last);
		segments.push_front(last);
	}
	if (local_path != m_sync_browse.local_root)
		return CServerPath();

	CServerPath remote_path = m_sync_browse.remote_root;
	for (std::list<wxString>::const_iterator iter = segments.begin(); iter != segments.end(); iter++)
		remote_path.AddSegment(*iter);

	return remote_path;
}
