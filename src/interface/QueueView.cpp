#include "FileZilla.h"
#include "QueueView.h"
#include "Mainfrm.h"
#include "Options.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(CQueueView, wxListCtrl)
EVT_FZ_NOTIFICATION(wxID_ANY, CQueueView::OnEngineEvent)
END_EVENT_TABLE()

CQueueItem::CQueueItem()
{
	m_visibleOffspring = 0;
	m_expanded = false;
	m_parent = 0;
}

CQueueItem::~CQueueItem()
{
	std::vector<CQueueItem*>::iterator iter;
	for (iter = m_children.begin(); iter != m_children.end(); iter++)
		delete *iter;
}

bool CQueueItem::IsExpanded() const
{
	return m_expanded;
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
	if (m_expanded)
		m_visibleOffspring += 1 + item->m_visibleOffspring;
}

unsigned int CQueueItem::GetVisibleCount() const
{
	return m_visibleOffspring;
}

CQueueItem* CQueueItem::GetChild(unsigned int item)
{
	std::vector<CQueueItem*>::iterator iter;
	for (iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if (!item)
			return *iter;
		
		unsigned int count = (*iter)->GetVisibleCount();
		if (item > count)
		{
			item -= count + 1;
			continue;
		}
		
		return (*iter)->GetChild(item - 1);
	}
	return 0;
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

CServerItem::CServerItem(const CServer& server)
{
	m_expanded = true;
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

CFileItem* CServerItem::GetIdleChild(bool immediateOnly)
{
	int i = 0;
	for (i = (PRIORITY_COUNT - 1); i >= 0; i--)
	{
		std::list<CFileItem*>& fileList = m_fileList[1][i];
		for (std::list<CFileItem*>::iterator iter = fileList.begin(); iter != fileList.end(); iter++)
		{
			CFileItem* item = *iter;
			if (!item->IsActive())
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
			if (!item->IsActive())
				return item;
		}
	}
	
	return 0;
}

void CFileItem::SetActive(bool active)
{
	if (active && !m_active)
	{
		//TODO add status child
	}
	else if (!active && m_active)
	{
		//TODO remove status child
	}
	m_active = active;
}

CQueueView::CQueueView(wxWindow* parent, wxWindowID id, CMainFrame* pMainFrame)
	: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL | wxSUNKEN_BORDER)
{
	m_pMainFrame = pMainFrame;

	m_itemCount = 0;
	m_activeCount = 0;
	m_activeMode = 0;
	
	InsertColumn(0, _("Server / Local file"), wxLIST_FORMAT_LEFT, 150);
	InsertColumn(1, _("Direction"), wxLIST_FORMAT_CENTER, 60);
	InsertColumn(2, _("Remote file"), wxLIST_FORMAT_LEFT, 150);
	InsertColumn(3, _("Size"), wxLIST_FORMAT_RIGHT, 50);

	wxImageList* pImageList = new wxImageList(16, 16);

	wxBitmap bmp;
	extern wxString resourcePath;
	
	wxLogNull *tmp = new wxLogNull;
	
	bmp.LoadFile(resourcePath + _T("16x16/server.png"), wxBITMAP_TYPE_PNG);
	pImageList->Add(bmp);

	bmp.LoadFile(resourcePath + _T("16x16/file.png"), wxBITMAP_TYPE_PNG);
	pImageList->Add(bmp);

	bmp.LoadFile(resourcePath + _T("16x16/folderclosed.png"), wxBITMAP_TYPE_PNG);
	pImageList->Add(bmp);

	bmp.LoadFile(resourcePath + _T("16x16/folder.png"), wxBITMAP_TYPE_PNG);
	pImageList->Add(bmp);

	delete tmp;

	AssignImageList(pImageList, wxIMAGE_LIST_SMALL);

	QueueFile(false, true, _T("c:\\test.txt"), _T("test.txt"), CServerPath(_T("/")), CServer(FTP, DEFAULT, _T("127.0.0.1"), 21), -1);
}

CQueueView::~CQueueView()
{
	std::vector<CServerItem*>::iterator iter;
	for (iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
		delete *iter;
}

bool CQueueView::QueueFile(bool queueOnly, bool download, const wxString& localFile, 
						   const wxString& remoteFile, const CServerPath& remotePath, const CServer& server, wxLongLong size)
{	CServerItem* item = GetServerItem(server);
	if (!item)
	{
		item = new CServerItem(server);
		m_serverList.push_back(item);
		m_itemCount++;
	}

	CFileItem* fileItem = new CFileItem(item, queueOnly, download, localFile, remoteFile, remotePath, size);
	item->AddChild(fileItem);
	item->AddFileItemToList(fileItem);

	if (item->IsExpanded())
		m_itemCount++;

	SetItemCount(m_itemCount);

	if (m_activeMode == 2 || !queueOnly)
		while (TryStartNextTransfer());

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
						return wxLongLong().ToString();
					else
						return _T("?");
				}
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
		if (pItem->IsExpanded())
			return 3;
		else
			return 2;
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
		
		unsigned int count = (*iter)->GetVisibleCount();
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
	if (m_activeCount >= m_pMainFrame->m_pOptions->GetOptionVal(OPTION_NUMTRANSFERS))
		return false;

	// Find first idle server and assign a transfer to it.
	unsigned int engineIndex;
	for (engineIndex = 1; engineIndex < m_engineData.size(); engineIndex++)
	{
		if (m_engineData[engineIndex].active)
			continue;
	}
	if (engineIndex >= m_engineData.size())
		return false;

	t_EngineData& engineData = m_engineData[engineIndex];

	CFileItem* fileItem = 0;
	// Check all servers for the item with the highest priority
	CServerItem* serverItem = 0;
	for (std::vector<CServerItem*>::iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
	{
		CServerItem* serverItem = *iter;
		int maxCount = serverItem->GetServer().MaximumMultipleConnections();
		if (maxCount && serverItem->m_activeCount >= maxCount)
			continue;

		CFileItem* newFileItem = serverItem->GetIdleChild(m_activeMode == 1);
		if (!fileItem || newFileItem->GetPriority() > fileItem->GetPriority())
		{
			serverItem = *iter;
			fileItem = newFileItem;
			if (fileItem->GetPriority() == priority_highest)
				break;
		}
	}
	if (!fileItem)
		return false;

	// Found idle item
	fileItem->SetActive(true);
	engineData.pItem = fileItem;
	serverItem->m_activeCount++;

	const CServer oldServer = engineData.lastServer;
	engineData.lastServer = serverItem->GetServer();
	if (engineData.pEngine->IsConnected() && oldServer != serverItem->GetServer())
	{
		engineData.state = t_EngineData::disconnect;
		engineData.pEngine->Command(CDisconnectCommand());
	}
	else if (!engineData.pEngine->IsConnected())
	{
		engineData.state = t_EngineData::connect;
		engineData.pEngine->Command(CConnectCommand(engineData.lastServer));
	}
	else
	{
		engineData.state = t_EngineData::transfer;
		engineData.pEngine->Command(CFileTransferCommand(fileItem->GetLocalFile(), fileItem->GetRemotePath(), 
														 fileItem->GetRemoteFile(), fileItem->Download()));
	}

	return true;
}