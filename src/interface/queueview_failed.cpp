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
	std::list<long> selectedItems;
	long item = -1;
	while (true)
	{
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1)
			break;

		selectedItems.push_front(item);
		SetItemState(item, 0, wxLIST_STATE_SELECTED);
	}

	long skip = -1;
	for (std::list<long>::const_iterator iter = selectedItems.begin(); iter != selectedItems.end(); iter++)
	{
		if (*iter == skip)
			continue;

		CQueueItem* pItem = GetQueueItem(*iter);
		if (!pItem)
			continue;

		CQueueItem* pTopLevelItem = pItem->GetTopLevelItem();
		if (!pTopLevelItem->GetChild(1))
			// Parent will get deleted, skip it so it doesn't get deleted twice.
			skip = GetItemIndex(pTopLevelItem);
		RemoveItem(pItem, true, false);
	}
	DisplayNumberQueuedFiles();
	SetItemCount(m_itemCount);
	Refresh();
}

void CQueueViewFailed::OnRequeueSelected(wxCommandEvent& event)
{
}
