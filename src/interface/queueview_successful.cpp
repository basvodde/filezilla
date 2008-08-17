#include "FileZilla.h"
#include "queue.h"
#include "queueview_successful.h"
#include "Options.h"

BEGIN_EVENT_TABLE(CQueueViewSuccessful, CQueueViewFailed)
EVT_CONTEXT_MENU(CQueueViewSuccessful::OnContextMenu)
END_EVENT_TABLE()

CQueueViewSuccessful::CQueueViewSuccessful(CQueue* parent, int index)
	: CQueueViewFailed(parent, index, _("Successful transfers"))
{
	CreateColumns();

	m_autoClear = COptions::Get()->GetOptionVal(OPTION_QUEUE_SUCCESSFUL_AUTOCLEAR) ? true : false;
}

void CQueueViewSuccessful::OnContextMenu(wxContextMenuEvent& event)
{
	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_QUEUE_SUCCESSFUL"));
	if (!pMenu)
		return;

#ifndef __WXMSW__
	// GetNextItem is O(n) if nothing is selected, GetSelectedItemCount() is O(1)
	const bool has_selection = GetSelectedItemCount() != 0;
#else
	const bool has_selection = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) != -1;
#endif

	pMenu->Enable(XRCID("ID_REMOVE"), has_selection);
	pMenu->Enable(XRCID("ID_REQUEUE"), has_selection);
	pMenu->Check(XRCID("ID_AUTOCLEAR"), m_autoClear);

	PopupMenu(pMenu);

	bool autoClear = pMenu->IsChecked(XRCID("ID_AUTOCLEAR"));
	if (autoClear != m_autoClear)
	{
		m_autoClear = autoClear;
		COptions::Get()->SetOption(OPTION_QUEUE_SUCCESSFUL_AUTOCLEAR, autoClear ? 1 : 0);
	}

	delete pMenu;
}
