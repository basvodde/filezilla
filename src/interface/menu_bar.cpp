#include <filezilla.h>
#include "context_control.h"
#include "bookmarks_dialog.h"
#include "listingcomparison.h"
#include "Mainfrm.h"
#include "menu_bar.h"
#include "Options.h"
#include "QueueView.h"
#include "sitemanager.h"
#include "state.h"

IMPLEMENT_DYNAMIC_CLASS(CMenuBar, wxMenuBar)

BEGIN_EVENT_TABLE(CMenuBar, wxMenuBar)
EVT_MENU(wxID_ANY, CMenuBar::OnMenuEvent)
END_EVENT_TABLE()

CMenuBar::CMenuBar()
	: CStateEventHandler(0)
{
}

CMenuBar::~CMenuBar()
{
	for (std::map<wxMenu*, std::map<int, wxMenuItem*> >::iterator menu_iter = m_hidden_items.begin(); menu_iter != m_hidden_items.end(); menu_iter++)
	{
		for (std::map<int, wxMenuItem*>::iterator iter = menu_iter->second.begin(); iter != menu_iter->second.end(); iter++)
			delete iter->second;
	}

}

CMenuBar* CMenuBar::Load(CMainFrame* pMainFrame)
{
	CMenuBar* menubar = wxDynamicCast(wxXmlResource::Get()->LoadMenuBar(_T("ID_MENUBAR")), CMenuBar);
	if (!menubar)
		return 0;

	menubar->m_pMainFrame = pMainFrame;

	
#if FZ_MANUALUPDATECHECK
	if (COptions::Get()->GetDefaultVal(DEFAULT_DISABLEUPDATECHECK))
#endif
	{
		wxMenu *helpMenu;

		wxMenuItem* pUpdateItem = menubar->FindItem(XRCID("ID_CHECKFORUPDATES"), &helpMenu);
		if (pUpdateItem)
		{
			// Get rid of separator
			unsigned int count = helpMenu->GetMenuItemCount();
			for (unsigned int i = 0; i < count - 1; i++)
			{
				if (helpMenu->FindItemByPosition(i) == pUpdateItem)
				{
					helpMenu->Delete(helpMenu->FindItemByPosition(i + 1));
					break;
				}
			}

			helpMenu->Delete(pUpdateItem);
		}
	}

	if (COptions::Get()->GetOptionVal(OPTION_DEBUG_MENU))
	{
		wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_DEBUG"));
		if (pMenu)
			menubar->Append(pMenu, _("&Debug"));
	}

	menubar->UpdateBookmarkMenu();

	menubar->Check(XRCID("ID_MENU_SERVER_VIEWHIDDEN"), COptions::Get()->GetOptionVal(OPTION_VIEW_HIDDEN_FILES) ? true : false);

	int mode = COptions::Get()->GetOptionVal(OPTION_COMPARISONMODE);
	if (mode != 1)
		menubar->Check(XRCID("ID_COMPARE_SIZE"), true);
	else
		menubar->Check(XRCID("ID_COMPARE_DATE"), true);

	menubar->Check(XRCID("ID_COMPARE_HIDEIDENTICAL"), COptions::Get()->GetOptionVal(OPTION_COMPARE_HIDEIDENTICAL) != 0);
	menubar->Check(XRCID("ID_VIEW_QUICKCONNECT"), COptions::Get()->GetOptionVal(OPTION_SHOW_QUICKCONNECT) != 0);
	menubar->Check(XRCID("ID_VIEW_MESSAGELOG"), COptions::Get()->GetOptionVal(OPTION_SHOW_MESSAGELOG) != 0);
	menubar->Check(XRCID("ID_VIEW_QUEUE"), COptions::Get()->GetOptionVal(OPTION_SHOW_QUEUE) != 0);
	menubar->Check(XRCID("ID_VIEW_LOCALTREE"), COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_LOCAL) != 0);
	menubar->Check(XRCID("ID_VIEW_REMOTETREE"), COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_REMOTE) != 0);
	menubar->Check(XRCID("ID_MENU_VIEW_FILELISTSTATUSBAR"), COptions::Get()->GetOptionVal(OPTION_FILELIST_STATUSBAR) != 0);

	switch (COptions::Get()->GetOptionVal(OPTION_ASCIIBINARY))
	{
	case 1:
		menubar->Check(XRCID("ID_MENU_TRANSFER_TYPE_ASCII"), true);
		break;
	case 2:
		menubar->Check(XRCID("ID_MENU_TRANSFER_TYPE_BINARY"), true);
		break;
	default:
		menubar->Check(XRCID("ID_MENU_TRANSFER_TYPE_AUTO"), true);
		break;
	}

	if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) == 2)
		menubar->HideItem(XRCID("ID_VIEW_MESSAGELOG"));

	menubar->UpdateMenubarState();

	pMainFrame->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CMenuBar::OnMenuEvent), 0, menubar);

	CContextManager::Get()->RegisterHandler(menubar, STATECHANGE_REMOTE_IDLE, true, true);
	CContextManager::Get()->RegisterHandler(menubar, STATECHANGE_SERVER, true, true);
	CContextManager::Get()->RegisterHandler(menubar, STATECHANGE_SYNC_BROWSE, true, true);
	CContextManager::Get()->RegisterHandler(menubar, STATECHANGE_COMPARISON, true, true);

	CContextManager::Get()->RegisterHandler(menubar, STATECHANGE_QUEUEPROCESSING, false, false);
	CContextManager::Get()->RegisterHandler(menubar, STATECHANGE_CHANGEDCONTEXT, false, false);

	menubar->RegisterOption(OPTION_ASCIIBINARY);
	menubar->RegisterOption(OPTION_PRESERVE_TIMESTAMPS);

	menubar->RegisterOption(OPTION_SHOW_MESSAGELOG);
	menubar->RegisterOption(OPTION_SHOW_QUEUE);
	menubar->RegisterOption(OPTION_SHOW_TREE_LOCAL);
	menubar->RegisterOption(OPTION_SHOW_TREE_REMOTE);
	menubar->RegisterOption(OPTION_MESSAGELOG_POSITION);
	menubar->RegisterOption(OPTION_COMPARISONMODE);
	menubar->RegisterOption(OPTION_COMPARE_HIDEIDENTICAL);
	menubar->RegisterOption(OPTION_SPEEDLIMIT_ENABLE);
	menubar->RegisterOption(OPTION_SPEEDLIMIT_INBOUND);
	menubar->RegisterOption(OPTION_SPEEDLIMIT_OUTBOUND);

	return menubar;
}

void CMenuBar::UpdateBookmarkMenu()
{
	wxMenu* pMenu;
	if (!FindItem(XRCID("ID_BOOKMARK_ADD"), &pMenu))
		return;

	// Delete old bookmarks
	for (std::map<int, wxString>::const_iterator iter = m_bookmark_menu_id_map_global.begin(); iter != m_bookmark_menu_id_map_global.end(); iter++)
	{
		pMenu->Delete(iter->first);
	}
	m_bookmark_menu_id_map_global.clear();

	for (std::map<int, wxString>::const_iterator iter = m_bookmark_menu_id_map_site.begin(); iter != m_bookmark_menu_id_map_site.end(); iter++)
	{
		pMenu->Delete(iter->first);
	}
	m_bookmark_menu_id_map_site.clear();

	// Delete the separators
	while (pMenu->GetMenuItemCount() > 2)
	{
		wxMenuItem* pSeparator = pMenu->FindItemByPosition(2);
		if (pSeparator)
			pMenu->Delete(pSeparator);
	}

	std::list<int>::iterator ids = m_bookmark_menu_ids.begin();

	// Insert global bookmarks
	std::list<wxString> global_bookmarks;
	if (CBookmarksDialog::GetBookmarks(global_bookmarks) && !global_bookmarks.empty())
	{
		pMenu->AppendSeparator();

		for (std::list<wxString>::const_iterator iter = global_bookmarks.begin(); iter != global_bookmarks.end(); iter++)
		{
			int id;
			if (ids == m_bookmark_menu_ids.end())
			{
				id = wxNewId();
				m_bookmark_menu_ids.push_back(id);
			}
			else
			{
				id = *ids;
				ids++;
			}
			pMenu->Append(id, *iter);

			m_bookmark_menu_id_map_global[id] = *iter;
		}
	}

	// Insert site-specific bookmarks
	CContextControl* pContextControl = m_pMainFrame ? m_pMainFrame->GetContextControl() : 0;
	CContextControl::_context_controls* controls = pContextControl ? pContextControl->GetCurrentControls() : 0;
	if (!controls)
		return;

	if (!controls->site_bookmarks || controls->site_bookmarks->bookmarks.empty())
		return;

	pMenu->AppendSeparator();

	for (std::list<wxString>::const_iterator iter = controls->site_bookmarks->bookmarks.begin(); iter != controls->site_bookmarks->bookmarks.end(); iter++)
	{
		int id;
		if (ids == m_bookmark_menu_ids.end())
		{
			id = wxNewId();
			m_bookmark_menu_ids.push_back(id);
		}
		else
		{
			id = *ids;
			ids++;
		}
		pMenu->Append(id, *iter);

		m_bookmark_menu_id_map_site[id] = *iter;
	}
}

void CMenuBar::ClearBookmarks()
{
	CContextControl* pContextControl = m_pMainFrame ? m_pMainFrame->GetContextControl() : 0;
	CContextControl::_context_controls* controls = pContextControl ? pContextControl->GetCurrentControls() : 0;

	if (!controls)
		controls->site_bookmarks = new CContextControl::_context_controls::_site_bookmarks;
	UpdateBookmarkMenu();
}

void CMenuBar::OnMenuEvent(wxCommandEvent& event)
{
	CState* pState = CContextManager::Get()->GetCurrentContext();

	std::map<int, wxString>::const_iterator iter = m_bookmark_menu_id_map_site.find(event.GetId());
	if (iter != m_bookmark_menu_id_map_site.end())
	{
		// We hit a site-specific bookmark
		CContextControl* pContextControl = m_pMainFrame ? m_pMainFrame->GetContextControl() : 0;
		CContextControl::_context_controls* controls = pContextControl ? pContextControl->GetCurrentControls() : 0;
		if (!controls)
			return;
		if (controls->site_bookmarks->path.empty())
			return;

		wxString name = iter->second;
		name.Replace(_T("\\"), _T("\\\\"));
		name.Replace(_T("/"), _T("\\/"));
		name = controls->site_bookmarks->path + _T("/") + name;

		CSiteManagerItemData_Site *pData = CSiteManager::GetSiteByPath(name);
		if (!pData)
			return;

		if (!pState)
			return;

		pState->SetSyncBrowse(false);
		if (!pData->m_remoteDir.IsEmpty() && pState->IsRemoteIdle())
		{
			const CServer* pServer = pState->GetServer();
			if (!pServer || *pServer != pData->m_server)
			{
				m_pMainFrame->ConnectToSite(pData);
				pData->m_localDir.clear(); // So not to set again below
			}
			else
				pState->ChangeRemoteDir(pData->m_remoteDir);
		}
		if (!pData->m_localDir.empty())
		{
			bool set = pState->SetLocalDir(pData->m_localDir);

			if (set && pData->m_sync)
			{
				wxASSERT(!pData->m_remoteDir.IsEmpty());
				pState->SetSyncBrowse(true, pData->m_remoteDir);
			}
		}

		delete pData;

		return;
	}

	std::map<int, wxString>::const_iterator iter2 = m_bookmark_menu_id_map_global.find(event.GetId());
	if (iter2 != m_bookmark_menu_id_map_global.end())
	{
		// We hit a global bookmark
		wxString local_dir;
		CServerPath remote_dir;
		bool sync;
		if (!CBookmarksDialog::GetBookmark(iter2->second, local_dir, remote_dir, sync))
			return;

		pState->SetSyncBrowse(false);
		if (!remote_dir.IsEmpty() && pState->IsRemoteIdle())
		{
			const CServer* pServer = pState->GetServer();
			if (pServer)
			{
				CServerPath current_remote_path = pState->GetRemotePath();
				if (!current_remote_path.IsEmpty() && current_remote_path.GetType() != remote_dir.GetType())
				{
					wxMessageBox(_("Selected global bookmark and current server use a different server type.\nUse site-specific bookmarks for this server."), _("Bookmark"), wxICON_EXCLAMATION, this);
					return;
				}
				pState->ChangeRemoteDir(remote_dir);
			}
		}
		if (!local_dir.empty())
		{
			bool set = pState->SetLocalDir(local_dir);

			if (set && sync)
			{
				wxASSERT(!remote_dir.IsEmpty());
				pState->SetSyncBrowse(true, remote_dir);
			}
		}

		return;
	}

	event.Skip();
}

void CMenuBar::OnStateChange(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2)
{
	switch (notification)
	{
	case STATECHANGE_CHANGEDCONTEXT:
		UpdateMenubarState();
		UpdateBookmarkMenu();
		break;
	case STATECHANGE_SERVER:
	case STATECHANGE_REMOTE_IDLE:
		UpdateMenubarState();
		break;
	case STATECHANGE_QUEUEPROCESSING:
		{
			const bool check = m_pMainFrame->GetQueue() && m_pMainFrame->GetQueue()->IsActive() != 0;
			Check(XRCID("ID_MENU_TRANSFER_PROCESSQUEUE"), check);
		}
		break;
	case STATECHANGE_SYNC_BROWSE:
		{
			bool is_sync_browse = pState && pState->GetSyncBrowse();
			Check(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), is_sync_browse);
		}
		break;
	case STATECHANGE_COMPARISON:
		{
			bool is_comparing = pState && pState->GetComparisonManager()->IsComparing();
			Check(XRCID("ID_TOOLBAR_COMPARISON"), is_comparing);
		}
		break;
	default:
		break;
	}
}

void CMenuBar::OnOptionChanged(int option)
{
	switch (option)
	{
	case OPTION_ASCIIBINARY:
		switch (COptions::Get()->GetOptionVal(OPTION_ASCIIBINARY))
		{
		default:
			Check(XRCID("ID_MENU_TRANSFER_TYPE_AUTO"), true);
			break;
		case 1:
			Check(XRCID("ID_MENU_TRANSFER_TYPE_ASCII"), true);
			break;
		case 2:
			Check(XRCID("ID_MENU_TRANSFER_TYPE_BINARY"), true);
			break;
		}
		break;
	case OPTION_PRESERVE_TIMESTAMPS:
		Check(XRCID("ID_MENU_TRANSFER_PRESERVETIMES"), COptions::Get()->GetOptionVal(OPTION_PRESERVE_TIMESTAMPS) ? true : false);
		break;
	case OPTION_SHOW_TREE_LOCAL:
		Check(XRCID("ID_VIEW_LOCALTREE"), COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_LOCAL) != 0);
		break;
	case OPTION_SHOW_TREE_REMOTE:
		Check(XRCID("ID_VIEW_REMOTETREE"), COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_REMOTE) != 0);
		break;
	case OPTION_SHOW_QUICKCONNECT:
		Check(XRCID("ID_VIEW_QUICKCONNECT"), COptions::Get()->GetOptionVal(OPTION_SHOW_QUICKCONNECT) != 0);
		break;
	case OPTION_SHOW_MESSAGELOG:
		Check(XRCID("ID_VIEW_MESSAGELOG"), COptions::Get()->GetOptionVal(OPTION_SHOW_MESSAGELOG) != 0);
		break;
	case OPTION_SHOW_QUEUE:
		Check(XRCID("ID_VIEW_QUEUE"), COptions::Get()->GetOptionVal(OPTION_SHOW_QUEUE) != 0);
		break;
	case OPTION_COMPARE_HIDEIDENTICAL:
		Check(XRCID("ID_COMPARE_HIDEIDENTICAL"), COptions::Get()->GetOptionVal(OPTION_COMPARE_HIDEIDENTICAL) != 0);
		break;
	case OPTION_COMPARISONMODE:
		if (COptions::Get()->GetOptionVal(OPTION_COMPARISONMODE) != 1)
			Check(XRCID("ID_COMPARE_SIZE"), true);
		else
			Check(XRCID("ID_COMPARE_DATE"), true);
		break;
	case OPTION_MESSAGELOG_POSITION:
		if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) == 2)
			HideItem(XRCID("ID_VIEW_MESSAGELOG"));
		else
		{
			ShowItem(XRCID("ID_VIEW_MESSAGELOG"));
			Check(XRCID("ID_VIEW_MESSAGELOG"), COptions::Get()->GetOptionVal(OPTION_SHOW_MESSAGELOG) != 0);
		}
		break;
	case OPTION_SPEEDLIMIT_ENABLE:
	case OPTION_SPEEDLIMIT_INBOUND:
	case OPTION_SPEEDLIMIT_OUTBOUND:
		{
			bool enable = COptions::Get()->GetOptionVal(OPTION_SPEEDLIMIT_ENABLE) != 0;

			int downloadLimit = COptions::Get()->GetOptionVal(OPTION_SPEEDLIMIT_INBOUND);
			int uploadLimit = COptions::Get()->GetOptionVal(OPTION_SPEEDLIMIT_OUTBOUND);

			if (!downloadLimit && !uploadLimit)
				enable = false;

			Check(XRCID("ID_MENU_TRANSFER_SPEEDLIMITS_ENABLE"), enable);
		}
		break;
	default:
		break;
	}
}

void CMenuBar::UpdateMenubarState()
{
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	const CServer* const pServer = pState->GetServer();
	const bool idle = pState->IsRemoteIdle();

	Enable(XRCID("ID_MENU_SERVER_DISCONNECT"), pServer && idle);
	Enable(XRCID("ID_CANCEL"), pServer && !idle);
	Enable(XRCID("ID_MENU_SERVER_CMD"), pServer && idle);
	Enable(XRCID("ID_MENU_FILE_COPYSITEMANAGER"), pServer != 0);
	Enable(XRCID("ID_TOOLBAR_COMPARISON"), pServer != 0);
	Enable(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pServer != 0);
	Enable(XRCID("ID_MENU_SERVER_SEARCH"), pServer && idle);

	Check(XRCID("ID_TOOLBAR_COMPARISON"), pState->GetComparisonManager()->IsComparing());
	Check(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pState->GetSyncBrowse());

	bool canReconnect;
	if (pServer || !idle)
		canReconnect = false;
	else
	{
		CServer tmp;
		canReconnect = pState->GetLastServer().GetHost() != _T("");
	}
	Enable(XRCID("ID_MENU_SERVER_RECONNECT"), canReconnect);

	wxMenuItem* pItem = FindItem(XRCID("ID_MENU_TRANSFER_TYPE"));
	if (!pServer || CServer::ProtocolHasDataTypeConcept(pServer->GetProtocol()))
	{
		pItem->Enable(true);
	}
	else
		pItem->Enable(false);
}

bool CMenuBar::ShowItem(int id)
{
	for (std::map<wxMenu*, std::map<int, wxMenuItem*> >::iterator menu_iter = m_hidden_items.begin(); menu_iter != m_hidden_items.end(); menu_iter++)
	{
		int offset = 0;

		for (std::map<int, wxMenuItem*>::iterator iter = menu_iter->second.begin(); iter != menu_iter->second.end(); iter++)
		{
			if (iter->second->GetId() != id)
			{
				offset++;
				continue;
			}

			menu_iter->first->Insert(iter->first - offset, iter->second);
			menu_iter->second.erase(iter);
			if (menu_iter->second.empty())
				m_hidden_items.erase(menu_iter);

			return true;
		}
	}
	return false;
}

bool CMenuBar::HideItem(int id)
{
	wxMenu* pMenu = 0;
	wxMenuItem* pItem = FindItem(id, &pMenu);
	if (!pItem || !pMenu)
		return false;

	size_t pos;
	pItem = pMenu->FindChildItem(id, &pos);
	if (!pItem)
		return false;

	pMenu->Remove(pItem);

	std::map<wxMenu*, std::map<int, wxMenuItem*> >::iterator menu_iter = m_hidden_items.insert(std::make_pair(pMenu, std::map<int, wxMenuItem*>())).first;

	for (std::map<int, wxMenuItem*>::iterator iter = menu_iter->second.begin(); iter != menu_iter->second.end(); iter++)
	{
		if (iter->first > (int)pos)
			break;
		
		pos++;
	}
	
	menu_iter->second[(int)pos] = pItem;

	return true;
}
