#include "FileZilla.h"
#include "queue.h"

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
	m_defaultFileExistsAction = -1;
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

void CFileItem::SetItemState(const enum ItemState itemState)
{
	m_itemState = itemState;
}

enum QueuePriority CFileItem::GetPriority() const
{
	return m_priority;
}

void CFileItem::SetActive(const bool active)
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
	if (m_size != -1)
		AddTextElement(&file, "Size", m_size.ToString());
	if (m_errorCount)
		AddTextElement(&file, "ErrorCount", m_errorCount);
	AddTextElement(&file, "Priority", m_priority);
	if (m_itemState)
		AddTextElement(&file, "ItemState", m_itemState);
	AddTextElement(&file, "TransferMode", m_transferSettings.binary ? _T("1") : _T("0"));

	pElement->InsertEndChild(file);
}

bool CFileItem::TryRemoveAll()
{
	if (!IsActive())
		return true;

	m_remove = true;
	return false;
}

CFolderItem::CFolderItem(CServerItem* parent, bool queued, const wxString& localFile)
	: CFileItem(parent, queued, true, localFile, _T(""), CServerPath(), -1)
{
}

CFolderItem::CFolderItem(CServerItem* parent, bool queued, const CServerPath& remotePath, const wxString& remoteFile)
	: CFileItem(parent, queued, false, _T(""), remoteFile, remotePath, -1)
{
}

void CFolderItem::SaveItem(TiXmlElement* pElement) const
{
	if (GetItemState() == ItemState_Complete ||
		GetItemState() == ItemState_Error)
		return;

	TiXmlElement file("Folder");

	if (m_download)
		AddTextElement(&file, "LocalFile", m_localFile);
	else
		AddTextElement(&file, "RemotePath", m_remotePath.GetSafePath());
	AddTextElement(&file, "Download", m_download ? _T("1") : _T("0"));

	pElement->InsertEndChild(file);
}

void CFolderItem::SetActive(const bool active)
{
	m_active = active;
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

void CServerItem::SetDefaultFileExistsAction(int action, const enum TransferDirection direction)
{
	for (std::vector<CQueueItem*>::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		CQueueItem *pItem = *iter;
		if (pItem->GetType() == QueueItemType_File)
		{
			CFileItem* pFileItem = ((CFileItem *)pItem);
			if (direction == upload && pFileItem->Download())
				continue;
			else if (direction == download && !pFileItem->Download())
				continue;
			pFileItem->m_defaultFileExistsAction = action;
		}
		else if (pItem->GetType() == QueueItemType_FolderScan)
		{
			if (direction == download)
				continue;
			((CFolderScanItem *)pItem)->m_defaultFileExistsAction = action;
		}
	}
}

CFileItem* CServerItem::GetIdleChild(bool immediateOnly, enum TransferDirection direction)
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

			if (direction == both)
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

			if (direction == both)
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

	if (pItem->GetType() == QueueItemType_File || pItem->GetType() == QueueItemType_Folder)
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
		m_fileList[0][pItem->GetPriority()].push_back(pItem);
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

wxLongLong CServerItem::GetTotalSize(int& filesWithUnknownSize, int& queuedFiles) const
{
	wxLongLong totalSize = 0;
	for (int i = 0; i < PRIORITY_COUNT; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			const std::list<CFileItem*>& fileList = m_fileList[j][i];
			queuedFiles += fileList.size();
			for (std::list<CFileItem*>::const_iterator iter = fileList.begin(); iter != fileList.end(); iter++)
			{
				const CFileItem* item = *iter;
				if (item->GetItemState() != ItemState_Complete)
				{
					wxLongLong size = item->GetSize();
					if (size >= 0)
						totalSize += size;
					else
						filesWithUnknownSize++;
				}
			}
		}
	}

	return totalSize;
}

CFolderScanItem::CFolderScanItem(CServerItem* parent, bool queued, bool download, const wxString& localPath, const CServerPath& remotePath)
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

	m_defaultFileExistsAction = -1;
}

bool CFolderScanItem::TryRemoveAll()
{
	if (!m_active)
		return true;

	m_remove = true;
	return false;
}
