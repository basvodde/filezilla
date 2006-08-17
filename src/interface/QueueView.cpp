#include "FileZilla.h"
#include "QueueView.h"
#include "Mainfrm.h"
#include "Options.h"
#include "StatusView.h"
#include "statuslinectrl.h"
#include "../tinyxml/tinyxml.h"
#include "xmlfunctions.h"
#include "filezillaapp.h"
#include "ipcmutex.h"
#include "state.h"
#include "asyncrequestqueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DECLARE_EVENT_TYPE(fzEVT_FOLDERTHREAD_COMPLETE, -1)
DEFINE_EVENT_TYPE(fzEVT_FOLDERTHREAD_COMPLETE)

DECLARE_EVENT_TYPE(fzEVT_FOLDERTHREAD_FILES, -1)
DEFINE_EVENT_TYPE(fzEVT_FOLDERTHREAD_FILES)

DECLARE_EVENT_TYPE(fzEVT_UPDATE_STATUSLINES, -1)
DEFINE_EVENT_TYPE(fzEVT_UPDATE_STATUSLINES)

BEGIN_EVENT_TABLE(CQueueView, wxListCtrl)
EVT_FZ_NOTIFICATION(wxID_ANY, CQueueView::OnEngineEvent)
EVT_COMMAND(wxID_ANY, fzEVT_FOLDERTHREAD_COMPLETE, CQueueView::OnFolderThreadComplete)
EVT_COMMAND(wxID_ANY, fzEVT_FOLDERTHREAD_FILES, CQueueView::OnFolderThreadFiles)
EVT_SCROLLWIN(CQueueView::OnScrollEvent)
EVT_COMMAND(wxID_ANY, fzEVT_UPDATE_STATUSLINES, CQueueView::OnUpdateStatusLines)
EVT_MOUSEWHEEL(CQueueView::OnMouseWheel)

EVT_CONTEXT_MENU(CQueueView::OnContextMenu)
EVT_MENU(XRCID("ID_PROCESSQUEUE"), CQueueView::OnProcessQueue)
EVT_MENU(XRCID("ID_REMOVEALL"), CQueueView::OnStopAndClear)
EVT_MENU(XRCID("ID_REMOVE"), CQueueView::OnRemoveSelected)

EVT_ERASE_BACKGROUND(CQueueView::OnEraseBackground)

END_EVENT_TABLE()

class CFolderItem;
class CFolderProcessingThread : public wxThread
{
public:
	CFolderProcessingThread(CQueueView* pOwner, CFolderItem* pFolderItem) : wxThread(wxTHREAD_JOINABLE) { 
		m_pOwner = pOwner;
		m_pFolderItem = pFolderItem;
	}
	~CFolderProcessingThread() { }

	void GetFiles(std::list<t_newEntry> &entryList)
	{
		wxASSERT(entryList.empty());
		wxCriticalSectionLocker locker(m_sync);
		entryList.swap(m_entryList);
	}

protected:

	ExitCode Entry()
	{
		wxMutexGuiEnter();
		wxASSERT(m_pFolderItem->GetTopLevelItem() && m_pFolderItem->GetTopLevelItem()->GetType() == QueueItemType_Server);
		wxMutexGuiLeave();

		wxASSERT(!m_pFolderItem->Download());

		while (!TestDestroy())
		{
			if (m_pFolderItem->m_remove)
			{
				wxCommandEvent evt(fzEVT_FOLDERTHREAD_COMPLETE, wxID_ANY);
				wxPostEvent(m_pOwner, evt);
				return 0;
			}
			bool found;
			wxString file;
			if (!m_pFolderItem->m_pDir)
			{
				if (!m_pFolderItem->m_dirsToCheck.empty())
				{
					const CFolderItem::t_dirPair& pair = m_pFolderItem->m_dirsToCheck.front();

					wxLogNull nullLog;

					m_pFolderItem->m_pDir = new wxDir(pair.localPath);

					if (m_pFolderItem->m_pDir->IsOpened())
					{
						m_pFolderItem->m_currentLocalPath = pair.localPath;
						m_pFolderItem->m_currentRemotePath = pair.remotePath;

						found = m_pFolderItem->m_pDir->GetFirst(&file);
					}
					else
						found = false;
					m_pFolderItem->m_dirsToCheck.pop_front();
				}
				else
				{
					wxCommandEvent evt(fzEVT_FOLDERTHREAD_COMPLETE, wxID_ANY);
					wxPostEvent(m_pOwner, evt);
					return 0;
				}
			}
			else
				found = m_pFolderItem->m_pDir->GetNext(&file);

			while (found && !TestDestroy() && !m_pFolderItem->m_remove)
			{
				const wxString& fullName = m_pFolderItem->m_currentLocalPath + wxFileName::GetPathSeparator() + file;

				wxStructStat buf;
				int result;
				result = wxStat(fullName, &buf);

				if (wxDir::Exists(fullName))
				{	
					CFolderItem::t_dirPair pair;
					pair.localPath = fullName;
					pair.remotePath = m_pFolderItem->m_currentRemotePath;
					pair.remotePath.AddSegment(file);
					m_pFolderItem->m_dirsToCheck.push_back(pair);
				}
				else
				{
					t_newEntry entry;
					entry.localFile = fullName;
					entry.remoteFile = file;
					entry.remotePath = m_pFolderItem->m_currentRemotePath;
					entry.size = result ? -1 : buf.st_size;

					bool send = false;
					m_sync.Enter();;
					if (!m_entryList.size())
						send = true;
					m_entryList.push_back(entry);
					m_sync.Leave();
					
					if (send)
					{
						wxCommandEvent evt(fzEVT_FOLDERTHREAD_FILES, wxID_ANY);
						wxPostEvent(m_pOwner, evt);
					}
				}

				found = m_pFolderItem->m_pDir->GetNext(&file);
			}
			delete m_pFolderItem->m_pDir;
			m_pFolderItem->m_pDir = 0;
		}

		return 0;
	}

	// Access has to be guarded by m_sync
	std::list<t_newEntry> m_entryList;

	CQueueView* m_pOwner;
	CFolderItem* m_pFolderItem;

	wxCriticalSection m_sync;
};

CQueueItem::CQueueItem()
{
	m_visibleOffspring = 0;
	m_parent = 0;
}

CQueueItem::~CQueueItem()
{
	std::vector<CQueueItem*>::iterator iter;
	for (iter = m_children.begin(); iter != m_children.end(); iter++)
		delete *iter;
}

void CQueueItem::SetPriority(enum QueuePriority priority)
{
	std::vector<CQueueItem*>::iterator iter;
	for (iter = m_children.begin(); iter != m_children.end(); iter++)
		(*iter)->SetPriority(priority);
}

void CQueueItem::AddChild(CQueueItem* item)
{
	item->m_indent = m_indent + _T("  ");
	m_children.push_back(item);

	m_visibleOffspring += 1 + item->m_visibleOffspring;
	CQueueItem* parent = GetParent();
	while (parent)
	{
		parent->m_visibleOffspring += 1 + item->m_visibleOffspring;
		parent = parent->GetParent();
	}
}

CQueueItem* CQueueItem::GetChild(unsigned int item, bool recursive /*=true*/)
{
	std::vector<CQueueItem*>::iterator iter;
	if (!recursive)
	{
		if (item >= m_children.size())
			return 0;
		iter = m_children.begin();
		iter += item;
		return *iter;
	}
	for (iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if (!item)
			return *iter;
		
		unsigned int count = (*iter)->GetChildrenCount(true);
		if (item > count)
		{
			item -= count + 1;
			continue;
		}
		
		return (*iter)->GetChild(item - 1);
	}
	return 0;
}

unsigned int CQueueItem::GetChildrenCount(bool recursive)
{
	if (!recursive)
		return m_children.size();

	return m_visibleOffspring;
}

bool CQueueItem::RemoveChild(CQueueItem* pItem)
{
	int oldVisibleOffspring = m_visibleOffspring;
	std::vector<CQueueItem*>::iterator iter;
	bool deleted = false;
	for (iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if (*iter == pItem)
		{
			m_visibleOffspring -= 1;
			m_visibleOffspring -= pItem->m_visibleOffspring;
			delete pItem;
			m_children.erase(iter);

			deleted = true;
			break;
		}

		int visibleOffspring = (*iter)->m_visibleOffspring;
		if ((*iter)->RemoveChild(pItem))
		{
			m_visibleOffspring -= visibleOffspring - (*iter)->m_visibleOffspring;
			if (!(*iter)->m_children.size())
			{
				m_visibleOffspring -= 1;
				delete *iter;
				m_children.erase(iter);
			}
			
			deleted = true;
			break;
		}
	}
	if (!deleted)
		return false;

	// Propagate new children count to parent
	CQueueItem* parent = GetParent();
	while (parent)
	{
		parent->m_visibleOffspring -= oldVisibleOffspring - m_visibleOffspring;
		parent = parent->GetParent();
	}
	
	return true;
}

bool CQueueItem::TryRemoveAll()
{
	const int oldVisibleOffspring = m_visibleOffspring;
	std::vector<CQueueItem*>::iterator iter;
	std::vector<CQueueItem*> keepChildren;
	m_visibleOffspring = 0;
	for (iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		CQueueItem* pItem = *iter;
		if (pItem->TryRemoveAll())
			delete pItem;
		else
		{
			keepChildren.push_back(pItem);
			m_visibleOffspring++;
			m_visibleOffspring += pItem->GetChildrenCount(true);
		}
	}
	m_children = keepChildren;

	CQueueItem* parent = GetParent();
	while (parent)
	{
		parent->m_visibleOffspring -= oldVisibleOffspring - m_visibleOffspring;
		parent = parent->GetParent();
	}

	return m_children.empty();
}

CQueueItem* CQueueItem::GetTopLevelItem()
{
	if (!m_parent)
		return this;

	CQueueItem* newParent = m_parent;
	CQueueItem* parent = 0;
	while (newParent)
	{
		parent = newParent;
		newParent = newParent->GetParent();
	}

	return parent;
}

const CQueueItem* CQueueItem::GetTopLevelItem() const
{
	if (!m_parent)
		return this;

	const CQueueItem* newParent = m_parent;
	const CQueueItem* parent = 0;
	while (newParent)
	{
		parent = newParent;
		newParent = newParent->GetParent();
	}

	return parent;
}

int CQueueItem::GetItemIndex() const
{
	const CQueueItem* pParent = GetParent();
	if (!pParent)
		return 0;

	int index = 1;
	for (std::vector<CQueueItem*>::const_iterator iter = pParent->m_children.begin(); iter != pParent->m_children.end(); iter++)
	{
		if (*iter == this)
			break;

		index += (*iter)->GetChildrenCount(true) + 1;
	}

	return index + pParent->GetItemIndex();
}

CFileItem::CFileItem(CServerItem* parent, bool queued, bool download, const wxString& localFile,
					 const wxString& remoteFile, const CServerPath& remotePath, wxLongLong size)
{
	m_parent = parent;
	m_priority = priority_normal;

	m_download = download;
	m_localFile = localFile;
	m_remoteFile = remoteFile;
	m_remotePath = remotePath;
	m_size = size;
	m_itemState = ItemState_Wait;
	m_queued = queued;
	m_active = false;
	m_errorCount = 0;
	m_remove = false;
	m_pEngineData = 0;
}

CFileItem::~CFileItem()
{
}

void CFileItem::SetPriority(enum QueuePriority priority)
{
	m_priority = priority;
}

enum ItemState CFileItem::GetItemState() const
{
	return m_itemState;
}

void CFileItem::SetItemState(enum ItemState itemState)
{
	m_itemState = itemState;
}

enum QueuePriority CFileItem::GetPriority() const
{
	return m_priority;
}

void CFileItem::SetActive(bool active)
{
	if (active && !m_active)
	{
		AddChild(new CStatusItem);
	}
	else if (!active && m_active)
	{
		CQueueItem* pItem = m_children.front();
		RemoveChild(pItem);
	}
	m_active = active;
}

void CFileItem::SaveItem(TiXmlElement* pElement) const
{
	if (GetItemState() == ItemState_Complete)
		return;

	//TODO: Save error items?
	TiXmlElement file("File");

	AddTextElement(&file, "LocalFile", m_localFile);
	AddTextElement(&file, "RemoteFile", m_remoteFile);
	AddTextElement(&file, "RemotePath", m_remotePath.GetSafePath());
	AddTextElement(&file, "Download", m_download ? _T("1") : _T("0"));
	AddTextElement(&file, "Size", m_size.ToString());
	AddTextElement(&file, "ErrorCount", wxString::Format(_T("%d"), m_errorCount));
	AddTextElement(&file, "Priority", wxString::Format(_T("%d"), m_priority));
	AddTextElement(&file, "ItemState", wxString::Format(_T("%d"), m_itemState));
	AddTextElement(&file, "TransferMode", m_transferSettings.binary ? _T("1") : _T("0"));

	pElement->InsertEndChild(file);
}

CServerItem::CServerItem(const CServer& server)
{
	m_server = server;
	m_activeCount = 0;
}

CServerItem::~CServerItem()
{
}

const CServer& CServerItem::GetServer() const
{
	return m_server;
}

wxString CServerItem::GetName() const
{
	return m_server.FormatServer();
}

void CServerItem::AddFileItemToList(CFileItem* pItem)
{
	if (!pItem)
		return;

	m_fileList[pItem->m_queued ? 0 : 1][pItem->GetPriority()].push_back(pItem);
}

CFileItem* CServerItem::GetIdleChild(bool immediateOnly, enum AcceptedTransferDirection direction)
{
	int i = 0;
	for (i = (PRIORITY_COUNT - 1); i >= 0; i--)
	{
		std::list<CFileItem*>& fileList = m_fileList[1][i];
		for (std::list<CFileItem*>::iterator iter = fileList.begin(); iter != fileList.end(); iter++)
		{
			CFileItem* item = *iter;
			if (item->IsActive())
				continue;

			if (direction == all)
				return item;

			if (direction == download)
			{	
				if (item->Download())
					return item;
			}
			else if (!item->Download())
				return item;
		}
	}
	if (immediateOnly)
		return 0;

	for (i = (PRIORITY_COUNT - 1); i >= 0; i--)
	{
		std::list<CFileItem*>& fileList = m_fileList[0][i];
		for (std::list<CFileItem*>::iterator iter = fileList.begin(); iter != fileList.end(); iter++)
		{
			CFileItem* item = *iter;
			if (item->IsActive())
				continue;

			if (direction == all)
				return item;

			if (direction == download)
			{	
				if (item->Download())
					return item;
			}
			else if (!item->Download())
				return item;
		}
	}
	
	return 0;
}

bool CServerItem::RemoveChild(CQueueItem* pItem)
{
	if (!pItem)
		return false;
	
	if (pItem->GetType() == QueueItemType_File)
	{
		CFileItem* pFileItem = reinterpret_cast<CFileItem*>(pItem);
		std::list<CFileItem*>& fileList = m_fileList[pFileItem->m_queued ? 0 : 1][pFileItem->GetPriority()];
		for (std::list<CFileItem*>::iterator iter = fileList.begin(); iter != fileList.end(); iter++)
		{
			if (*iter == pFileItem)
			{
				fileList.erase(iter);
				break;
			}
		}
	}

	return CQueueItem::RemoveChild(pItem);
}

void CServerItem::QueueImmediateFiles()
{
	for (int i = 0; i < PRIORITY_COUNT; i++)
	{
		std::list<CFileItem*> activeList;
		std::list<CFileItem*>& fileList = m_fileList[1][i];
		for (std::list<CFileItem*>::iterator iter = fileList.begin(); iter != fileList.end(); iter++)
		{
			CFileItem* item = *iter;
			wxASSERT(!item->m_queued);
			if (item->IsActive())
				activeList.push_back(item);
			else
			{
				item->m_queued = true;
				m_fileList[0][i].push_back(item);
			}
		}
		fileList = activeList;
	}
}

void CServerItem::QueueImmediateFile(CFileItem* pItem)
{
	if (pItem->m_queued)
		return;

	std::list<CFileItem*>& fileList = m_fileList[1][pItem->GetPriority()];
	for (std::list<CFileItem*>::iterator iter = fileList.begin(); iter != fileList.end(); iter++)
	{
		if (*iter != pItem)
			continue;

		pItem->m_queued = true;
		fileList.erase(iter);
		m_fileList[1][pItem->GetPriority()].push_back(pItem);
		return;
	}
	wxASSERT(false);
}

void CServerItem::SaveItem(TiXmlElement* pElement) const
{
	TiXmlElement server("Server");
	SetServer(&server, m_server);

	for (std::vector<CQueueItem*>::const_iterator iter = m_children.begin(); iter != m_children.end(); iter++)
		(*iter)->SaveItem(&server);

	pElement->InsertEndChild(server);
}

CFolderItem::CFolderItem(CServerItem* parent, bool queued, bool download, const wxString& localPath, const CServerPath& remotePath)
{
	m_parent = parent;

	m_download = download;
	m_localPath = localPath;
	m_remotePath = remotePath;
	m_queued = queued;
	m_remove = false;
	m_active = false;
	m_count = 0;
	m_pDir = 0;

	t_dirPair pair;
	pair.localPath = localPath;
	pair.remotePath = remotePath;
	m_dirsToCheck.push_back(pair);
}

bool CFolderItem::TryRemoveAll()
{
	if (!m_active)
		return true;

	m_remove = true;
	return false;
}

wxLongLong CServerItem::GetTotalSize(bool& partialSizeInfo) const
{
	wxLongLong totalSize = 0;
	for (int i = 0; i < PRIORITY_COUNT; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			const std::list<CFileItem*>& fileList = m_fileList[j][i];
			for (std::list<CFileItem*>::const_iterator iter = fileList.begin(); iter != fileList.end(); iter++)
			{
				const CFileItem* item = *iter;
				if (item->GetItemState() != ItemState_Complete)
				{
					wxLongLong size = item->GetSize();
					if (size >= 0)
						totalSize += size;
					else
						partialSizeInfo = true;
				}
			}
		}
	}

	return totalSize;
}

bool CFileItem::TryRemoveAll()
{
	if (!IsActive())
		return true;

	m_remove = true;
	return false;
}

CQueueView::CQueueView(wxWindow* parent, wxWindowID id, CMainFrame* pMainFrame, CAsyncRequestQueue *pAsyncRequestQueue)
	: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN | wxLC_REPORT | wxLC_VIRTUAL | wxSUNKEN_BORDER),
	  m_pMainFrame(pMainFrame),
	  m_pAsyncRequestQueue(pAsyncRequestQueue)
{
	m_allowBackgroundErase = true;

	m_itemCount = 0;
	m_activeCount = 0;
	m_activeCountDown = 0;
	m_activeCountUp = 0;
	m_activeMode = 0;
	m_quit = false;
	m_waitStatusLineUpdate = false;
	m_pFolderProcessingThread = 0;
	
	InsertColumn(0, _("Server / Local file"), wxLIST_FORMAT_LEFT, 150);
	InsertColumn(1, _("Direction"), wxLIST_FORMAT_CENTER, 60);
	InsertColumn(2, _("Remote file"), wxLIST_FORMAT_LEFT, 150);
	InsertColumn(3, _("Size"), wxLIST_FORMAT_RIGHT, 70);
	InsertColumn(4, _("Priority"), wxLIST_FORMAT_LEFT, 60);
	InsertColumn(5, _("Status"), wxLIST_FORMAT_LEFT, 150);

	// Create and assign the image list for the queue
	wxImageList* pImageList = new wxImageList(16, 16);

	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FOLDERCLOSED"),  wxART_OTHER, wxSize(16, 16)));
	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FILE"),  wxART_OTHER, wxSize(16, 16)));
	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FOLDERCLOSED"),  wxART_OTHER, wxSize(16, 16)));
	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FOLDER"),  wxART_OTHER, wxSize(16, 16)));

	AssignImageList(pImageList, wxIMAGE_LIST_SMALL);

	//Initialize the engine data
	t_EngineData data;
	data.active = false;
	data.pEngine = 0; // TODO: Primary transfer engine data
	data.pItem = 0;
	data.pStatusLineCtrl = 0;
	data.state = t_EngineData::none;
	m_engineData.push_back(data);
	
	int engineCount = COptions::Get()->GetOptionVal(OPTION_NUMTRANSFERS);
	for (int i = 0; i < engineCount; i++)
	{
		data.pEngine = new CFileZillaEngine();
		data.pEngine->Init(this, COptions::Get());
		m_engineData.push_back(data);
	}

	SettingsChanged();
	LoadQueue();
}

CQueueView::~CQueueView()
{
	if (m_pFolderProcessingThread)
	{
		m_pFolderProcessingThread->Delete();
		m_pFolderProcessingThread->Wait();
		delete m_pFolderProcessingThread;
	}

	for (std::vector<CServerItem*>::iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
		delete *iter;

	for (unsigned int engineIndex = 1; engineIndex < m_engineData.size(); engineIndex++)
		delete m_engineData[engineIndex].pEngine;
}

bool CQueueView::QueueFile(bool queueOnly, bool download, const wxString& localFile, 
						   const wxString& remoteFile, const CServerPath& remotePath,
						   const CServer& server, wxLongLong size)
{
	CServerItem* item = GetServerItem(server);
	if (!item)
	{
		item = new CServerItem(server);
		m_serverList.push_back(item);
		m_itemCount++;
	}

	CFileItem* fileItem = new CFileItem(item, queueOnly, download, localFile, remoteFile, remotePath, size);
	fileItem->m_transferSettings.binary = ShouldUseBinaryMode(download ? remoteFile : wxFileName(localFile).GetFullName());
	item->AddChild(fileItem);
	item->AddFileItemToList(fileItem);

	m_itemCount++;

	SetItemCount(m_itemCount);

	if (!m_activeMode && !queueOnly)
		m_activeMode = 1;

	m_waitStatusLineUpdate = true;
	while (TryStartNextTransfer());
	m_waitStatusLineUpdate = false;
	UpdateStatusLinePositions();

	UpdateQueueSize();

	return true;
}

// Declared const due to design error in wxWidgets.
// Won't be fixed since a fix would break backwards compatibility
// Both functions use a const_cast<CQueueView *>(this) and modify
// the instance.
wxString CQueueView::OnGetItemText(long item, long column) const
{
	CQueueView* pThis = const_cast<CQueueView*>(this);
	
	CQueueItem* pItem = pThis->GetQueueItem(item);
	if (!pItem)
		return _T("");

	switch (pItem->GetType())
	{
	case QueueItemType_Server:
		{
			CServerItem* pServerItem = reinterpret_cast<CServerItem*>(pItem);
			if (!column)
				return pServerItem->GetName();
		}
		break;
	case QueueItemType_File:
		{
			CFileItem* pFileItem = reinterpret_cast<CFileItem*>(pItem);
			switch (column)
			{
			case 0:
				return pFileItem->GetIndent() + pFileItem->GetLocalFile();
			case 1:
				if (pFileItem->Download())
					if (pFileItem->Queued())
						return _T("<--");
					else
						return _T("<<--");
				else
					if (pFileItem->Queued())
						return _T("-->");
					else
						return _T("-->>");
				break;
			case 2:
				return pFileItem->GetRemotePath().FormatFilename(pFileItem->GetRemoteFile());
			case 3:
				{
					wxLongLong size = pFileItem->GetSize();
					if (size >= 0)
						return size.ToString();
					else
						return _T("?");
				}
			case 4:
				switch (pFileItem->GetPriority())
				{
				case 0:
					return _("Lowest");
				case 1:
					return _("Low");
				default:
				case 2:
					return _("Normal");
				case 3:
					return _("High");
				case 4:
					return _("Highest");
				}
				break;
			case 5:
				return pFileItem->m_statusMessage;
			default:
				break;
			}
		}
		break;
	case QueueItemType_Folder:
		{
			CFolderItem* pFolderItem = reinterpret_cast<CFolderItem*>(pItem);
			switch (column)
			{
			case 0:
				return _T("  ") + pFolderItem->GetLocalPath();
			case 1:
				if (pFolderItem->Download())
					if (pFolderItem->Queued())
						return _T("<--");
					else
						return _T("<<--");
				else
					if (pFolderItem->Queued())
						return _T("-->");
					else
						return _T("-->>");
				break;
			case 2:
				return pFolderItem->GetRemotePath().GetPath();
			case 5:
				return pFolderItem->m_statusMessage;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	return _T("");
}

int CQueueView::OnGetItemImage(long item) const
{
	CQueueView* pThis = const_cast<CQueueView*>(this);

	CQueueItem* pItem = pThis->GetQueueItem(item);
	if (!pItem)
		return -1;

	switch (pItem->GetType())
	{
	case QueueItemType_Server:
		return 0;
	case QueueItemType_File:
		return 1;
	case QueueItemType_Folder:
			return 3;
	default:
		return -1;
	}

	return -1;
}

CQueueItem* CQueueView::GetQueueItem(unsigned int item)
{
	std::vector<CServerItem*>::iterator iter;
	for (iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
	{
		if (!item)
			return *iter;
		
		unsigned int count = (*iter)->GetChildrenCount(true);
		if (item > count)
		{
			item -= count + 1;
			continue;
		}
		
		return (*iter)->GetChild(item - 1);
	}
	return 0;
}

void CQueueView::OnEngineEvent(wxEvent &event)
{
	std::vector<t_EngineData>::iterator iter;
	for (iter = m_engineData.begin(); iter != m_engineData.end(); iter++)
	{
		if ((wxObject *)iter->pEngine == event.GetEventObject())
			break;
	}
	if (iter == m_engineData.end())
		return;

	t_EngineData& data = *iter;

	CNotification *pNotification = data.pEngine->GetNextNotification();
	while (pNotification)
	{
		switch (pNotification->GetID())
		{
		case nId_logmsg:
			m_pMainFrame->GetStatusView()->AddToLog(reinterpret_cast<CLogmsgNotification *>(pNotification));
			delete pNotification;
			break;
		case nId_operation:
			ProcessReply(*iter, reinterpret_cast<COperationNotification*>(pNotification));
			delete pNotification;
			break;
		case nId_listing:
			{
				const CServer* const pServer = m_pMainFrame->GetState()->GetServer();
				if (pServer && *pServer == data.lastServer)
					m_pMainFrame->GetState()->SetRemoteDir(reinterpret_cast<CDirectoryListingNotification *>(pNotification)->DetachDirectoryListing(), true);
			}
			delete pNotification;
			break;
		case nId_asyncrequest:
			m_pMainFrame->AddToRequestQueue(data.pEngine, reinterpret_cast<CAsyncRequestNotification *>(pNotification));
			break;
		case nId_active:
			{
				CActiveNotification *pActiveNotification = reinterpret_cast<CActiveNotification *>(pNotification);
				if (pActiveNotification->IsRecv())
					m_pMainFrame->UpdateRecvLed();
				else
					m_pMainFrame->UpdateSendLed();
				delete pNotification;
			}
			break;
		case nId_transferstatus:
			{
				CTransferStatusNotification *pTransferStatusNotification = reinterpret_cast<CTransferStatusNotification *>(pNotification);
				const CTransferStatus *pStatus = pTransferStatusNotification->GetStatus();

				if (data.active)
					data.pStatusLineCtrl->SetTransferStatus(pStatus);

				delete pNotification;
			}
			break;
		default:
			delete pNotification;
			break;
		}

		if (!data.pEngine || m_engineData.empty())
			break;

		pNotification = data.pEngine->GetNextNotification();
	}
}

CServerItem* CQueueView::GetServerItem(const CServer& server)
{
	for (std::vector<CServerItem*>::iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
	{
		if ((*iter)->GetServer() == server)
			return *iter;
	}
	return NULL;
}

bool CQueueView::TryStartNextTransfer()
{
	if (m_quit || !m_activeMode)
		return false;

	// Check transfer limit
	if (m_activeCount >= COptions::Get()->GetOptionVal(OPTION_NUMTRANSFERS))
		return false;

	// Check limits for concurrent up/downloads
	const int maxDownloads = COptions::Get()->GetOptionVal(OPTION_CONCURRENTDOWNLOADLIMIT);
	const int maxUploads = COptions::Get()->GetOptionVal(OPTION_CONCURRENTUPLOADLIMIT);
	enum AcceptedTransferDirection wantedDirection;
	if (maxDownloads && m_activeCountDown >= maxDownloads)
	{
		if (maxUploads && m_activeCountUp >= maxUploads)
			return false;
		else
			wantedDirection = upload;
	}
	else if (maxUploads && m_activeCountUp >= maxUploads)
		wantedDirection = download;
	else
		wantedDirection = all;

	// Find first idle server and assign a transfer to it.
	unsigned int engineIndex;
	for (engineIndex = 1; engineIndex < m_engineData.size(); engineIndex++)
	{
		if (!m_engineData[engineIndex].active)
			break;
	}
	if (engineIndex >= m_engineData.size())
		return false;

	t_EngineData& engineData = m_engineData[engineIndex];

	// Check all servers for the item with the highest priority
	CFileItem* fileItem = 0;
	CServerItem* serverItem = 0;
	for (std::vector<CServerItem*>::iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
	{
		CServerItem* currentServerItem = *iter;
		int maxCount = currentServerItem->GetServer().MaximumMultipleConnections();
		if (maxCount && currentServerItem->m_activeCount >= maxCount)
			continue;

		CFileItem* newFileItem = currentServerItem->GetIdleChild(m_activeMode == 1, wantedDirection);
		if (!newFileItem)
			continue;

		if (!fileItem || newFileItem->GetPriority() > fileItem->GetPriority())
		{
			serverItem = currentServerItem;
			fileItem = newFileItem;
			if (fileItem->GetPriority() == priority_highest)
				break;
		}
	}
	if (!fileItem)
		return false;

	// Found idle item
	fileItem->SetActive(true);
	m_itemCount++;
	SetItemCount(m_itemCount);

	engineData.pItem = fileItem;
	fileItem->m_pEngineData = &engineData;
	engineData.active = true;
	serverItem->m_activeCount++;
	m_activeCount++;
	if (fileItem->Download())
		m_activeCountDown++;
	else
		m_activeCountUp++;

	const CServer oldServer = engineData.lastServer;
	engineData.lastServer = serverItem->GetServer();
	
	if (!engineData.pEngine->IsConnected())
		engineData.state = t_EngineData::connect;
	else if (oldServer != serverItem->GetServer())
		engineData.state = t_EngineData::disconnect;
	else
		engineData.state = t_EngineData::transfer;

	// Create status line
	int lineIndex = GetItemIndex(fileItem);
	
	wxRect rect;
	GetItemRect(lineIndex + 1, rect);
	m_allowBackgroundErase = false;
	if (!engineData.pStatusLineCtrl)
		engineData.pStatusLineCtrl = new CStatusLineCtrl(this, engineData, rect);
	else
	{
		engineData.pStatusLineCtrl->SetTransferStatus(0);
		engineData.pStatusLineCtrl->SetSize(rect);
		engineData.pStatusLineCtrl->Show();
	}
	m_allowBackgroundErase = true;
	m_statusLineList.push_back(engineData.pStatusLineCtrl);

	SendNextCommand(engineData);
	
	return true;
}

void CQueueView::ProcessReply(t_EngineData& engineData, COperationNotification* pNotification)
{
	// Cancel pending requests
	m_pAsyncRequestQueue->ClearPending(engineData.pEngine);

	// Process reply from the engine
	int replyCode = pNotification->nReplyCode;

	if ((replyCode & FZ_REPLY_CANCELED) == FZ_REPLY_CANCELED)
	{
		ResetEngine(engineData, engineData.pItem ? engineData.pItem->m_remove : false);
		return;
	}

	// Cycle through queue states
	switch (engineData.state)
	{
	case t_EngineData::disconnect:
		engineData.state = t_EngineData::connect;
		if (engineData.active)
			engineData.pStatusLineCtrl->SetTransferStatus(0);
		break;
	case t_EngineData::connect:
		if (replyCode == FZ_REPLY_OK)
		{
			engineData.state = t_EngineData::transfer;
			if (engineData.active)
				engineData.pStatusLineCtrl->SetTransferStatus(0);
		}
		else if (!IncreaseErrorCount(engineData))
			return;
		break;
	case t_EngineData::transfer:
		if (replyCode == FZ_REPLY_OK)
		{
			ResetEngine(engineData, true);
			return;
		}
		else if (!IncreaseErrorCount(engineData))
			return;
		if (replyCode & FZ_REPLY_DISCONNECTED)
			engineData.state = t_EngineData::connect;
		break;
	default:
		return;
	}

	if (!m_activeMode)
	{
		ResetEngine(engineData, engineData.pItem ? engineData.pItem->m_remove : false);
		return;
	}
	
	SendNextCommand(engineData);
}

void CQueueView::ResetEngine(t_EngineData& data, bool removeFileItem)
{
	if (!data.active)
		return;

	m_waitStatusLineUpdate = true;

	if (data.pStatusLineCtrl)
	{
		for (std::list<CStatusLineCtrl*>::iterator iter = m_statusLineList.begin(); iter != m_statusLineList.end(); iter++)
		{
			if (*iter == data.pStatusLineCtrl)
			{
				m_statusLineList.erase(iter);
				break;
			}
		}
		m_allowBackgroundErase = false;
		data.pStatusLineCtrl->Hide();
		m_allowBackgroundErase = true;
	}
	if (data.pItem)
	{
		wxASSERT(data.pItem->IsActive());
		wxASSERT(data.pItem->m_pEngineData = &data);
		if (data.pItem->IsActive())
		{
			data.pItem->SetActive(false);
			m_itemCount--;
			SetItemCount(m_itemCount);
		}
		if (data.pItem->Download())
		{
			wxASSERT(m_activeCountDown > 0);
			if (m_activeCountDown > 0)
				m_activeCountDown--;
		}
		else
		{
			wxASSERT(m_activeCountUp > 0);
			if (m_activeCountUp > 0)
				m_activeCountUp--;
		}

		if (removeFileItem)
			RemoveItem(data.pItem);
		else
			ResetItem(data.pItem);
		data.pItem = 0;
	}
	data.active = false;
	data.state = t_EngineData::none;
	CServerItem* item = GetServerItem(data.lastServer);
	if (item)
	{
		wxASSERT(item->m_activeCount > 0);
		if (item->m_activeCount > 0)
			item->m_activeCount--;
	}
	m_activeCount--;

	while (TryStartNextTransfer());

	m_waitStatusLineUpdate = false;
	UpdateStatusLinePositions();
	UpdateQueueSize();

	CheckQueueState();
}

void CQueueView::RemoveItem(CQueueItem* item)
{
	// RemoveItem assumes that the item has already removed from all engines

	CQueueItem* topLevelItem = item->GetTopLevelItem();

	int count = topLevelItem->GetChildrenCount(true);
	topLevelItem->RemoveChild(item);

	int oldCount = m_itemCount;
	if (!topLevelItem->GetChild(0))
	{
		std::vector<CServerItem*>::iterator iter;
		for (iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
		{
			if (*iter == topLevelItem)
				break;
		}
		if (iter != m_serverList.end())
			m_serverList.erase(iter);
		delete topLevelItem;

		m_itemCount -= count + 1;
		SetItemCount(m_itemCount);
	}
	else
	{
		count -= topLevelItem->GetChildrenCount(true);
		m_itemCount -= count;
		SetItemCount(m_itemCount);
	}

	if (oldCount > m_itemCount)
	{
		bool eraseBackground = GetTopItem() + GetCountPerPage() + 1 >= m_itemCount;
		Refresh(eraseBackground);
	}

	UpdateStatusLinePositions();
}

void CQueueView::SendNextCommand(t_EngineData& engineData)
{
	while (true)
	{
		if (engineData.state == t_EngineData::disconnect)
		{
			engineData.pItem->m_statusMessage = _("Disconnecting from previous server");
			if (engineData.pEngine->Command(CDisconnectCommand()) == FZ_REPLY_WOULDBLOCK)
				return;

			engineData.state = t_EngineData::connect;
			if (engineData.active)
				engineData.pStatusLineCtrl->SetTransferStatus(0);
		}

		if (engineData.state == t_EngineData::connect)
		{
			engineData.pItem->m_statusMessage = _("Connecting");

			int	res = engineData.pEngine->Command(CConnectCommand(engineData.lastServer));
			if (res == FZ_REPLY_WOULDBLOCK)
				return;

			if (res == FZ_REPLY_ALREADYCONNECTED)
			{
				engineData.state = t_EngineData::disconnect;
				continue;
			}

			if (res == FZ_REPLY_OK)
			{
				engineData.state = t_EngineData::transfer;
				if (engineData.active)
					engineData.pStatusLineCtrl->SetTransferStatus(0);
				break;
			}

			if (!IncreaseErrorCount(engineData))
				return;
			continue;
		}

		if (engineData.state == t_EngineData::transfer)
		{
			CFileItem* fileItem = engineData.pItem;

			fileItem->m_statusMessage = _("Transferring");

			int res = engineData.pEngine->Command(CFileTransferCommand(fileItem->GetLocalFile(), fileItem->GetRemotePath(), 
												fileItem->GetRemoteFile(), fileItem->Download(), fileItem->m_transferSettings));
			if (res == FZ_REPLY_WOULDBLOCK)
				return;

			if (res == FZ_REPLY_NOTCONNECTED)
			{
				engineData.state = t_EngineData::connect;
				continue;
			}

			if (res == FZ_REPLY_OK)
			{
				ResetEngine(engineData, true);
				return;
			}

			if (!IncreaseErrorCount(engineData))
				return;
			continue;
		}
	}
}

bool CQueueView::SetActive(bool active /*=true*/)
{
	if (!active)
	{
		m_activeMode = 0;
		for (std::vector<CServerItem*>::iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
			(*iter)->QueueImmediateFiles();

		UpdateStatusLinePositions();

		// Send active engines the cancel command
		unsigned int engineIndex;
		for (engineIndex = 1; engineIndex < m_engineData.size(); engineIndex++)
		{
			t_EngineData& data = m_engineData[engineIndex];
			if (data.active)
				data.pEngine->Command(CCancelCommand());
		}

		return m_activeCount == 0;
	}
	else
	{
		m_activeMode = 2;

		m_waitStatusLineUpdate = true;
		while (TryStartNextTransfer());
		m_waitStatusLineUpdate = false;
		UpdateStatusLinePositions();

		CheckQueueState();
	}

	return true;
}

bool CQueueView::Quit()
{
	m_quit = true;
	
	bool canQuit = true;
	if (!SetActive(false))
		canQuit = false;

	for (unsigned int i = 0; i < 2; i++)
	{
		if (!m_queuedFolders[i].empty())
		{
			canQuit = false;
			for (std::list<CFolderItem*>::iterator iter = m_queuedFolders[i].begin(); iter != m_queuedFolders[i].end(); iter++)
				(*iter)->m_remove = true;
		}
	}
	if (m_pFolderProcessingThread)
		canQuit = false;

	if (!canQuit)
		return false;

	for (unsigned int engineIndex = 1; engineIndex < m_engineData.size(); engineIndex++)
	{
		t_EngineData& data = m_engineData[engineIndex];
		delete data.pEngine;
		data.pEngine = 0;
	}
	m_engineData.clear();

	SaveQueue();

	return true;
}

void CQueueView::CheckQueueState()
{
	if (!m_activeCount)
		m_activeMode = 0;

	if (m_quit)
		m_pMainFrame->Close();
}

bool CQueueView::IncreaseErrorCount(t_EngineData& engineData)
{
	engineData.pItem->m_errorCount++;
	if (engineData.pItem->m_errorCount <= COptions::Get()->GetOptionVal(OPTION_TRANSFERRETRYCOUNT))
		return true;

	ResetEngine(engineData, true);

	//TODO: Error description?

	return false;
}

void CQueueView::ResetItem(CFileItem* item)
{
	if (!item->Queued())
		reinterpret_cast<CServerItem*>(item->GetTopLevelItem())->QueueImmediateFiles();

	UpdateStatusLinePositions();
}

int CQueueView::GetItemIndex(const CQueueItem* item)
{
	const CQueueItem* pTopLevelItem = item->GetTopLevelItem();

	int index = 0;
	for (std::vector<CServerItem*>::const_iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
	{
		if (pTopLevelItem == *iter)
			break;

		index += (*iter)->GetChildrenCount(true) + 1;
	}

	return index + item->GetItemIndex();
}

void CQueueView::UpdateStatusLinePositions()
{
	if (m_waitStatusLineUpdate)
		return;

	int topItem = GetTopItem();
	int bottomItem = topItem + GetCountPerPage();

	for (std::list<CStatusLineCtrl*>::iterator iter = m_statusLineList.begin(); iter != m_statusLineList.end(); iter++)
	{
		CStatusLineCtrl *pCtrl = *iter;
		int index = GetItemIndex(pCtrl->GetItem()) + 1;
		if (index < topItem || index > bottomItem)
		{
			pCtrl->Show(false);
			continue;
		}

		wxRect rect;
		if (!GetItemRect(index, rect))
		{
			pCtrl->Show(false);
			continue;
		}
		m_allowBackgroundErase = GetTopItem() + GetCountPerPage() + 1 >= m_itemCount;
		pCtrl->SetSize(rect);
		m_allowBackgroundErase = false;
		pCtrl->Show();
		m_allowBackgroundErase = true;
	}
}

void CQueueView::UpdateQueueSize()
{
	wxStatusBar *pStatusBar = m_pMainFrame->GetStatusBar();
	if (!pStatusBar)
		return;

	// Collect total queue size
	wxLongLong totalSize = 0;

	bool partialSizeInfo = false;
	for (std::vector<CServerItem*>::const_iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
		totalSize += (*iter)->GetTotalSize(partialSizeInfo);

	wxString queueSize;
	if (totalSize == 0 && !partialSizeInfo)
		queueSize = _("Queue: empty");
	if (totalSize > (1000 * 1000))
	{
		totalSize /= 1000 * 1000;
		queueSize.Printf(_("Queue: %s%d MB"), partialSizeInfo ? _T(">") : _T(""), totalSize.GetLo());
	}
	else if (totalSize > 1000)
	{
		totalSize /= 1000;
		queueSize.Printf(_("Queue: %s%d KB"), partialSizeInfo ? _T(">") : _T(""), totalSize.GetLo());
	}
	else
		queueSize.Printf(_("Queue: %s%d bytes"), partialSizeInfo ? _T(">") : _T(""), totalSize.GetLo());
	pStatusBar->SetStatusText(queueSize, 4);
}

bool CQueueView::QueueFolder(bool queueOnly, bool download, const wxString& localPath, const CServerPath& remotePath, const CServer& server)
{
	CServerItem* item = GetServerItem(server);
	if (!item)
	{
		item = new CServerItem(server);
		m_serverList.push_back(item);
		m_itemCount++;
	}

	CFolderItem* folderItem = new CFolderItem(item, queueOnly, download, localPath, remotePath);
	item->AddChild(folderItem);

	m_itemCount++;

	folderItem->m_statusMessage = _("Waiting");

	SetItemCount(m_itemCount);

	m_queuedFolders[download ? 0 : 1].push_back(folderItem);

	ProcessFolderItems();

	return true;
}

bool CQueueView::ProcessFolderItems(int type /*=-1*/)
{
	if (type == -1)
	{
		while (ProcessFolderItems(0));
		ProcessUploadFolderItems();

		return true;
	}

	return false;
}

void CQueueView::ProcessUploadFolderItems()
{
	if (m_queuedFolders[1].empty())
	{
		if (m_quit)
			m_pMainFrame->Close();

		return;
	}

	if (m_pFolderProcessingThread)
		return;

	CFolderItem* pItem = m_queuedFolders[1].front();

	if (pItem->Queued())
		pItem->m_statusMessage = _("Scanning for files to add to queue");
	else
		pItem->m_statusMessage = _("Scanning for files to upload");
	pItem->m_active = true;
	m_pFolderProcessingThread = new CFolderProcessingThread(this, pItem);
	m_pFolderProcessingThread->Create();
	m_pFolderProcessingThread->Run();

	Refresh(false);
}

void CQueueView::OnFolderThreadComplete(wxCommandEvent& event)
{
	if (!m_pFolderProcessingThread)
		return;

	wxASSERT(!m_queuedFolders[1].empty());
	CFolderItem* pItem = m_queuedFolders[1].front();
	m_queuedFolders[1].pop_front();

	RemoveItem(pItem);
	
	m_pFolderProcessingThread->Wait();
	delete m_pFolderProcessingThread;
	m_pFolderProcessingThread = 0;

	ProcessUploadFolderItems();
}

bool CQueueView::QueueFiles(const std::list<t_newEntry> &entryList, bool queueOnly, bool download, CServerItem* pServerItem)
{
	wxASSERT(pServerItem);

	for (std::list<t_newEntry>::const_iterator iter = entryList.begin(); iter != entryList.end(); iter++)
	{
		const t_newEntry& entry = *iter;

		CFileItem* fileItem = new CFileItem(pServerItem, queueOnly, download, entry.localFile, entry.remoteFile, entry.remotePath, entry.size);
		fileItem->m_transferSettings.binary = ShouldUseBinaryMode(download ? entry.remoteFile : wxFileName(entry.localFile).GetFullName());
		pServerItem->AddChild(fileItem);
		pServerItem->AddFileItemToList(fileItem);

		m_itemCount++;
	}

	SetItemCount(m_itemCount);

	if (!m_activeMode && !queueOnly)
		m_activeMode = 1;

	m_waitStatusLineUpdate = true;
	while (TryStartNextTransfer());
	m_waitStatusLineUpdate = false;
	UpdateStatusLinePositions();

	UpdateQueueSize();

	return true;
}

void CQueueView::SaveQueue()
{
	// We have to synchronize access to queue.xml so that multiple processed don't write 
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_QUEUE);

	wxFileName file(wxGetApp().GetSettingsDir(), _T("queue.xml"));
	TiXmlElement* pDocument = GetXmlFile(file);
	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nThe queue will not be saved."), file.GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return;
	}

	TiXmlElement* pQueue = pDocument->FirstChildElement("Queue");
	if (!pQueue)
	{
		pQueue = pDocument->InsertEndChild(TiXmlElement("Queue"))->ToElement();
	}

	wxASSERT(pQueue);

	for (std::vector<CServerItem*>::iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
		(*iter)->SaveItem(pQueue);

	if (!pDocument->GetDocument()->SaveFile(file.GetFullPath().mb_str()))
	{
		wxString msg = wxString::Format(_("Could not write \"%s\", the queue could not be saved."), file.GetFullPath().c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
	}

	delete pDocument->GetDocument();
}

void CQueueView::LoadQueue()
{
	// We have to synchronize access to queue.xml so that multiple processed don't write 
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_QUEUE);

	wxFileName file(wxGetApp().GetSettingsDir(), _T("queue.xml"));
	TiXmlElement* pDocument = GetXmlFile(file);
	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nThe queue will not be saved."), file.GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return;
	}

	TiXmlElement* pQueue = pDocument->FirstChildElement("Queue");
	if (!pQueue)
	{
		delete pDocument->GetDocument();
		return;
	}

	TiXmlElement* pServer = pQueue->FirstChildElement("Server");
	while (pServer)
	{
		CServer server;
		if (GetServer(pServer, server))
		{
			CServerItem *pServerItem = 0;

			TiXmlElement* pFile = pServer->FirstChildElement("File");
			while (pFile)
			{
				wxString localFile = GetTextElement(pFile, "LocalFile");
				wxString remoteFile = GetTextElement(pFile, "RemoteFile");
				wxString safeRemotePath = GetTextElement(pFile, "RemotePath");
				bool download = GetTextElementInt(pFile, "Download") != 0;
				wxLongLong size = GetTextElementLongLong(pFile, "Size", -1);
				unsigned int errorCount = GetTextElementInt(pFile, "ErrorCount");
				unsigned int priority = GetTextElementInt(pFile, "Priority", priority_normal);
				unsigned int itemState = GetTextElementInt(pFile, "ItemState", ItemState_Wait);
				bool binary = GetTextElementInt(pFile, "TransferMode", 1) != 0;

				CServerPath remotePath;
				if (localFile != _T("") && remoteFile != _T("") && remotePath.SetSafePath(safeRemotePath) &&
					size >= -1 && priority < PRIORITY_COUNT &&
					(itemState == ItemState_Wait || itemState == ItemState_Error))
				{
					if (!pServerItem)
					{
						pServerItem = GetServerItem(server);
						if (!pServerItem)
						{
							pServerItem = new CServerItem(server);
							m_serverList.push_back(pServerItem);
							m_itemCount++;
						}
					}
					CFileItem* fileItem = new CFileItem(pServerItem, true, download, localFile, remoteFile, remotePath, size);
					fileItem->m_transferSettings.binary = binary;
					fileItem->SetPriority((enum QueuePriority)priority);
					fileItem->SetItemState((enum ItemState)itemState);
					fileItem->m_errorCount = errorCount;
					pServerItem->AddChild(fileItem);
					pServerItem->AddFileItemToList(fileItem);
					m_itemCount++;
				}

				pFile = pFile->NextSiblingElement("File");
			}
		}

		pServer = pServer->NextSiblingElement("Server");
	}
	pDocument->RemoveChild(pQueue);

	if (!pDocument->GetDocument()->SaveFile(file.GetFullPath().mb_str()))
	{
		wxString msg = wxString::Format(_("Could not write \"%s\", the queue could not be saved."), file.GetFullPath().c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
	}

	delete pDocument->GetDocument();

	SetItemCount(m_itemCount);
	UpdateQueueSize();
}

void CQueueView::SettingsChanged()
{
	m_asciiFiles.clear();
	wxString extensions = COptions::Get()->GetOption(OPTION_ASCIIFILES);
	wxString ext;
	int pos = extensions.Find(_T("|"));
	while (pos != -1)
	{
		if (!pos)
		{
			if (ext != _T(""))
			{
				ext.Replace(_T("\\\\"), _T("\\")); 
				m_asciiFiles.push_back(ext);
				ext = _T("");
			}
		}
		else if (extensions.c_str()[pos - 1] != '\\')
		{
			ext += extensions.Left(pos);
			ext.Replace(_T("\\\\"), _T("\\")); 
			m_asciiFiles.push_back(ext);
			ext = _T("");
		}
		else
		{
			ext += extensions.Left(pos - 1) + _T("|");
		}
		extensions = extensions.Mid(pos + 1);
		pos = extensions.Find(_T("|"));
	}
	ext += extensions;
	ext.Replace(_T("\\\\"), _T("\\")); 
	m_asciiFiles.push_back(ext);
}

bool CQueueView::ShouldUseBinaryMode(wxString filename)
{
	int mode = COptions::Get()->GetOptionVal(OPTION_ASCIIBINARY);
	if (mode == 1)
		return false;
	else if (mode == 2)
		return true;

	int pos = filename.find('.');
	if (pos == -1)
		return COptions::Get()->GetOptionVal(OPTION_ASCIINOEXT) != 0;
	else if (!pos)
		return COptions::Get()->GetOptionVal(OPTION_ASCIIDOTFILE) != 0;
	
	wxString ext = filename.Mid(pos + 1);
	for (std::list<wxString>::const_iterator iter = m_asciiFiles.begin(); iter != m_asciiFiles.end(); iter++)
		if (*iter == ext)
			return false;

	return true;
}

void CQueueView::OnScrollEvent(wxScrollWinEvent& event)
{
	event.Skip();
	wxCommandEvent evt(fzEVT_UPDATE_STATUSLINES, wxID_ANY);
	AddPendingEvent(evt);
}

void CQueueView::OnUpdateStatusLines(wxCommandEvent& event)
{
	UpdateStatusLinePositions();
}

void CQueueView::OnMouseWheel(wxMouseEvent& event)
{
	event.Skip();
	wxCommandEvent evt(fzEVT_UPDATE_STATUSLINES, wxID_ANY);
	AddPendingEvent(evt);
}

void CQueueView::OnContextMenu(wxContextMenuEvent& event)
{
	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_QUEUE"));
	if (!pMenu)
		return;

	pMenu->Check(XRCID("ID_PROCESSQUEUE"), IsActive() ? true : false);

	PopupMenu(pMenu);
	delete pMenu;
}

void CQueueView::OnProcessQueue(wxCommandEvent& event)
{
	SetActive(event.IsChecked());
}

void CQueueView::OnStopAndClear(wxCommandEvent& event)
{
	SetActive(false);
	RemoveAll();
}

void CQueueView::RemoveAll()
{
	// This function removes all inactive items and queues active items
	// for removal
	std::vector<CServerItem*> newServerList;
	m_itemCount = 0;
	for (std::vector<CServerItem*>::iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
	{
		if ((*iter)->TryRemoveAll())
			delete *iter;
		else
		{
			newServerList.push_back(*iter);
			m_itemCount += 1 + (*iter)->GetChildrenCount(true);
		}
	}
	SetItemCount(m_itemCount);

	m_serverList = newServerList;
	UpdateStatusLinePositions();
	UpdateQueueSize();
	CheckQueueState();
	Refresh();
}

void CQueueView::OnRemoveSelected(wxCommandEvent& event)
{
	std::list<long> selectedItems;
	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		selectedItems.push_front(item);
	}

	m_waitStatusLineUpdate = true;

	long skip = -1;
	for (std::list<long>::const_iterator iter = selectedItems.begin(); iter != selectedItems.end(); iter++)
	{
		if (*iter == skip)
			continue;

		CQueueItem* pItem = GetQueueItem(*iter);
		if (!pItem)
			continue;

		if (pItem->GetType() == QueueItemType_Status)
			continue;
		else if (pItem->GetType() == QueueItemType_Folder)
		{
			CFolderItem* pFolder = (CFolderItem*)pItem;
			if (pFolder->m_active)
			{
				pFolder->m_remove = true;
				continue;
			}
		}
		else if (pItem->GetType() == QueueItemType_Server)
		{
			CServerItem* pServer = (CServerItem*)pItem;
			StopItem(pServer);

			// Server items get deleted automatically if all children are gone
			continue;
		}
		else if (pItem->GetType() == QueueItemType_File)
		{
			CFileItem* pFile = (CFileItem*)pItem;
			if (pFile->IsActive())
			{
				StopItem(pFile);
				pFile->m_remove = true;
				continue;
			}
		}

		CQueueItem* pTopLevelItem = pItem->GetTopLevelItem();
		if (!pTopLevelItem->GetChild(1))
			// Parent will get deleted, skip it so it doesn't get deleted twice.
			skip = GetItemIndex(pTopLevelItem);
		RemoveItem(pItem);
	}

	m_waitStatusLineUpdate = false;
	UpdateStatusLinePositions();
}

bool CQueueView::StopItem(CFileItem* item)
{
	if (!item->IsActive())
		return true;

	((CServerItem*)item->GetTopLevelItem())->QueueImmediateFile(item);
	item->m_pEngineData->pEngine->Command(CCancelCommand());
	return false;
}

bool CQueueView::StopItem(CServerItem* pServerItem)
{
	for (unsigned int i = 0; i < pServerItem->GetChildrenCount(false); i++)
	{
		CQueueItem* pItem = pServerItem->GetChild(0, false);
		if (pItem->GetType() == QueueItemType_Folder)
		{
			CFolderItem* pFolder = (CFolderItem*)pItem;
			if (pFolder->m_active)
			{
				pFolder->m_remove = true;
				continue;
			}
		}
		else if (pItem->GetType() == QueueItemType_File)
		{
			CFileItem* pFile = (CFileItem*)pItem;
			if (pFile->IsActive())
			{
				StopItem(pFile);
				pFile->m_remove = true;
				continue;
			}
		}
		else
			// Unknown type, shouldn't be here.
			wxASSERT(false);

		if (pServerItem->GetChildrenCount(false) == 1)
		{
			RemoveItem(pItem);
			return true;
		}
		
		RemoveItem(pItem);
		i--;
	}

	return pServerItem->GetChildrenCount(false) == 0;
}

void CQueueView::OnFolderThreadFiles(wxCommandEvent& event)
{
	if (!m_pFolderProcessingThread)
		return;

	wxASSERT(!m_queuedFolders[1].empty());
	CFolderItem* pItem = m_queuedFolders[1].front();

	std::list<t_newEntry> entryList;
	m_pFolderProcessingThread->GetFiles(entryList);
	QueueFiles(entryList, pItem->Queued(), false, (CServerItem*)pItem->GetTopLevelItem());

	pItem->m_count += entryList.size();
	pItem->m_statusMessage = wxString::Format(_("%d files added to queue"), pItem->GetCount());
	RefreshItem(GetItemIndex(pItem));
}

void CQueueView::OnEraseBackground(wxEraseEvent& event)
{
	if (m_allowBackgroundErase)
		event.Skip();
}
