#include "FileZilla.h"
#include "recursive_operation.h"
#include "commandqueue.h"
#include "chmoddialog.h"
#include "filter.h"
#include "queue.h"
#include "local_filesys.h"

CRecursiveOperation::CNewDir::CNewDir()
{
	recurse = true;
	second_try = false;
	link = false;
	doVisit = true;
}

CRecursiveOperation::CRecursiveOperation(CState* pState)
	: CStateEventHandler(pState),
	  m_operationMode(recursive_none), m_pState(pState)
{
	pState->RegisterHandler(this, STATECHANGE_REMOTE_DIR, false);

	m_pChmodDlg = 0;
	m_pQueue = 0;
}

CRecursiveOperation::~CRecursiveOperation()
{
	if (m_pChmodDlg)
	{
		m_pChmodDlg->Destroy();
		m_pChmodDlg = 0;
	}
}

void CRecursiveOperation::OnStateChange(enum t_statechange_notifications notification, const wxString& data)
{
	wxASSERT(m_pState);
	wxASSERT(notification == STATECHANGE_REMOTE_DIR);
	ProcessDirectoryListing(m_pState->GetRemoteDir().Value());
}

void CRecursiveOperation::StartRecursiveOperation(enum OperationMode mode, const CServerPath& startDir, const std::list<CFilter>& filters, bool allowParent /*=false*/, const CServerPath& finalDir /*=CServerPath()*/)
{
	wxCHECK_RET(m_operationMode == recursive_none, _T("StartRecursiveOperation called with m_operationMode != recursive_none"));
	wxCHECK_RET(m_pState->IsRemoteConnected(), _T("StartRecursiveOperation while disconnected"));
	wxCHECK_RET(!startDir.IsEmpty(), _T("Empty startDir in StartRecursiveOperation"));

	if (mode == recursive_chmod && !m_pChmodDlg)
		return;

	if ((mode == recursive_download || mode == recursive_addtoqueue || recursive_download_flatten || mode == recursive_addtoqueue_flatten) && !m_pQueue)
		return;

	if (m_dirsToVisit.empty())
	{
		// Nothing to do in this case
		return;
	}

	m_operationMode = mode;
	m_pState->NotifyHandlers(STATECHANGE_REMOTE_IDLE);

	m_startDir = startDir;

	if (finalDir.IsEmpty())
		m_finalDir = startDir;
	else
		m_finalDir = finalDir;

	m_allowParent = allowParent;

	m_filters = filters;

	NextOperation();
}

void CRecursiveOperation::AddDirectoryToVisit(const CServerPath& path, const wxString& subdir, const wxString& localDir /*=_T("")*/, bool is_link /*=false*/)
{
	CNewDir dirToVisit;

	dirToVisit.localDir = localDir;
	if (localDir != _T("") && localDir.Last() != CLocalFileSystem::path_separator)
		dirToVisit.localDir += CLocalFileSystem::path_separator;
	dirToVisit.parent = path;
	dirToVisit.subdir = subdir;
	dirToVisit.link = is_link;
	m_dirsToVisit.push_back(dirToVisit);
}

void CRecursiveOperation::AddDirectoryToVisitRestricted(const CServerPath& path, const wxString& restrict, bool recurse)
{
	CNewDir dirToVisit;
	dirToVisit.parent = path;
	dirToVisit.recurse = recurse;
	dirToVisit.restrict = restrict;
	m_dirsToVisit.push_back(dirToVisit);
}

bool CRecursiveOperation::NextOperation()
{
	if (m_operationMode == recursive_none)
		return false;

	while (!m_dirsToVisit.empty())
	{
		const CNewDir& dirToVisit = m_dirsToVisit.front();
		if (m_operationMode == recursive_delete && !dirToVisit.doVisit)
		{
			m_pState->m_pCommandQueue->ProcessCommand(new CRemoveDirCommand(dirToVisit.parent, dirToVisit.subdir));
			m_dirsToVisit.pop_front();
			continue;
		}

		CListCommand* cmd = new CListCommand(dirToVisit.parent, dirToVisit.subdir, dirToVisit.link ? LIST_FLAG_LINK : 0);
		m_pState->m_pCommandQueue->ProcessCommand(cmd);
		return true;
	}

	StopRecursiveOperation();
	m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(m_finalDir));
	return false;
}

void CRecursiveOperation::ProcessDirectoryListing(const CDirectoryListing* pDirectoryListing)
{
	if (!pDirectoryListing)
	{
		StopRecursiveOperation();
		return;
	}

	if (m_operationMode == recursive_none)
		return;

	if (pDirectoryListing->m_failed)
	{
		// Ignore this.
		// It will get handled by the failed command in ListingFailed
		return;
	}

	wxASSERT(!m_dirsToVisit.empty());

	if (!m_pState->IsRemoteConnected() || m_dirsToVisit.empty())
	{
		StopRecursiveOperation();
		return;
	}

	CNewDir dir = m_dirsToVisit.front();
	m_dirsToVisit.pop_front();

	if (!pDirectoryListing->path.IsSubdirOf(m_startDir, false))
	{
		// In some cases (chmod from tree for example) it is neccessary to list the
		// parent first
		if (pDirectoryListing->path != m_startDir || !m_allowParent)
		{
			NextOperation();
			return;
		}
	}

	if (m_operationMode == recursive_delete && dir.doVisit && dir.subdir != _T(""))
	{
		// After recursing into directory to delete its contents, delete directory itself
		// Gets handled in NextOperation
		CNewDir dir2 = dir;
		dir2.doVisit = false;
		m_dirsToVisit.push_front(dir2);
	}

	if (dir.link && !dir.recurse)
	{
		NextOperation();
		return;
	}

	// Check if we have already visited the directory
	if (!m_visitedDirs.insert(pDirectoryListing->path).second)
	{
		NextOperation();
		return;
	}

	const CServer* pServer = m_pState->GetServer();
	wxASSERT(pServer);

	if (!pDirectoryListing->GetCount())
	{
		if (m_operationMode == recursive_download)
		{
			wxFileName fn(dir.localDir, _T(""));
			wxFileName::Mkdir(fn.GetPath(), 0777, wxPATH_MKDIR_FULL);
			m_pState->RefreshLocalFile(fn.GetFullPath());
		}
		else if (m_operationMode == recursive_addtoqueue)
		{
			m_pQueue->QueueFile(true, true, dir.localDir, _T(""), CServerPath(), *pServer, -1);
			m_pQueue->QueueFile_Finish(false);
		}
	}

	CFilterManager filter;

	// Is operation restricted to a single child?
	bool restrict = !dir.restrict.IsEmpty();

	std::list<wxString> filesToDelete;

	const wxString path = pDirectoryListing->path.GetPath();
	
	bool added = false;

	for (int i = pDirectoryListing->GetCount() - 1; i >= 0; i--)
	{
		const CDirentry& entry = (*pDirectoryListing)[i];

		if (restrict)
		{
			if (entry.name != dir.restrict)
				continue;
		}
		else if (filter.FilenameFiltered(m_filters, entry.name, path, entry.dir, entry.size, false, 0))
			continue;

		if (entry.dir && (!entry.link || m_operationMode != recursive_delete))
		{
			if (dir.recurse)
			{
				CNewDir dirToVisit;
				dirToVisit.parent = pDirectoryListing->path;
				dirToVisit.subdir = entry.name;
				dirToVisit.localDir = dir.localDir;
				
				if (m_operationMode != recursive_addtoqueue_flatten && m_operationMode != recursive_download_flatten)
				{
					dirToVisit.localDir += entry.name + CLocalFileSystem::path_separator;
				}
				if (entry.link)
				{
					dirToVisit.link = true;
					dirToVisit.recurse = false;
				}
				m_dirsToVisit.push_front(dirToVisit);
			}
		}
		else
		{
			switch (m_operationMode)
			{
			case recursive_addtoqueue:
			case recursive_download:
			case recursive_addtoqueue_flatten:
			case recursive_download_flatten:
				{
					m_pQueue->QueueFile(m_operationMode == recursive_addtoqueue, true,
						dir.localDir + entry.name,
						entry.name, pDirectoryListing->path, *pServer, entry.size);
					added = true;
				}
				break;
			case recursive_delete:
				filesToDelete.push_back(entry.name);
				break;
			default:
				break;
			}
		}

		if (m_operationMode == recursive_chmod && m_pChmodDlg)
		{
			const int applyType = m_pChmodDlg->GetApplyType();
			if (!applyType ||
				(!entry.dir && applyType == 1) ||
				(entry.dir && applyType == 2))
			{
				char permissions[9];
				bool res = m_pChmodDlg->ConvertPermissions(entry.permissions, permissions);
				wxString newPerms = m_pChmodDlg->GetPermissions(res ? permissions : 0, entry.dir);
				m_pState->m_pCommandQueue->ProcessCommand(new CChmodCommand(pDirectoryListing->path, entry.name, newPerms));
			}
		}
	}
	if (added)
		m_pQueue->QueueFile_Finish(m_operationMode != recursive_addtoqueue && m_operationMode != recursive_addtoqueue_flatten);

	if (m_operationMode == recursive_delete && !filesToDelete.empty())
		m_pState->m_pCommandQueue->ProcessCommand(new CDeleteCommand(pDirectoryListing->path, filesToDelete));

	NextOperation();
}

void CRecursiveOperation::SetChmodDialog(CChmodDialog* pChmodDialog)
{
	wxASSERT(pChmodDialog);

	if (m_pChmodDlg)
		m_pChmodDlg->Destroy();

	m_pChmodDlg = pChmodDialog;
}

void CRecursiveOperation::StopRecursiveOperation()
{
	m_operationMode = recursive_none;
	m_pState->NotifyHandlers(STATECHANGE_REMOTE_IDLE);
	m_dirsToVisit.clear();
	m_visitedDirs.clear();

	if (m_pChmodDlg)
	{
		m_pChmodDlg->Destroy();
		m_pChmodDlg = 0;
	}
}

void CRecursiveOperation::ListingFailed(int error)
{
	if (m_operationMode == recursive_none)
		return;

	wxASSERT(!m_dirsToVisit.empty());
	if (m_dirsToVisit.empty())
		return;

	CNewDir dir = m_dirsToVisit.front();
	m_dirsToVisit.pop_front();
	if ((error & FZ_REPLY_CRITICALERROR) != FZ_REPLY_CRITICALERROR && !dir.second_try)
	{
		// Retry, could have been a temporary socket creating failure
		// (e.g. hitting a blocked port) or a disconnect (e.g. no-filetransfer-timeout)
		dir.second_try = true;
		m_dirsToVisit.push_front(dir);
	}

	NextOperation();
}

void CRecursiveOperation::SetQueue(CQueueView* pQueue)
{
	m_pQueue = pQueue;
}

bool CRecursiveOperation::ChangeOperationMode(enum OperationMode mode)
{
	if (mode != recursive_addtoqueue && m_operationMode != recursive_download && mode != recursive_addtoqueue_flatten && m_operationMode != recursive_download_flatten)
		return false;

	m_operationMode = mode;

	return true;
}

void CRecursiveOperation::LinkIsNotDir()
{
	if (m_operationMode == recursive_none)
		return;

	wxASSERT(!m_dirsToVisit.empty());
	if (m_dirsToVisit.empty())
		return;

	CNewDir dir = m_dirsToVisit.front();
	m_dirsToVisit.pop_front();

	const CServer* pServer = m_pState->GetServer();
	if (!pServer)
	{
		NextOperation();
		return;
	}

	wxString local_file = dir.localDir;
	if (m_operationMode == recursive_addtoqueue_flatten || m_operationMode == recursive_download_flatten)
	{
		if (local_file.Last() != CLocalFileSystem::path_separator)
			local_file += CLocalFileSystem::path_separator;
		local_file += dir.subdir;
	}
	else
	{
		if (local_file.Last() == CLocalFileSystem::path_separator)
			local_file.RemoveLast();
	}
	m_pQueue->QueueFile(m_operationMode == recursive_addtoqueue || m_operationMode == recursive_addtoqueue_flatten, true, local_file, dir.subdir, dir.parent, *pServer, -1);
	m_pQueue->QueueFile_Finish(m_operationMode != recursive_addtoqueue);

	NextOperation();
}
