#include "FileZilla.h"
#include "queue.h"
#include "queueview_failed.h"

BEGIN_EVENT_TABLE(CQueueViewFailed, CQueueViewBase)
EVT_CONTEXT_MENU(CQueueViewFailed::OnContextMenu)
EVT_MENU(XRCID("ID_REMOVEALL"), CQueueViewFailed::OnRemoveAll)
EVT_MENU(XRCID("ID_REMOVE"), CQueueViewFailed::OnRemoveSelected)
EVT_MENU(XRCID("ID_REQUEUE"), CQueueViewFailed::OnRequeueSelected)
END_EVENT_TABLE()

CQueueViewFailed::CQueueViewFailed(CQueue* parent, int index)
	: CQueueViewBase(parent, index, _("Failed transfers"))
{
	CreateColumns(_("Reason"));
}

CQueueViewFailed::CQueueViewFailed(CQueue* parent, int index, const wxString& title)
	: CQueueViewBase(parent, index, title)
{
}

void CQueueViewFailed::OnContextMenu(wxContextMenuEvent& event)
{
	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_QUEUE_FAILED"));
	if (!pMenu)
		return;

	pMenu->Enable(XRCID("ID_REMOVE"), GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) != -1);
	pMenu->Enable(XRCID("ID_REQUEUE"), GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) != -1);

	PopupMenu(pMenu);
	delete pMenu;
}

void CQueueViewFailed::OnRemoveAll(wxCommandEvent& event)
{
	// First, clear all selections
	int item;
	while ((item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1)
		SetItemState(item, 0, wxLIST_STATE_SELECTED);

	for (std::vector<CServerItem*>::iterator iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
		delete *iter;
	m_serverList.clear();

	m_itemCount = 0;
	SetItemCount(0);
	m_fileCount = 0;
	m_folderScanCount = 0;
	
	DisplayNumberQueuedFiles();

	Refresh();
}

void CQueueViewFailed::OnRemoveSelected(wxCommandEvent& event)
{
	std::list<CQueueItem*> selectedItems;
	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		selectedItems.push_front(GetQueueItem(item));
		SetItemState(item, 0, wxLIST_STATE_SELECTED);
	}

	while (!selectedItems.empty())
	{
		CQueueItem* pItem = selectedItems.front();
		selectedItems.pop_front();

		CQueueItem* pTopLevelItem = pItem->GetTopLevelItem();
		if (!pTopLevelItem->GetChild(1))
		{
			// Parent will get deleted
			// If next selected item is parent, remove it from list
			if (!selectedItems.empty() && selectedItems.front() == pTopLevelItem)
				selectedItems.pop_front();
		}
		RemoveItem(pItem, true, false, false);
	}
	DisplayNumberQueuedFiles();
	SetItemCount(m_itemCount);
	Refresh();
}

void CQueueViewFailed::OnRequeueSelected(wxCommandEvent& event)
{
	std::list<CQueueItem*> selectedItems;
	CQueueItem* pServer = 0;
	long item = -1;
	long skipTo = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;
		SetItemState(item, 0, wxLIST_STATE_SELECTED);
		if (item < skipTo)
			continue;

		CQueueItem* pItem = GetQueueItem(item);
		if (pItem->GetType() == QueueItemType_Server)
			skipTo = item + pItem->GetChildrenCount(true) + 1;
		selectedItems.push_back(GetQueueItem(item));
	}

	if (selectedItems.empty())
		return;

	CQueueViewBase* pQueueView = m_pQueue->GetQueueView();

	while (!selectedItems.empty())
	{
		CQueueItem* pItem = selectedItems.front();
		selectedItems.pop_front();

		if (pItem->GetType() == QueueItemType_Server)
		{
			CServerItem* pOldServerItem = (CServerItem*)pItem;
			CServerItem* pServerItem = pQueueView->CreateServerItem(pOldServerItem->GetServer());			

			unsigned int childrenCount = pOldServerItem->GetChildrenCount(false);
			for (unsigned int i = 0; i < childrenCount; i++)
			{
				CFileItem* pFileItem = (CFileItem*)pItem->GetChild(i, false);
				pFileItem->m_statusMessage.Clear();
				pFileItem->SetParent(pServerItem);
				pQueueView->InsertItem(pServerItem, pFileItem);
			}
			
			m_fileCount -= childrenCount;
			m_itemCount -= childrenCount + 1;
			pOldServerItem->DetachChildren();
			delete pOldServerItem;

			std::vector<CServerItem*>::iterator iter;
			for (iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
			{
				if (*iter == pOldServerItem)
					break;
			}
			if (iter != m_serverList.end())
				m_serverList.erase(iter);
		}
		else
		{
			CFileItem* pFileItem = (CFileItem*)pItem;
			pFileItem->m_statusMessage.Clear();

			CServerItem* pOldServerItem = (CServerItem*)pItem->GetTopLevelItem();
			CServerItem* pServerItem = pQueueView->CreateServerItem(pOldServerItem->GetServer());
			RemoveItem(pItem, false, false, false);

			pItem->SetParent(pServerItem);
			pQueueView->InsertItem(pServerItem, pItem);
		}		
	}
	m_fileCountChanged = true;

	pQueueView->CommitChanges();

	DisplayNumberQueuedFiles();
	SetItemCount(m_itemCount);
	Refresh();
}
