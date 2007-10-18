#include "FileZilla.h"
#include "recursive_operation.h"
#include "commandqueue.h"
#include "chmoddialog.h"
#include "filter.h"
#include "queue.h"

CRecursiveOperation::CNewDir::CNewDir()
{
	recurse = true;
}

CRecursiveOperation::CRecursiveOperation(CState* pState)
	: CStateEventHandler(pState, STATECHANGE_REMOTE_DIR),
	  m_operationMode(recursive_none), m_pState(pState)
{
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

void CRecursiveOperation::OnStateChange(unsigned int event, const wxString& data)
{
	wxASSERT(m_pState);
	if (event == STATECHANGE_REMOTE_DIR)
		ProcessDirectoryListing(m_pState->GetRemoteDir());
}

void CRecursiveOperation::StartRecursiveOperation(enum OperationMode mode, const CServerPath& startDir, bool allowParent /*=false*/, const CServerPath& finalDir /*=CServerPath()*/)
{
	wxCHECK_RET(m_operationMode == recursive_none, _T("StartRecursiveOperation called with m_operationMode != recursive_none"));
	wxCHECK_RET(m_pState->IsRemoteConnected(), _T("StartRecursiveOperation while disconnected"));
	wxCHECK_RET(!startDir.IsEmpty(), _T("Empty startDir in StartRecursiveOperation"));

	if (mode == recursive_chmod && !m_pChmodDlg)
		return;

	if ((mode == recursive_download || mode == recursive_addtoqueue) && !m_pQueue)
		return;

	if (m_dirsToVisit.empty())
	{
		// Nothing to do in this case
		return;
	}

	m_operationMode = mode;

	m_startDir = startDir;

	if (finalDir.IsEmpty())
		m_finalDir = startDir;
	else
		m_finalDir = finalDir;

	m_allowParent = allowParent;

	NextOperation();
}

void CRecursiveOperation::AddDirectoryToVisit(const CServerPath& path, const wxString& subdir, const wxString& localDir /*=_T("")*/)
{
	CNewDir dirToVisit;
	dirToVisit.doVisit = true;
	dirToVisit.localDir = localDir;
	dirToVisit.parent = path;
	dirToVisit.subdir = subdir;
	m_dirsToVisit.push_back(dirToVisit);
}

void CRecursiveOperation::AddDirectoryToVisitRestricted(const CServerPath& path, const wxString& restrict, bool recurse)
{
	CNewDir dirToVisit;
	dirToVisit.doVisit = true;
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

		m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(dirToVisit.parent, dirToVisit.subdir));
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

	// Check if we have already visited the directory
	for (std::list<CServerPath>::const_iterator iter = m_visitedDirs.begin(); iter != m_visitedDirs.end(); iter++)
	{
		if (*iter == pDirectoryListing->path)
		{
			NextOperation();
			return;
		}
	}
	m_visitedDirs.push_back(pDirectoryListing->path);

	const CServer* pServer = m_pState->GetServer();
	wxASSERT(pServer);

	if (!pDirectoryListing->GetCount())
	{
		wxFileName fn(dir.localDir, _T(""));
		if (m_operationMode == recursive_download)
		{
			wxFileName::Mkdir(fn.GetPath(), 0777, wxPATH_MKDIR_FULL);
			m_pState->RefreshLocalFile(fn.GetFullPath());
		}
		else if (m_operationMode == recursive_addtoqueue)
			m_pQueue->QueueFile(true, true, fn.GetFullPath(), _T(""), CServerPath(), *pServer, -1);
	}

	CFilterDialog filter;

	// Is operation restricted to a single child?
	bool restrict = !dir.restrict.IsEmpty();

	std::list<wxString> filesToDelete;
	
	for (int i = pDirectoryListing->GetCount() - 1; i >= 0; i--)
	{
		const CDirentry& entry = (*pDirectoryListing)[i];

		if (restrict)
		{
			if (entry.name != dir.restrict)
				continue;
		}
		else if (filter.FilenameFiltered(entry.name, entry.dir, entry.size, false))
			continue;

		if (entry.dir && !entry.link)
		{
			if (dir.recurse)
			{
				CNewDir dirToVisit;
				wxFileName fn(dir.localDir, _T(""));
				fn.AppendDir(entry.name);
				dirToVisit.parent = pDirectoryListing->path;
				dirToVisit.subdir = entry.name;
				dirToVisit.localDir = fn.GetFullPath();
				dirToVisit.doVisit = true;
				m_dirsToVisit.push_front(dirToVisit);
			}
		}
		else
		{
			switch (m_operationMode)
			{
			case recursive_addtoqueue:
			case recursive_download:
				{
					wxFileName fn(dir.localDir, entry.name);
					m_pQueue->QueueFile(m_operationMode == recursive_addtoqueue, true, fn.GetFullPath(), entry.name, pDirectoryListing->path, *pServer, entry.size);
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
				wxString newPerms = m_pChmodDlg->GetPermissions(res ? permissions : 0);
				m_pState->m_pCommandQueue->ProcessCommand(new CChmodCommand(pDirectoryListing->path, entry.name, newPerms));
			}
		}
	}

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
	m_dirsToVisit.clear();
	m_visitedDirs.clear();

	if (m_pChmodDlg)
	{
		m_pChmodDlg->Destroy();
		m_pChmodDlg = 0;
	}
}

void CRecursiveOperation::ListingFailed()
{
	if (m_operationMode == recursive_none)
		return;

	wxASSERT(!m_dirsToVisit.empty());
	if (m_dirsToVisit.empty())
		return;

	m_dirsToVisit.pop_front();

	NextOperation();
}

void CRecursiveOperation::SetQueue(CQueueView* pQueue)
{
	m_pQueue = pQueue;
}
