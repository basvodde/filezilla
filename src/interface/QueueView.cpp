#include "FileZilla.h"
#include "QueueView.h"
#include "Mainfrm.h"
#include "Options.h"
#include "StatusView.h"
#include "statuslinectrl.h"

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
	{
		m_visibleOffspring += 1 + item->m_visibleOffspring;
		CQueueItem* parent = GetParent();
		while (parent)
		{
			parent->m_visibleOffspring += 1 + item->m_visibleOffspring;
			parent = parent->GetParent();
		}
	}
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

bool CQueueItem::RemoveChild(CQueueItem* pItem)
{
	int oldVisibleOffspring = m_visibleOffspring;
	std::vector<CQueueItem*>::iterator iter;
	bool deleted = false;
	for (iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if (*iter == pItem)
		{
			if (IsExpanded())
			{
				m_visibleOffspring -= 1;
				if (pItem->IsExpanded())
					m_visibleOffspring -= pItem->m_visibleOffspring;
			}
			delete pItem;
			m_children.erase(iter);

			deleted = true;
			break;
		}

		int visibleOffspring = (*iter)->m_visibleOffspring;
		if ((*iter)->RemoveChild(pItem))
		{
			if (IsExpanded() && (*iter)->IsExpanded())
			{
				m_visibleOffspring -= visibleOffspring - (*iter)->m_visibleOffspring;
			}
			if (!(*iter)->m_children.size())
			{
				if (IsExpanded())
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

	if (IsExpanded())
	{
		CQueueItem* parent = GetParent();
		while (parent)
		{
			parent->m_visibleOffspring -= oldVisibleOffspring - m_visibleOffspring;
			parent = parent->GetParent();
		}
	}
	
	return true;
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

		index += (*iter)->GetVisibleCount() + 1;
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
	m_expanded = true;
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
			if (item->IsActive())
				activeList.push_back(item);
			else
				m_fileList[0][i].push_back(item);
		}
		fileList = activeList;
	}
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

CQueueView::CQueueView(wxWindow* parent, wxWindowID id, CMainFrame* pMainFrame)
	: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN | wxLC_REPORT | wxLC_VIRTUAL | wxSUNKEN_BORDER)
{
	m_pMainFrame = pMainFrame;

	m_itemCount = 0;
	m_activeCount = 0;
	m_activeMode = 0;
	m_quit = false;
	m_waitStatusLineUpdate = false;
	
	InsertColumn(0, _("Server / Local file"), wxLIST_FORMAT_LEFT, 150);
	InsertColumn(1, _("Direction"), wxLIST_FORMAT_CENTER, 60);
	InsertColumn(2, _("Remote file"), wxLIST_FORMAT_LEFT, 150);
	InsertColumn(3, _("Size"), wxLIST_FORMAT_RIGHT, 70);
	InsertColumn(4, _("Priority"), wxLIST_FORMAT_LEFT, 60);
	InsertColumn(5, _("Status"), wxLIST_FORMAT_LEFT, 150);

	// Create and assign the image list for the queue
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

	//Initialize the engine data
	t_EngineData data;
	data.active = false;
	data.pEngine = 0; // TODO: Primary transfer engine data
	data.pItem = 0;
	data.pStatusLineCtrl = 0;
	data.state = t_EngineData::none;
	m_engineData.push_back(data);
	
	int engineCount = m_pMainFrame->m_pOptions->GetOptionVal(OPTION_NUMTRANSFERS);
	for (int i = 0; i < engineCount; i++)
	{
		data.pEngine = new CFileZillaEngine();
		data.pEngine->Init(this, m_pMainFrame->m_pOptions);
		m_engineData.push_back(data);
	}
}

CQueueView::~CQueueView()
{
	for (std::vector<CServerItem*>::iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
		delete *iter;

	for (unsigned int engineIndex = 1; engineIndex < m_engineData.size(); engineIndex++)
		delete m_engineData[engineIndex].pEngine;
}

bool CQueueView::QueueFile(bool queueOnly, bool download, const wxString& localFile, 
						   const wxString& remoteFile, const CServerPath& remotePath, const CServer& server, wxLongLong size)
{
	CServerItem* item = GetServerItem(server);
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
		/*case nId_listing:
			m_pState->SetRemoteDir(reinterpret_cast<CDirectoryListingNotification *>(pNotification)->GetDirectoryListing());
			delete pNotification;
			break;*/
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

				if (data.pStatusLineCtrl)
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

	if (m_activeCount >= m_pMainFrame->m_pOptions->GetOptionVal(OPTION_NUMTRANSFERS))
		return false;

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

		CFileItem* newFileItem = currentServerItem->GetIdleChild(m_activeMode == 1);
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
	engineData.active = true;
	serverItem->m_activeCount++;
	m_activeCount++;

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
	GetItemRect(lineIndex, rect);
	CStatusLineCtrl* pStatusLineCtrl = new CStatusLineCtrl(this, engineData, rect);
	m_statusLineList.push_back(pStatusLineCtrl);
	engineData.pStatusLineCtrl = pStatusLineCtrl;

	SendNextCommand(engineData);
	
	return true;
}

void CQueueView::ProcessReply(t_EngineData& engineData, COperationNotification* pNotification)
{
	// Process reply from the engine

	int replyCode = pNotification->nReplyCode;

	if (replyCode & FZ_REPLY_CANCELED)
	{
		ResetEngine(engineData, false);
		return;
	}

	// Cycle through queue states
	switch (engineData.state)
	{
	case t_EngineData::disconnect:
		engineData.state = t_EngineData::connect;
		if (engineData.pStatusLineCtrl)
			engineData.pStatusLineCtrl->SetTransferStatus(0);
		break;
	case t_EngineData::connect:
		if (replyCode == FZ_REPLY_OK)
		{
			engineData.state = t_EngineData::transfer;
			if (engineData.pStatusLineCtrl)
				engineData.pStatusLineCtrl->SetTransferStatus(0);
		}
		else if (!IncreaseErrorCount(engineData))
			return;
		break;
	case t_EngineData::transfer:
		if (replyCode == FZ_REPLY_OK)
		{
			CQueueItem* pItem = engineData.pItem;
			ResetEngine(engineData, true);
			return;
		}
		else if (!IncreaseErrorCount(engineData))
			return;
		break;
	default:
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
		delete data.pStatusLineCtrl;
		data.pStatusLineCtrl = 0;
	}
	if (data.pItem)
	{
		if (data.pItem->IsActive())
		{
			data.pItem->SetActive(false);
			m_itemCount--;
			SetItemCount(m_itemCount);
		}

		if (removeFileItem)
			RemoveItem(data.pItem);
		else
			ResetItem(data.pItem);
		data.pItem = 0;
	}
	data.active = false;
	data.state = t_EngineData::none;
	m_activeCount--;

	while (TryStartNextTransfer());
	m_waitStatusLineUpdate = false;
	UpdateStatusLinePositions();
	UpdateQueueSize();

	CheckQueueState();
}

void CQueueView::RemoveItem(CQueueItem* item)
{
	// Remove item assumes that the item has already removed from all engines

	CQueueItem* topLevelItem = item->GetTopLevelItem();

	int count = topLevelItem->GetVisibleCount();
	topLevelItem->RemoveChild(item);
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
		Refresh();
	}
	else
	{
		count -= topLevelItem->GetVisibleCount();
		m_itemCount -= count;
		SetItemCount(m_itemCount);
	}

	UpdateStatusLinePositions();
}

void CQueueView::SendNextCommand(t_EngineData& engineData)
{
	if (engineData.state == t_EngineData::disconnect)
	{
		engineData.pItem->m_statusMessage = _("Disconnecting from previous server");
		if (engineData.pEngine->Command(CDisconnectCommand()) == FZ_REPLY_WOULDBLOCK)
			return;

		engineData.state = t_EngineData::connect;
		if (engineData.pStatusLineCtrl)
			engineData.pStatusLineCtrl->SetTransferStatus(0);
	}

	if (engineData.state == t_EngineData::connect)
	{
		engineData.pItem->m_statusMessage = _("Connecting");
		int res;
		while (true)
		{
			res = engineData.pEngine->Command(CConnectCommand(engineData.lastServer));
			if (res == FZ_REPLY_WOULDBLOCK)
				return;

			if (res == FZ_REPLY_OK)
			{
				engineData.state = t_EngineData::transfer;
				if (engineData.pStatusLineCtrl)
					engineData.pStatusLineCtrl->SetTransferStatus(0);
				break;
			}

			if (!IncreaseErrorCount(engineData))
				return;
		}
	}
	
	if (engineData.state == t_EngineData::transfer)
	{
		CFileItem* fileItem = engineData.pItem;

		fileItem->m_statusMessage = _("Transferring");

		int res;
		while (true)
		{
			res = engineData.pEngine->Command(CFileTransferCommand(fileItem->GetLocalFile(), fileItem->GetRemotePath(), 
											  fileItem->GetRemoteFile(), fileItem->Download()));
			if (res == FZ_REPLY_WOULDBLOCK)
				return;

			if (res == FZ_REPLY_OK)
			{
				ResetEngine(engineData, true);
				return;
			}

			if (!IncreaseErrorCount(engineData))
				return;
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
	if (!SetActive(false))
		return false;

	for (unsigned int engineIndex = 1; engineIndex < m_engineData.size(); engineIndex++)
	{
		t_EngineData& data = m_engineData[engineIndex];
		delete data.pEngine;
		data.pEngine = 0;
	}
	m_engineData.clear();

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
	if (engineData.pItem->m_errorCount <= m_pMainFrame->m_pOptions->GetOptionVal(OPTION_TRANSFERRETRYCOUNT))
		return true;

	CQueueItem* pItem = engineData.pItem;
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

		index += (*iter)->GetVisibleCount() + 1;
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
		pCtrl->SetSize(rect);
		pCtrl->Show();
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
