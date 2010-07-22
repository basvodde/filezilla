#include <filezilla.h>
#include "sitemanager.h"
#include "Options.h"
#include "xmlfunctions.h"
#include "filezillaapp.h"
#include "ipcmutex.h"
#include "wrapengine.h"
#include "conditionaldialog.h"
#include "window_state_manager.h"
#include <wx/dnd.h>
#ifdef __WXMSW__
#include "commctrl.h"
#endif

std::map<int, struct CSiteManager::_menu_data> CSiteManager::m_idMap;

BEGIN_EVENT_TABLE(CSiteManager, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CSiteManager::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CSiteManager::OnCancel)
EVT_BUTTON(XRCID("ID_CONNECT"), CSiteManager::OnConnect)
EVT_BUTTON(XRCID("ID_NEWSITE"), CSiteManager::OnNewSite)
EVT_BUTTON(XRCID("ID_NEWFOLDER"), CSiteManager::OnNewFolder)
EVT_BUTTON(XRCID("ID_RENAME"), CSiteManager::OnRename)
EVT_BUTTON(XRCID("ID_DELETE"), CSiteManager::OnDelete)
EVT_TREE_BEGIN_LABEL_EDIT(XRCID("ID_SITETREE"), CSiteManager::OnBeginLabelEdit)
EVT_TREE_END_LABEL_EDIT(XRCID("ID_SITETREE"), CSiteManager::OnEndLabelEdit)
EVT_TREE_SEL_CHANGING(XRCID("ID_SITETREE"), CSiteManager::OnSelChanging)
EVT_TREE_SEL_CHANGED(XRCID("ID_SITETREE"), CSiteManager::OnSelChanged)
EVT_CHOICE(XRCID("ID_LOGONTYPE"), CSiteManager::OnLogontypeSelChanged)
EVT_BUTTON(XRCID("ID_BROWSE"), CSiteManager::OnRemoteDirBrowse)
EVT_TREE_ITEM_ACTIVATED(XRCID("ID_SITETREE"), CSiteManager::OnItemActivated)
EVT_CHECKBOX(XRCID("ID_LIMITMULTIPLE"), CSiteManager::OnLimitMultipleConnectionsChanged)
EVT_RADIOBUTTON(XRCID("ID_CHARSET_AUTO"), CSiteManager::OnCharsetChange)
EVT_RADIOBUTTON(XRCID("ID_CHARSET_UTF8"), CSiteManager::OnCharsetChange)
EVT_RADIOBUTTON(XRCID("ID_CHARSET_CUSTOM"), CSiteManager::OnCharsetChange)
EVT_CHOICE(XRCID("ID_PROTOCOL"), CSiteManager::OnProtocolSelChanged)
EVT_BUTTON(XRCID("ID_COPY"), CSiteManager::OnCopySite)
EVT_TREE_BEGIN_DRAG(XRCID("ID_SITETREE"), CSiteManager::OnBeginDrag)
EVT_CHAR(CSiteManager::OnChar)
EVT_TREE_ITEM_MENU(XRCID("ID_SITETREE"), CSiteManager::OnContextMenu)
EVT_MENU(XRCID("ID_EXPORT"), CSiteManager::OnExportSelected)
EVT_BUTTON(XRCID("ID_NEWBOOKMARK"), CSiteManager::OnNewBookmark)
EVT_BUTTON(XRCID("ID_BOOKMARK_BROWSE"), CSiteManager::OnBookmarkBrowse)
END_EVENT_TABLE()

class CSiteManagerDataObject : public wxDataObjectSimple
{
public:
	CSiteManagerDataObject()
		: wxDataObjectSimple(wxDataFormat(_T("FileZilla3SiteManagerObject")))
	{
	}

	// GTK doesn't like data size of 0
	virtual size_t GetDataSize() const { return 1; }

	virtual bool GetDataHere(void *buf) const { memset(buf, 0, 1); return true; }

	virtual bool SetData(size_t len, const void *buf) { return true; }
};

class CSiteManagerDropTarget : public wxDropTarget
{
public:
	CSiteManagerDropTarget(CSiteManager* pSiteManager)
		: wxDropTarget(new CSiteManagerDataObject())
	{
		m_pSiteManager = pSiteManager;
	}

	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
	{
		ClearDropHighlight();
		if (def == wxDragError ||
			def == wxDragNone ||
			def == wxDragCancel)
		{
			return def;
		}

		wxTreeItemId hit = GetHit(wxPoint(x, y));
		if (!hit)
			return wxDragNone;
		if (hit == m_pSiteManager->m_dropSource)
			return wxDragNone;

		const bool predefined = m_pSiteManager->IsPredefinedItem(hit);
		if (predefined)
			return wxDragNone;

		wxTreeCtrl *pTree = XRCCTRL(*m_pSiteManager, "ID_SITETREE", wxTreeCtrl);
		CSiteManagerItemData *pData = (CSiteManagerItemData *)pTree->GetItemData(hit);
		CSiteManagerItemData *pSourceData = (CSiteManagerItemData *)pTree->GetItemData(m_pSiteManager->m_dropSource);
		if (pData)
		{
			if (pData->m_type == CSiteManagerItemData::BOOKMARK)
				return wxDragNone;
			if (!pSourceData || pSourceData->m_type != CSiteManagerItemData::BOOKMARK)
				return wxDragNone;
		}
		else if (pSourceData && pSourceData->m_type == CSiteManagerItemData::BOOKMARK)
			return wxDragNone;

		wxTreeItemId item = hit;
		while (item != pTree->GetRootItem())
		{
			if (item == m_pSiteManager->m_dropSource)
			{
				ClearDropHighlight();
				return wxDragNone;
			}
			item = pTree->GetItemParent(item);
		}

		if (def == wxDragMove && pTree->GetItemParent(m_pSiteManager->m_dropSource) == hit)
			return wxDragNone;

		if (!m_pSiteManager->MoveItems(m_pSiteManager->m_dropSource, hit, def == wxDragCopy))
			return wxDragNone;

		return def;
	}

	virtual bool OnDrop(wxCoord x, wxCoord y)
	{
		ClearDropHighlight();

		wxTreeItemId hit = GetHit(wxPoint(x, y));
		if (!hit)
			return false;
		if (hit == m_pSiteManager->m_dropSource)
			return false;

		const bool predefined = m_pSiteManager->IsPredefinedItem(hit);
		if (predefined)
			return false;

		wxTreeCtrl *pTree = XRCCTRL(*m_pSiteManager, "ID_SITETREE", wxTreeCtrl);
		CSiteManagerItemData *pData = (CSiteManagerItemData *)pTree->GetItemData(hit);
		CSiteManagerItemData *pSourceData = (CSiteManagerItemData *)pTree->GetItemData(m_pSiteManager->m_dropSource);
		if (pData)
		{
			if (pData->m_type == CSiteManagerItemData::BOOKMARK)
				return false;
			if (!pSourceData || pSourceData->m_type != CSiteManagerItemData::BOOKMARK)
				return false;
		}
		else if (pSourceData && pSourceData->m_type == CSiteManagerItemData::BOOKMARK)
			return false;

		wxTreeItemId item = hit;
		while (item != pTree->GetRootItem())
		{
			if (item == m_pSiteManager->m_dropSource)
			{
				ClearDropHighlight();
				return false;
			}
			item = pTree->GetItemParent(item);
		}

		return true;
	}

	virtual void OnLeave()
	{
		ClearDropHighlight();
	}

	virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def)
	{
		return OnDragOver(x, y, def);
	}

	wxTreeItemId GetHit(const wxPoint& point)
	{
		int flags = 0;

		wxTreeCtrl *pTree = XRCCTRL(*m_pSiteManager, "ID_SITETREE", wxTreeCtrl);
		wxTreeItemId hit = pTree->HitTest(point, flags);

		if (flags & (wxTREE_HITTEST_ABOVE | wxTREE_HITTEST_BELOW | wxTREE_HITTEST_NOWHERE | wxTREE_HITTEST_TOLEFT | wxTREE_HITTEST_TORIGHT))
			return wxTreeItemId();

		return hit;
	}

	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
	{
		if (def == wxDragError ||
			def == wxDragNone ||
			def == wxDragCancel)
		{
			ClearDropHighlight();
			return def;
		}

		wxTreeItemId hit = GetHit(wxPoint(x, y));
		if (!hit)
		{
			ClearDropHighlight();
			return wxDragNone;
		}
		if (hit == m_pSiteManager->m_dropSource)
		{
			ClearDropHighlight();
			return wxDragNone;
		}

		const bool predefined = m_pSiteManager->IsPredefinedItem(hit);
		if (predefined)
		{
			ClearDropHighlight();
			return wxDragNone;
		}

		wxTreeCtrl *pTree = XRCCTRL(*m_pSiteManager, "ID_SITETREE", wxTreeCtrl);
		CSiteManagerItemData *pData = reinterpret_cast<CSiteManagerItemData *>(pTree->GetItemData(hit));
		CSiteManagerItemData *pSourceData = (CSiteManagerItemData *)pTree->GetItemData(m_pSiteManager->m_dropSource);
		if (pData)
		{
			if (pData->m_type == CSiteManagerItemData::BOOKMARK)
			{
				ClearDropHighlight();
				return wxDragNone;
			}
			if (!pSourceData || pSourceData->m_type != CSiteManagerItemData::BOOKMARK)
			{
				ClearDropHighlight();
				return wxDragNone;
			}
		}
		else if (pSourceData && pSourceData->m_type == CSiteManagerItemData::BOOKMARK)
		{
			ClearDropHighlight();
			return wxDragNone;
		}

		wxTreeItemId item = hit;
		while (item != pTree->GetRootItem())
		{
			if (item == m_pSiteManager->m_dropSource)
			{
				ClearDropHighlight();
				return wxDragNone;
			}
			item = pTree->GetItemParent(item);
		}

		if (def == wxDragMove && pTree->GetItemParent(m_pSiteManager->m_dropSource) == hit)
		{
			ClearDropHighlight();
			return wxDragNone;
		}

		DisplayDropHighlight(hit);

		return def;
	}

	void ClearDropHighlight()
	{
		if (m_dropHighlight == wxTreeItemId())
			return;

		wxTreeCtrl *pTree = XRCCTRL(*m_pSiteManager, "ID_SITETREE", wxTreeCtrl);
		pTree->SetItemDropHighlight(m_dropHighlight, false);
		m_dropHighlight = wxTreeItemId();
	}

	void DisplayDropHighlight(wxTreeItemId item)
	{
		ClearDropHighlight();

		wxTreeCtrl *pTree = XRCCTRL(*m_pSiteManager, "ID_SITETREE", wxTreeCtrl);
		pTree->SetItemDropHighlight(item, true);
		m_dropHighlight = item;
	}

protected:
	CSiteManager* m_pSiteManager;
	wxTreeItemId m_dropHighlight;
};

CSiteManager::CSiteManager()
{
	m_pSiteManagerMutex = 0;
	m_pWindowStateManager = 0;

	m_pNotebook_Site = 0;
	m_pNotebook_Bookmark = 0;

	m_is_deleting = false;
}

CSiteManager::~CSiteManager()
{
	delete m_pSiteManagerMutex;

	if (m_pWindowStateManager)
	{
		m_pWindowStateManager->Remember(OPTION_SITEMANAGER_POSITION);
		delete m_pWindowStateManager;
	}
}

bool CSiteManager::Create(wxWindow* parent, std::vector<_connected_site> *connected_sites, const CServer* pServer /*=0*/)
{
	m_pSiteManagerMutex = new CInterProcessMutex(MUTEX_SITEMANAGERGLOBAL, false);
	if (m_pSiteManagerMutex->TryLock() == 0)
	{
		int answer = wxMessageBox(_("The Site Manager is opened in another instance of FileZilla 3.\nDo you want to continue? Any changes made in the Site Manager won't be saved then."),
								  _("Site Manager already open"), wxYES_NO | wxICON_QUESTION);
		if (answer != wxYES)
			return false;

		delete m_pSiteManagerMutex;
		m_pSiteManagerMutex = 0;
	}
	CreateControls(parent);

	// Now create the imagelist for the site tree
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;
	wxImageList* pImageList = new wxImageList(16, 16);

	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FOLDERCLOSED"),  wxART_OTHER, wxSize(16, 16)));
	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_FOLDER"),  wxART_OTHER, wxSize(16, 16)));
	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_SERVER"),  wxART_OTHER, wxSize(16, 16)));
	pImageList->Add(wxArtProvider::GetBitmap(_T("ART_BOOKMARK"),  wxART_OTHER, wxSize(16, 16)));

	pTree->AssignImageList(pImageList);

	m_pNotebook_Site = XRCCTRL(*this, "ID_NOTEBOOK", wxNotebook);

#ifdef __WXMSW__
	// Make pages at least wide enough to fit all tabs
	HWND hWnd = (HWND)m_pNotebook_Site->GetHandle();

	int width = 4;
	for (unsigned int i = 0; i < m_pNotebook_Site->GetPageCount(); i++)
	{
		RECT tab_rect;
		TabCtrl_GetItemRect(hWnd, i, &tab_rect);
		width += tab_rect.right - tab_rect.left;
	}
	int margin = m_pNotebook_Site->GetSize().x - m_pNotebook_Site->GetPage(0)->GetSize().x;
	m_pNotebook_Site->GetPage(0)->GetSizer()->SetMinSize(wxSize(width - margin, 0));
#else
	// Make pages at least wide enough to fit all tabs
	int width = 10; // Guessed
	wxClientDC dc(m_pNotebook_Site);
	for (unsigned int i = 0; i < m_pNotebook_Site->GetPageCount(); i++)
	{
		wxCoord w, h;
		dc.GetTextExtent(m_pNotebook_Site->GetPageText(i), &w, &h);
		
		width += w;
#ifdef __WXMAC__
		width += 20; // Guessed
#else
		width += 10;
#endif
	}
	
	wxSize page_min_size = m_pNotebook_Site->GetPage(0)->GetSizer()->GetMinSize();
	if (page_min_size.x < width)
	{
		page_min_size.x = width;
		m_pNotebook_Site->GetPage(0)->GetSizer()->SetMinSize(page_min_size);
	}
#endif

	Layout();
	wxGetApp().GetWrapEngine()->WrapRecursive(this, 1.33, "Site Manager");

	wxSize minSize = GetSizer()->GetMinSize();

	wxSize size = GetSize();
	wxSize clientSize = GetClientSize();
	SetMinSize(GetSizer()->GetMinSize() + size - clientSize);
	SetClientSize(minSize);

	// Load bookmark notebook
	m_pNotebook_Bookmark = new wxNotebook(m_pNotebook_Site->GetParent(), -1);
	wxPanel* pPanel = new wxPanel;
	wxXmlResource::Get()->LoadPanel(pPanel, m_pNotebook_Bookmark, _T("ID_SITEMANAGER_BOOKMARK_PANEL"));
	m_pNotebook_Bookmark->Hide();
	m_pNotebook_Bookmark->AddPage(pPanel, _("Bookmark"));
	wxSizer *pSizer = m_pNotebook_Site->GetContainingSizer();
	pSizer->Add(m_pNotebook_Bookmark, 1, wxGROW);
	pSizer->SetItemMinSize(1, pSizer->GetItem((size_t)0)->GetMinSize().GetWidth(), -1);

	Load();

	XRCCTRL(*this, "ID_TRANSFERMODE_DEFAULT", wxRadioButton)->Update();
	XRCCTRL(*this, "ID_TRANSFERMODE_ACTIVE", wxRadioButton)->Update();
	XRCCTRL(*this, "ID_TRANSFERMODE_PASSIVE", wxRadioButton)->Update();

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		pTree->SelectItem(m_ownSites);
	SetCtrlState();

	m_pWindowStateManager = new CWindowStateManager(this);
	m_pWindowStateManager->Restore(OPTION_SITEMANAGER_POSITION);

	pTree->SetDropTarget(new CSiteManagerDropTarget(this));

#ifdef __WXGTK__
	{
		CSiteManagerItemData* data = 0;
		wxTreeItemId item = pTree->GetSelection();
		if (item.IsOk())
			data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
		if (!data)
			XRCCTRL(*this, "wxID_OK", wxButton)->SetFocus();
	}
#endif

	m_connected_sites = connected_sites;
	MarkConnectedSites();

	if (pServer)
		CopyAddServer(*pServer);

	return true;
}

void CSiteManager::MarkConnectedSites()
{
	for (int i = 0; i < (int)m_connected_sites->size(); i++)
		MarkConnectedSite(i);
}

void CSiteManager::MarkConnectedSite(int connected_site)
{
	wxString connected_site_path = (*m_connected_sites)[connected_site].old_path;

	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	if (connected_site_path.Left(1) == _T("1"))
	{
		// Default sites never change
		(*m_connected_sites)[connected_site].new_path = (*m_connected_sites)[connected_site].old_path;
		return;
	}

	if (connected_site_path.Left(1) != _T("0"))
		return;

	std::list<wxString> segments;
	if (!UnescapeSitePath(connected_site_path.Mid(1), segments))
		return;

	wxTreeItemId current = m_ownSites;
	while (!segments.empty())
	{
		wxTreeItemIdValue c;
		wxTreeItemId child = pTree->GetFirstChild(current, c);
		while (child)
		{
			if (pTree->GetItemText(child) == segments.front())
				break;

			child = pTree->GetNextChild(current, c);
		}
		if (!child)
			return;

		segments.pop_front();
		current = child;
	}

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(current));
	if (!data || data->m_type != CSiteManagerItemData::SITE)
		return;

	CSiteManagerItemData_Site *site_data = reinterpret_cast<CSiteManagerItemData_Site* >(data);
	wxASSERT(site_data->connected_item == -1);
	site_data->connected_item = connected_site;
}

void CSiteManager::CreateControls(wxWindow* parent)
{
	wxXmlResource::Get()->LoadDialog(this, parent, _T("ID_SITEMANAGER"));

	wxChoice *pProtocol = XRCCTRL(*this, "ID_PROTOCOL", wxChoice);
	pProtocol->Append(CServer::GetProtocolName(FTP));
	pProtocol->Append(CServer::GetProtocolName(SFTP));
	pProtocol->Append(CServer::GetProtocolName(FTPS));
	pProtocol->Append(CServer::GetProtocolName(FTPES));

	wxChoice *pChoice = XRCCTRL(*this, "ID_SERVERTYPE", wxChoice);
	wxASSERT(pChoice);
	for (int i = 0; i < SERVERTYPE_MAX; i++)
		pChoice->Append(CServer::GetNameFromServerType((enum ServerType)i));

	pChoice = XRCCTRL(*this, "ID_LOGONTYPE", wxChoice);
	wxASSERT(pChoice);
	for (int i = 0; i < LOGONTYPE_MAX; i++)
		pChoice->Append(CServer::GetNameFromLogonType((enum LogonType)i));
}

void CSiteManager::OnOK(wxCommandEvent& event)
{
	if (!Verify())
		return;

	UpdateItem();

	Save();

	RememberLastSelected();

	EndModal(wxID_OK);
}

void CSiteManager::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

void CSiteManager::OnConnect(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
	{
		wxBell();
		return;
	}

	if (!Verify())
	{
		wxBell();
		return;
	}

	UpdateItem();

	Save();

	RememberLastSelected();

	EndModal(wxID_YES);
}

class CSiteManagerXmlHandler
{
public:
	virtual ~CSiteManagerXmlHandler() {};

	// Adds a folder and descents
	virtual bool AddFolder(const wxString& name, bool expanded) = 0;
	virtual bool AddSite(CSiteManagerItemData_Site* data) = 0;
	virtual bool AddBookmark(const wxString& name, CSiteManagerItemData* data) = 0;

	// Go up a level
	virtual bool LevelUp() = 0; // *Ding*
};

class CSiteManagerXmlHandler_Tree : public CSiteManagerXmlHandler
{
public:
	CSiteManagerXmlHandler_Tree(wxTreeCtrl* pTree, wxTreeItemId root, const wxString& lastSelection, bool predefined)
		: m_pTree(pTree), m_item(root), m_predefined(predefined)
	{
		if (!CSiteManager::UnescapeSitePath(lastSelection, m_lastSelection))
			m_lastSelection.clear();
		m_wrong_sel_depth = 0;
		m_kiosk = COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE);
	}

	virtual ~CSiteManagerXmlHandler_Tree()
	{
		m_pTree->SortChildren(m_item);
		m_pTree->Expand(m_item);
	}

	virtual bool AddFolder(const wxString& name, bool expanded)
	{
		wxTreeItemId newItem = m_pTree->AppendItem(m_item, name, 0, 0);
		m_pTree->SetItemImage(newItem, 1, wxTreeItemIcon_Expanded);
		m_pTree->SetItemImage(newItem, 1, wxTreeItemIcon_SelectedExpanded);

		m_item = newItem;
		m_expand.push_back(expanded);

		if (!m_wrong_sel_depth && !m_lastSelection.empty())
		{
			const wxString& first = m_lastSelection.front();
			if (first == name)
			{
				m_lastSelection.pop_front();
				if (m_lastSelection.empty())
					m_pTree->SelectItem(newItem);
			}
			else
				m_wrong_sel_depth++;
		}
		else
			m_wrong_sel_depth++;

		return true;
	}

	virtual bool AddSite(CSiteManagerItemData_Site* data)
	{
		if (m_kiosk && !m_predefined &&
			data->m_server.GetLogonType() == NORMAL)
		{
			// Clear saved password
			data->m_server.SetLogonType(ASK);
			data->m_server.SetUser(data->m_server.GetUser());
		}

		const wxString name(data->m_server.GetName());

		wxTreeItemId newItem = m_pTree->AppendItem(m_item, name, 2, 2, data);

		m_item = newItem;
		m_expand.push_back(true);

		if (!m_wrong_sel_depth && !m_lastSelection.empty())
		{
			const wxString& first = m_lastSelection.front();
			if (first == name)
			{
				m_lastSelection.pop_front();
				if (m_lastSelection.empty())
					m_pTree->SelectItem(newItem);
			}
			else
				m_wrong_sel_depth++;
		}
		else
			m_wrong_sel_depth++;

		return true;
	}

	virtual bool AddBookmark(const wxString& name, CSiteManagerItemData* data)
	{
		wxTreeItemId newItem = m_pTree->AppendItem(m_item, name, 3, 3, data);

		if (!m_wrong_sel_depth && !m_lastSelection.empty())
		{
			const wxString& first = m_lastSelection.front();
			if (first == name)
			{
				m_lastSelection.clear();
				m_pTree->SelectItem(newItem);
			}
		}

		return true;
	}

	virtual bool LevelUp()
	{
		if (m_wrong_sel_depth)
			m_wrong_sel_depth--;

		if (!m_expand.empty())
		{
			const bool expand = m_expand.back();
			m_expand.pop_back();
			if (expand)
				m_pTree->Expand(m_item);
		}
		m_pTree->SortChildren(m_item);

		wxTreeItemId parent = m_pTree->GetItemParent(m_item);
		if (!parent)
			return false;

		m_item = parent;
		return true;
	}

protected:
	wxTreeCtrl* m_pTree;
	wxTreeItemId m_item;

	std::list<wxString> m_lastSelection;
	int m_wrong_sel_depth;

	std::list<bool> m_expand;

	bool m_predefined;
	int m_kiosk;
};

bool CSiteManager::Load()
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;

	pTree->DeleteAllItems();

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	// Load default sites
	bool hasDefaultSites = LoadDefaultSites();
	if (hasDefaultSites)
		m_ownSites = pTree->AppendItem(pTree->GetRootItem(), _("My Sites"), 0, 0);
	else
		m_ownSites = pTree->AddRoot(_("My Sites"), 0, 0);

	wxTreeItemId treeId = m_ownSites;
	pTree->SetItemImage(treeId, 1, wxTreeItemIcon_Expanded);
	pTree->SetItemImage(treeId, 1, wxTreeItemIcon_SelectedExpanded);

	CXmlFile file(_T("sitemanager"));
	TiXmlElement* pDocument = file.Load();
	if (!pDocument)
	{
		wxString msg = file.GetError() + _T("\n") + _("Any changes made in the Site Manager will not be saved unless you repair the file.");
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return true;

	wxString lastSelection = COptions::Get()->GetOption(OPTION_SITEMANAGER_LASTSELECTED);
	if (lastSelection[0] == '0')
	{
		if (lastSelection == _T("0"))
			pTree->SelectItem(treeId);
		else
			lastSelection = lastSelection.Mid(1);
	}
	else
		lastSelection = _T("");
	CSiteManagerXmlHandler_Tree handler(pTree, treeId, lastSelection, false);

	bool res = Load(pElement, &handler);

	pTree->SortChildren(treeId);
	pTree->Expand(treeId);
	if (!pTree->GetSelection())
		pTree->SelectItem(treeId);

	pTree->EnsureVisible(pTree->GetSelection());

	return res;
}

bool CSiteManager::Load(TiXmlElement *pElement, CSiteManagerXmlHandler* pHandler)
{
	wxASSERT(pElement);
	wxASSERT(pHandler);

	for (TiXmlElement* pChild = pElement->FirstChildElement(); pChild; pChild = pChild->NextSiblingElement())
	{
		if (!strcmp(pChild->Value(), "Folder"))
		{
			wxString name = GetTextElement_Trimmed(pChild);
			if (name.empty())
				continue;

			const bool expand = GetTextAttribute(pChild, "expanded") != _T("0");
			if (!pHandler->AddFolder(name, expand))
				return false;
			Load(pChild, pHandler);
			if (!pHandler->LevelUp())
				return false;
		}
		else if (!strcmp(pChild->Value(), "Server"))
		{
			CSiteManagerItemData_Site* data = ReadServerElement(pChild);

			if (data)
			{
				pHandler->AddSite(data);

				// Bookmarks
				for (TiXmlElement* pBookmark = pChild->FirstChildElement("Bookmark"); pBookmark; pBookmark = pBookmark->NextSiblingElement("Bookmark"))
				{
					TiXmlHandle handle(pBookmark);

					wxString name = GetTextElement_Trimmed(pBookmark, "Name");
					if (name.empty())
						continue;

					CSiteManagerItemData* data = new CSiteManagerItemData(CSiteManagerItemData::BOOKMARK);

					TiXmlText* localDir = handle.FirstChildElement("LocalDir").FirstChild().Text();
					if (localDir)
						data->m_localDir = GetTextElement(pBookmark, "LocalDir");

					TiXmlText* remoteDir = handle.FirstChildElement("RemoteDir").FirstChild().Text();
					if (remoteDir)
						data->m_remoteDir.SetSafePath(ConvLocal(remoteDir->Value()));

					if (data->m_localDir.empty() && data->m_remoteDir.IsEmpty())
					{
						delete data;
						continue;
					}

					if (!data->m_localDir.empty() && !data->m_remoteDir.IsEmpty())
						data->m_sync = GetTextElementBool(pBookmark, "SyncBrowsing", false);

					pHandler->AddBookmark(name, data);
				}

				if (!pHandler->LevelUp())
					return false;
			}
		}
	}

	return true;
}

CSiteManagerItemData_Site* CSiteManager::ReadServerElement(TiXmlElement *pElement)
{
	CServer server;
	if (!::GetServer(pElement, server))
		return 0;
	if (server.GetName().empty())
		return 0;

	CSiteManagerItemData_Site* data = new CSiteManagerItemData_Site(server);

	TiXmlHandle handle(pElement);

	TiXmlText* comments = handle.FirstChildElement("Comments").FirstChild().Text();
	if (comments)
		data->m_comments = ConvLocal(comments->Value());

	TiXmlText* localDir = handle.FirstChildElement("LocalDir").FirstChild().Text();
	if (localDir)
		data->m_localDir = ConvLocal(localDir->Value());

	TiXmlText* remoteDir = handle.FirstChildElement("RemoteDir").FirstChild().Text();
	if (remoteDir)
		data->m_remoteDir.SetSafePath(ConvLocal(remoteDir->Value()));

	if (!data->m_localDir.empty() && !data->m_remoteDir.IsEmpty())
		data->m_sync = GetTextElementBool(pElement, "SyncBrowsing", false);

	return data;
}

bool CSiteManager::Save(TiXmlElement *pElement /*=0*/, wxTreeItemId treeId /*=wxTreeItemId()*/)
{
	if (!m_pSiteManagerMutex)
		return false;

	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;

	if (!pElement || !treeId)
	{
		// We have to synchronize access to sitemanager.xml so that multiple processed don't write
		// to the same file or one is reading while the other one writes.
		CInterProcessMutex mutex(MUTEX_SITEMANAGER);

		wxFileName file(COptions::Get()->GetOption(OPTION_DEFAULT_SETTINGSDIR), _T("sitemanager.xml"));
		CXmlFile xml(file);

		TiXmlElement* pDocument = xml.Load();
		if (!pDocument)
		{
			wxString msg = xml.GetError() + _T("\n") + _("Any changes made in the Site Manager could not be saved.");
			wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

			return false;
		}

		TiXmlElement *pServers = pDocument->FirstChildElement("Servers");
		while (pServers)
		{
			pDocument->RemoveChild(pServers);
			pServers = pDocument->FirstChildElement("Servers");
		}
		pElement = pDocument->LinkEndChild(new TiXmlElement("Servers"))->ToElement();

		if (!pElement)
			return true;

		bool res = Save(pElement, m_ownSites);

		wxString error;
		if (!xml.Save(&error))
		{
			if (COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE) == 2)
				return res;
			wxString msg = wxString::Format(_("Could not write \"%s\", any changes to the Site Manager could not be saved: %s"), file.GetFullPath().c_str(), error.c_str());
			wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
		}

		return res;
	}

	wxTreeItemId child;
	wxTreeItemIdValue cookie;
	child = pTree->GetFirstChild(treeId, cookie);
	while (child.IsOk())
	{
		SaveChild(pElement, child);

		child = pTree->GetNextChild(treeId, cookie);
	}

	return false;
}

bool CSiteManager::SaveChild(TiXmlElement *pElement, wxTreeItemId child)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;

	wxString name = pTree->GetItemText(child);
	char* utf8 = ConvUTF8(name);

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(child));
	if (!data)
	{
		TiXmlNode* pNode = pElement->LinkEndChild(new TiXmlElement("Folder"));
		const bool expanded = pTree->IsExpanded(child);
		SetTextAttribute(pNode->ToElement(), "expanded", expanded ? _T("1") : _T("0"));

		pNode->LinkEndChild(new TiXmlText(utf8));

		Save(pNode->ToElement(), child);
	}
	else if (data->m_type == CSiteManagerItemData::SITE)
	{
		CSiteManagerItemData_Site *site_data = reinterpret_cast<CSiteManagerItemData_Site* >(data);
		TiXmlElement* pNode = pElement->LinkEndChild(new TiXmlElement("Server"))->ToElement();
		SetServer(pNode, site_data->m_server);

		// Save comments
		AddTextElement(pNode, "Comments", site_data->m_comments);

		// Save local dir
		AddTextElement(pNode, "LocalDir", data->m_localDir);
		
		// Save remote dir
		AddTextElement(pNode, "RemoteDir", data->m_remoteDir.GetSafePath());

		AddTextElementRaw(pNode, "SyncBrowsing", data->m_sync ? "1" : "0");

		pNode->LinkEndChild(new TiXmlText(utf8));

		Save(pNode, child);

		if (site_data->connected_item != -1)
		{
			if ((*m_connected_sites)[site_data->connected_item].server == site_data->m_server)
			{
				(*m_connected_sites)[site_data->connected_item].new_path = GetSitePath(child);
				(*m_connected_sites)[site_data->connected_item].server = site_data->m_server;
			}
		}
	}
	else
	{
		TiXmlElement* pNode = pElement->LinkEndChild(new TiXmlElement("Bookmark"))->ToElement();

		AddTextElement(pNode, "Name", name);

		// Save local dir
		AddTextElement(pNode, "LocalDir", data->m_localDir);
		
		// Save remote dir
		AddTextElement(pNode, "RemoteDir", data->m_remoteDir.GetSafePath());

		AddTextElementRaw(pNode, "SyncBrowsing", data->m_sync ? "1" : "0");
	}
	
	delete [] utf8;
	return true;
}

void CSiteManager::OnNewFolder(wxCommandEvent&event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	while (pTree->GetItemData(item))
		item = pTree->GetItemParent(item);

	if (!Verify())
		return;

	wxString name = FindFirstFreeName(item, _("New folder"));

	wxTreeItemId newItem = pTree->AppendItem(item, name, 0, 0);
	pTree->SetItemImage(newItem, 1, wxTreeItemIcon_Expanded);
	pTree->SetItemImage(newItem, 1, wxTreeItemIcon_SelectedExpanded);
	pTree->SortChildren(item);
	pTree->EnsureVisible(newItem);
	pTree->SelectItem(newItem);
	pTree->EditLabel(newItem);
}

bool CSiteManager::Verify()
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return true;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return true;

	CSiteManagerItemData* data = (CSiteManagerItemData *)pTree->GetItemData(item);
	if (!data)
		return true;

	if (data->m_type == CSiteManagerItemData::SITE)
	{
		const wxString& host = XRCCTRL(*this, "ID_HOST", wxTextCtrl)->GetValue();
		if (host == _T(""))
		{
			XRCCTRL(*this, "ID_HOST", wxTextCtrl)->SetFocus();
			wxMessageBox(_("You have to enter a hostname."), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this); 
			return false;
		}

		enum LogonType logon_type = CServer::GetLogonTypeFromName(XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->GetStringSelection());

		wxString protocolName = XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->GetStringSelection();
		enum ServerProtocol protocol = CServer::GetProtocolFromName(protocolName);
		if (protocol == SFTP &&
			logon_type == ACCOUNT)
		{
			XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetFocus();
			wxMessageBox(_("'Account' logontype not supported by selected protocol"), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
			return false;
		}

		if (COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE) != 0 &&
			!IsPredefinedItem(item) &&
			(logon_type == ACCOUNT || logon_type == NORMAL))
		{
			XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetFocus();
			wxMessageBox(_("FileZilla is running in kiosk mode.\n'Normal' and 'Account' logontypes are not available in this mode."), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
			return false;
		}

		// Set selected type
		CServer server;
		server.SetLogonType(logon_type);

		if (protocol != UNKNOWN)
			server.SetProtocol(protocol);

		wxString port = XRCCTRL(*this, "ID_PORT", wxTextCtrl)->GetValue();
		CServerPath path;
		wxString error;
		if (!server.ParseUrl(host, port, _T(""), _T(""), error, path))
		{
			XRCCTRL(*this, "ID_HOST", wxTextCtrl)->SetFocus();
			wxMessageBox(error, _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
			return false;
		}

		XRCCTRL(*this, "ID_HOST", wxTextCtrl)->SetValue(server.FormatHost(true));
		XRCCTRL(*this, "ID_PORT", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), server.GetPort()));

		protocolName = CServer::GetProtocolName(server.GetProtocol());
		if (protocolName == _T(""))
			CServer::GetProtocolName(FTP);
		XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->SetStringSelection(protocolName);

		if (XRCCTRL(*this, "ID_CHARSET_CUSTOM", wxRadioButton)->GetValue())
		{
			if (XRCCTRL(*this, "ID_ENCODING", wxTextCtrl)->GetValue() == _T(""))
			{
				XRCCTRL(*this, "ID_ENCODING", wxTextCtrl)->SetFocus();
				wxMessageBox(_("Need to specify a character encoding"), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
				return false;
			}
		}

		// Require username for non-anonymous, non-ask logon type
		const wxString user = XRCCTRL(*this, "ID_USER", wxTextCtrl)->GetValue();
		if (logon_type != ANONYMOUS &&
			logon_type != ASK &&
			logon_type != INTERACTIVE &&
			user == _T(""))
		{
			XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetFocus();
			wxMessageBox(_("You have to specify a user name"), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
			return false;
		}

		// The way TinyXML handles blanks, we can't use username of only spaces
		if (user != _T(""))
		{
			bool space_only = true;
			for (unsigned int i = 0; i < user.Len(); i++)
			{
				if (user[i] != ' ')
				{
					space_only = false;
					break;
				}
			}
			if (space_only)
			{
				XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetFocus();
				wxMessageBox(_("Username cannot be a series of spaces"), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
				return false;
			}
		}

		// Require account for account logon type
		if (logon_type == ACCOUNT &&
			XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->GetValue() == _T(""))
		{
			XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->SetFocus();
			wxMessageBox(_("You have to enter an account name"), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
			return false;
		}

		const wxString remotePathRaw = XRCCTRL(*this, "ID_REMOTEDIR", wxTextCtrl)->GetValue();
		if (remotePathRaw != _T(""))
		{
			const wxString serverType = XRCCTRL(*this, "ID_SERVERTYPE", wxChoice)->GetStringSelection();

			CServerPath remotePath;
			remotePath.SetType(CServer::GetServerTypeFromName(serverType));
			if (!remotePath.SetPath(remotePathRaw))
			{
				XRCCTRL(*this, "ID_REMOTEDIR", wxTextCtrl)->SetFocus();
				wxMessageBox(_("Default remote path cannot be parsed. Make sure it is a valid absolute path for the selected server type."), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
				return false;
			}
		}

		const wxString localPath = XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->GetValue();
		if (XRCCTRL(*this, "ID_SYNC", wxCheckBox)->GetValue())
		{
			if (remotePathRaw.empty() || localPath.empty())
			{
				XRCCTRL(*this, "ID_SYNC", wxCheckBox)->SetFocus();
				wxMessageBox(_("You need to enter both a local and a remote path to enable synchronized browsing for this site."), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
				return false;
			}
		}
	}
	else
	{
		wxTreeItemId parent = pTree->GetItemParent(item);
		CSiteManagerItemData_Site* pServer = reinterpret_cast<CSiteManagerItemData_Site* >(pTree->GetItemData(parent));
		if (!pServer)
			return false;
		
		const wxString remotePathRaw = XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxTextCtrl)->GetValue();
		if (remotePathRaw != _T(""))
		{
			CServerPath remotePath;
			remotePath.SetType(pServer->m_server.GetType());
			if (!remotePath.SetPath(remotePathRaw))
			{
				XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxTextCtrl)->SetFocus();
				wxString msg;
				if (pServer->m_server.GetType() != DEFAULT)
					msg = wxString::Format(_("Remote path cannot be parsed. Make sure it is a valid absolute path and is supported by the servertype (%s) selected on the parent site."), CServer::GetNameFromServerType(pServer->m_server.GetType()).c_str());
				else
					msg = _("Remote path cannot be parsed. Make sure it is a valid absolute path.");
				wxMessageBox(msg, _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
				return false;
			}
		}

		const wxString localPath = XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->GetValue();

		if (remotePathRaw.empty() && localPath.empty())
		{
			XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->SetFocus();
			wxMessageBox(_("You need to enter at least one path, empty bookmarks are not supported."), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
			return false;
		}

		if (XRCCTRL(*this, "ID_BOOKMARK_SYNC", wxCheckBox)->GetValue())
		{
			if (remotePathRaw.empty() || localPath.empty())
			{
				XRCCTRL(*this, "ID_BOOKMARK_SYNC", wxCheckBox)->SetFocus();
				wxMessageBox(_("You need to enter both a local and a remote path to enable synchronized browsing for this bookmark."), _("Site Manager - Invalid data"), wxICON_EXCLAMATION, this);
				return false;
			}
		}
	}

	return true;
}

void CSiteManager::OnBeginLabelEdit(wxTreeEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
	{
		event.Veto();
		return;
	}

	if (event.GetItem() != pTree->GetSelection())
	{
		if (!Verify())
		{
			event.Veto();
			return;
		}
	}

	wxTreeItemId item = event.GetItem();
	if (!item.IsOk() || item == pTree->GetRootItem() || item == m_ownSites || IsPredefinedItem(item))
	{
		event.Veto();
		return;
	}
}

void CSiteManager::OnEndLabelEdit(wxTreeEvent& event)
{
	if (event.IsEditCancelled())
		return;

	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
	{
		event.Veto();
		return;
	}

	wxTreeItemId item = event.GetItem();
	if (item != pTree->GetSelection())
	{
		if (!Verify())
		{
			event.Veto();
			return;
		}
	}

	if (!item.IsOk() || item == pTree->GetRootItem() || item == m_ownSites || IsPredefinedItem(item))
	{
		event.Veto();
		return;
	}

	wxString name = event.GetLabel();

	wxTreeItemId parent = pTree->GetItemParent(item);

	wxTreeItemIdValue cookie;
	for (wxTreeItemId child = pTree->GetFirstChild(parent, cookie); child.IsOk(); child = pTree->GetNextChild(parent, cookie))
	{
		if (child == item)
			continue;
		if (!name.CmpNoCase(pTree->GetItemText(child)))
		{
			wxMessageBox(_("Name already exists"), _("Cannot rename entry"), wxICON_EXCLAMATION, this);
			event.Veto();
			return;
		}
	}

	pTree->SortChildren(parent);
}

void CSiteManager::OnRename(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk() || item == pTree->GetRootItem() || item == m_ownSites || IsPredefinedItem(item))
		return;

	pTree->EditLabel(item);
}

void CSiteManager::OnDelete(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk() || item == pTree->GetRootItem() || item == m_ownSites || IsPredefinedItem(item))
		return;

	CConditionalDialog dlg(this, CConditionalDialog::sitemanager_confirmdelete, CConditionalDialog::yesno);
	dlg.SetTitle(_("Delete Site Manager entry"));

	dlg.AddText(_("Do you really want to delete selected entry?"));

	if (!dlg.Run())
		return;

	wxTreeItemId parent = pTree->GetItemParent(item);
	if (pTree->GetChildrenCount(parent) == 1)
		pTree->Collapse(parent);

	m_is_deleting = true;

	pTree->Delete(item);
	pTree->SelectItem(parent);

	m_is_deleting = false;
}

void CSiteManager::OnSelChanging(wxTreeEvent& event)
{
	if (m_is_deleting)
		return;

	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	if (!Verify())
		event.Veto();

	UpdateItem();
}

void CSiteManager::OnSelChanged(wxTreeEvent& event)
{
	SetCtrlState();
}

void CSiteManager::OnNewSite(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk() || IsPredefinedItem(item))
		return;

	while (pTree->GetItemData(item))
		item = pTree->GetItemParent(item);

	if (!Verify())
		return;

	CServer server;
	AddNewSite(item, server);
}

void CSiteManager::OnLogontypeSelChanged(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
		return;

	XRCCTRL(*this, "ID_USER", wxTextCtrl)->Enable(event.GetString() != _("Anonymous"));
	XRCCTRL(*this, "ID_PASS", wxTextCtrl)->Enable(event.GetString() == _("Normal") || event.GetString() == _("Account"));
	XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->Enable(event.GetString() == _("Account"));
}

bool CSiteManager::UpdateItem()
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return false;

	if (IsPredefinedItem(item))
		return true;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
		return false;

	if (data->m_type == CSiteManagerItemData::SITE)
		return UpdateServer(*(CSiteManagerItemData_Site *)data, pTree->GetItemText(item));
	else
	{
		wxTreeItemId parent = pTree->GetItemParent(item);
		CSiteManagerItemData_Site* pServer = reinterpret_cast<CSiteManagerItemData_Site* >(pTree->GetItemData(parent));
		if (!pServer)
			return false;
		return UpdateBookmark(*data, pServer->m_server);
	}
}

bool CSiteManager::UpdateBookmark(CSiteManagerItemData &bookmark, const CServer& server)
{
	bookmark.m_localDir = XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->GetValue();
	bookmark.m_remoteDir = CServerPath();
	bookmark.m_remoteDir.SetType(server.GetType());
	bookmark.m_remoteDir.SetPath(XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxTextCtrl)->GetValue());
	bookmark.m_sync = XRCCTRL(*this, "ID_BOOKMARK_SYNC", wxCheckBox)->GetValue();

	return true;
}

bool CSiteManager::UpdateServer(CSiteManagerItemData_Site &server, const wxString &name)
{
	unsigned long port;
	XRCCTRL(*this, "ID_PORT", wxTextCtrl)->GetValue().ToULong(&port);
	wxString host = XRCCTRL(*this, "ID_HOST", wxTextCtrl)->GetValue();
	// SetHost does not accept URL syntax
	if (host[0] == '[')
	{
		host.RemoveLast();
		host = host.Mid(1);
	}
	server.m_server.SetHost(host, port);

	const wxString& protocolName = XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->GetStringSelection();
	const enum ServerProtocol protocol = CServer::GetProtocolFromName(protocolName);
	if (protocol != UNKNOWN)
		server.m_server.SetProtocol(protocol);
	else
		server.m_server.SetProtocol(FTP);

	enum LogonType logon_type = CServer::GetLogonTypeFromName(XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->GetStringSelection());
	server.m_server.SetLogonType(logon_type);

	server.m_server.SetUser(XRCCTRL(*this, "ID_USER", wxTextCtrl)->GetValue(),
						   XRCCTRL(*this, "ID_PASS", wxTextCtrl)->GetValue());
	server.m_server.SetAccount(XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->GetValue());

	server.m_comments = XRCCTRL(*this, "ID_COMMENTS", wxTextCtrl)->GetValue();

	const wxString serverType = XRCCTRL(*this, "ID_SERVERTYPE", wxChoice)->GetStringSelection();
	server.m_server.SetType(CServer::GetServerTypeFromName(serverType));

	server.m_localDir = XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->GetValue();
	server.m_remoteDir = CServerPath();
	server.m_remoteDir.SetType(server.m_server.GetType());
	server.m_remoteDir.SetPath(XRCCTRL(*this, "ID_REMOTEDIR", wxTextCtrl)->GetValue());
	server.m_sync = XRCCTRL(*this, "ID_SYNC", wxCheckBox)->GetValue();

	int hours, minutes;
	hours = XRCCTRL(*this, "ID_TIMEZONE_HOURS", wxSpinCtrl)->GetValue();
	minutes = XRCCTRL(*this, "ID_TIMEZONE_MINUTES", wxSpinCtrl)->GetValue();

	server.m_server.SetTimezoneOffset(hours * 60 + minutes);

	if (XRCCTRL(*this, "ID_TRANSFERMODE_ACTIVE", wxRadioButton)->GetValue())
		server.m_server.SetPasvMode(MODE_ACTIVE);
	else if (XRCCTRL(*this, "ID_TRANSFERMODE_PASSIVE", wxRadioButton)->GetValue())
		server.m_server.SetPasvMode(MODE_PASSIVE);
	else
		server.m_server.SetPasvMode(MODE_DEFAULT);

	if (XRCCTRL(*this, "ID_LIMITMULTIPLE", wxCheckBox)->GetValue())
	{
		server.m_server.MaximumMultipleConnections(XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->GetValue());
	}
	else
		server.m_server.MaximumMultipleConnections(0);

	if (XRCCTRL(*this, "ID_CHARSET_UTF8", wxRadioButton)->GetValue())
		server.m_server.SetEncodingType(ENCODING_UTF8);
	else if (XRCCTRL(*this, "ID_CHARSET_CUSTOM", wxRadioButton)->GetValue())
	{
		wxString encoding = XRCCTRL(*this, "ID_ENCODING", wxTextCtrl)->GetValue();
		server.m_server.SetEncodingType(ENCODING_CUSTOM, encoding);
	}
	else
		server.m_server.SetEncodingType(ENCODING_AUTO);

	if (XRCCTRL(*this, "ID_BYPASSPROXY", wxCheckBox)->GetValue())
		server.m_server.SetBypassProxy(true);
	else
		server.m_server.SetBypassProxy(false);

	server.m_server.SetName(name);

	return true;
}

bool CSiteManager::GetServer(CSiteManagerItemData_Site& data)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return false;

	CSiteManagerItemData* pData = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!pData)
		return false;

	if (pData->m_type == CSiteManagerItemData::BOOKMARK)
	{
		item = pTree->GetItemParent(item);
		CSiteManagerItemData_Site* pSiteData = reinterpret_cast<CSiteManagerItemData_Site* >(pTree->GetItemData(item));

		data = *pSiteData;
		if (!pData->m_localDir.empty())
			data.m_localDir = pData->m_localDir;
		if (!pData->m_remoteDir.IsEmpty())
			data.m_remoteDir = pData->m_remoteDir;
		if (data.m_localDir.empty() || data.m_remoteDir.IsEmpty())
			data.m_sync = false;
		else
			data.m_sync = pData->m_sync;
	}
	else
		data = *(CSiteManagerItemData_Site *)pData;

	return true;
}

void CSiteManager::OnRemoteDirBrowse(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
		return;

	wxDirDialog dlg(this, _("Choose the default local directory"), XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->GetValue(), wxDD_NEW_DIR_BUTTON);
	if (dlg.ShowModal() == wxID_OK)
	{
		XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->SetValue(dlg.GetPath());
	}
}

void CSiteManager::OnItemActivated(wxTreeEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
		return;

	wxCommandEvent cmdEvent;
	OnConnect(cmdEvent);
}

void CSiteManager::OnLimitMultipleConnectionsChanged(wxCommandEvent& event)
{
	XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->Enable(event.IsChecked());
}

void CSiteManager::SetCtrlState()
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();

	const bool predefined = IsPredefinedItem(item);

#ifdef __WXGTK__
	wxWindow* pFocus = FindFocus();
#endif

	CSiteManagerItemData* data = 0;
	if (item.IsOk())
		data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
	{
		m_pNotebook_Site->Show();
		m_pNotebook_Bookmark->Hide();
		m_pNotebook_Site->GetContainingSizer()->Layout();

		// Set the control states according if it's possible to use the control
		const bool root_or_predefined = (item == pTree->GetRootItem() || item == m_ownSites || predefined);

		XRCCTRL(*this, "ID_RENAME", wxWindow)->Enable(!root_or_predefined);
		XRCCTRL(*this, "ID_DELETE", wxWindow)->Enable(!root_or_predefined);
		XRCCTRL(*this, "ID_COPY", wxWindow)->Enable(false);
		m_pNotebook_Site->Enable(false);
		XRCCTRL(*this, "ID_NEWFOLDER", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_NEWSITE", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_NEWBOOKMARK", wxWindow)->Enable(false);
		XRCCTRL(*this, "ID_CONNECT", wxWindow)->Enable(false);

		// Empty all site information
		XRCCTRL(*this, "ID_HOST", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_PORT", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->SetStringSelection(_("FTP"));
		XRCCTRL(*this, "ID_BYPASSPROXY", wxCheckBox)->SetValue(false);
		XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetStringSelection(_("Anonymous"));
		XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_COMMENTS", wxTextCtrl)->SetValue(_T(""));

		XRCCTRL(*this, "ID_SERVERTYPE", wxChoice)->SetSelection(0);
		XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_REMOTEDIR", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_SYNC", wxCheckBox)->SetValue(false);
		XRCCTRL(*this, "ID_TIMEZONE_HOURS", wxSpinCtrl)->SetValue(0);
		XRCCTRL(*this, "ID_TIMEZONE_MINUTES", wxSpinCtrl)->SetValue(0);

		XRCCTRL(*this, "ID_TRANSFERMODE_DEFAULT", wxRadioButton)->SetValue(true);
		XRCCTRL(*this, "ID_LIMITMULTIPLE", wxCheckBox)->SetValue(false);
		XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->SetValue(1);

		XRCCTRL(*this, "ID_CHARSET_AUTO", wxRadioButton)->SetValue(true);
		XRCCTRL(*this, "ID_ENCODING", wxTextCtrl)->SetValue(_T(""));
#ifdef __WXGTK__
		XRCCTRL(*this, "wxID_OK", wxButton)->SetDefault();
#endif
	}
	else if (data->m_type == CSiteManagerItemData::SITE)
	{
		m_pNotebook_Site->Show();
		m_pNotebook_Bookmark->Hide();
		m_pNotebook_Site->GetContainingSizer()->Layout();

		CSiteManagerItemData_Site* site_data = (CSiteManagerItemData_Site *)data;

		// Set the control states according if it's possible to use the control
		XRCCTRL(*this, "ID_RENAME", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_DELETE", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_COPY", wxWindow)->Enable(true);
		m_pNotebook_Site->Enable(true);
		XRCCTRL(*this, "ID_NEWFOLDER", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_NEWSITE", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_NEWBOOKMARK", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_CONNECT", wxWindow)->Enable(true);

		XRCCTRL(*this, "ID_HOST", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_HOST", wxTextCtrl)->SetValue(site_data->m_server.FormatHost(true));
		unsigned int port = site_data->m_server.GetPort();

		if (port != CServer::GetDefaultPort(site_data->m_server.GetProtocol()))
			XRCCTRL(*this, "ID_PORT", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), port));
		else
			XRCCTRL(*this, "ID_PORT", wxTextCtrl)->SetValue(_T(""));
		XRCCTRL(*this, "ID_PORT", wxWindow)->Enable(!predefined);

		const wxString& protocolName = CServer::GetProtocolName(site_data->m_server.GetProtocol());
		if (protocolName != _T(""))
			XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->SetStringSelection(protocolName);
		else
			XRCCTRL(*this, "ID_PROTOCOL", wxChoice)->SetStringSelection(CServer::GetProtocolName(FTP));
		XRCCTRL(*this, "ID_PROTOCOL", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_BYPASSPROXY", wxCheckBox)->SetValue(site_data->m_server.GetBypassProxy());

		XRCCTRL(*this, "ID_USER", wxTextCtrl)->Enable(!predefined && site_data->m_server.GetLogonType() != ANONYMOUS);
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->Enable(!predefined && (site_data->m_server.GetLogonType() == NORMAL || site_data->m_server.GetLogonType() == ACCOUNT));
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->Enable(!predefined && site_data->m_server.GetLogonType() == ACCOUNT);

		XRCCTRL(*this, "ID_LOGONTYPE", wxChoice)->SetStringSelection(CServer::GetNameFromLogonType(site_data->m_server.GetLogonType()));
		XRCCTRL(*this, "ID_LOGONTYPE", wxWindow)->Enable(!predefined);

		XRCCTRL(*this, "ID_USER", wxTextCtrl)->SetValue(site_data->m_server.GetUser());
		XRCCTRL(*this, "ID_ACCOUNT", wxTextCtrl)->SetValue(site_data->m_server.GetAccount());
		XRCCTRL(*this, "ID_PASS", wxTextCtrl)->SetValue(site_data->m_server.GetPass());
		XRCCTRL(*this, "ID_COMMENTS", wxTextCtrl)->SetValue(site_data->m_comments);
		XRCCTRL(*this, "ID_COMMENTS", wxWindow)->Enable(!predefined);

		XRCCTRL(*this, "ID_SERVERTYPE", wxChoice)->SetSelection(site_data->m_server.GetType());
		XRCCTRL(*this, "ID_SERVERTYPE", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_LOCALDIR", wxTextCtrl)->SetValue(site_data->m_localDir);
		XRCCTRL(*this, "ID_LOCALDIR", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_REMOTEDIR", wxTextCtrl)->SetValue(site_data->m_remoteDir.GetPath());
		XRCCTRL(*this, "ID_REMOTEDIR", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_SYNC", wxCheckBox)->Enable(!predefined);
		XRCCTRL(*this, "ID_SYNC", wxCheckBox)->SetValue(site_data->m_sync);
		XRCCTRL(*this, "ID_TIMEZONE_HOURS", wxSpinCtrl)->SetValue(site_data->m_server.GetTimezoneOffset() / 60);
		XRCCTRL(*this, "ID_TIMEZONE_HOURS", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_TIMEZONE_MINUTES", wxSpinCtrl)->SetValue(site_data->m_server.GetTimezoneOffset() % 60);
		XRCCTRL(*this, "ID_TIMEZONE_MINUTES", wxWindow)->Enable(!predefined);

		enum PasvMode pasvMode = site_data->m_server.GetPasvMode();
		if (pasvMode == MODE_ACTIVE)
			XRCCTRL(*this, "ID_TRANSFERMODE_ACTIVE", wxRadioButton)->SetValue(true);
		else if (pasvMode == MODE_PASSIVE)
			XRCCTRL(*this, "ID_TRANSFERMODE_PASSIVE", wxRadioButton)->SetValue(true);
		else
			XRCCTRL(*this, "ID_TRANSFERMODE_DEFAULT", wxRadioButton)->SetValue(true);
		XRCCTRL(*this, "ID_TRANSFERMODE_ACTIVE", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_TRANSFERMODE_PASSIVE", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_TRANSFERMODE_DEFAULT", wxWindow)->Enable(!predefined);

		int maxMultiple = site_data->m_server.MaximumMultipleConnections();
		XRCCTRL(*this, "ID_LIMITMULTIPLE", wxCheckBox)->SetValue(maxMultiple != 0);
		XRCCTRL(*this, "ID_LIMITMULTIPLE", wxWindow)->Enable(!predefined);
		if (maxMultiple != 0)
		{
			XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->Enable(!predefined);
			XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->SetValue(maxMultiple);
		}
		else
		{
			XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->Enable(false);
			XRCCTRL(*this, "ID_MAXMULTIPLE", wxSpinCtrl)->SetValue(1);
		}

		switch (site_data->m_server.GetEncodingType())
		{
		default:
		case ENCODING_AUTO:
			XRCCTRL(*this, "ID_CHARSET_AUTO", wxRadioButton)->SetValue(true);
			break;
		case ENCODING_UTF8:
			XRCCTRL(*this, "ID_CHARSET_UTF8", wxRadioButton)->SetValue(true);
			break;
		case ENCODING_CUSTOM:
			XRCCTRL(*this, "ID_CHARSET_CUSTOM", wxRadioButton)->SetValue(true);
			break;
		}
		XRCCTRL(*this, "ID_CHARSET_AUTO", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_CHARSET_UTF8", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_CHARSET_CUSTOM", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_ENCODING", wxTextCtrl)->Enable(!predefined && site_data->m_server.GetEncodingType() == ENCODING_CUSTOM);
		XRCCTRL(*this, "ID_ENCODING", wxTextCtrl)->SetValue(site_data->m_server.GetCustomEncoding());
#ifdef __WXGTK__
		XRCCTRL(*this, "ID_CONNECT", wxButton)->SetDefault();
#endif
	}
	else
	{
		m_pNotebook_Site->Hide();
		m_pNotebook_Bookmark->Show();
		m_pNotebook_Site->GetContainingSizer()->Layout();

		// Set the control states according if it's possible to use the control
		XRCCTRL(*this, "ID_RENAME", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_DELETE", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_COPY", wxWindow)->Enable(true);
		XRCCTRL(*this, "ID_NEWFOLDER", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_NEWSITE", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_NEWBOOKMARK", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_CONNECT", wxWindow)->Enable(true);

		XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->SetValue(data->m_localDir);
		XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxWindow)->Enable(!predefined);
		XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxTextCtrl)->SetValue(data->m_remoteDir.GetPath());
		XRCCTRL(*this, "ID_BOOKMARK_REMOTEDIR", wxWindow)->Enable(!predefined);

		XRCCTRL(*this, "ID_BOOKMARK_SYNC", wxCheckBox)->Enable(true);
		XRCCTRL(*this, "ID_BOOKMARK_SYNC", wxCheckBox)->SetValue(data->m_sync);
	}
#ifdef __WXGTK__
	if (pFocus && !pFocus->IsEnabled())
	{
		for (wxWindow* pParent = pFocus->GetParent(); pParent; pParent = pParent->GetParent())
		{
			if (pParent == this)
			{
				XRCCTRL(*this, "wxID_OK", wxButton)->SetFocus();
				break;
			}
		}
	}
#endif
}

void CSiteManager::OnCharsetChange(wxCommandEvent& event)
{
	bool checked = XRCCTRL(*this, "ID_CHARSET_CUSTOM", wxRadioButton)->GetValue();
	XRCCTRL(*this, "ID_ENCODING", wxTextCtrl)->Enable(checked);
}

void CSiteManager::OnProtocolSelChanged(wxCommandEvent& event)
{
}

void CSiteManager::OnCopySite(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data)
		return;

	if (!Verify())
		return;

	if (!UpdateItem())
		return;

	wxTreeItemId parent;
	if (IsPredefinedItem(item))
		parent = m_ownSites;
	else
		parent = pTree->GetItemParent(item);

	const wxString name = pTree->GetItemText(item);
	wxString newName = wxString::Format(_("Copy of %s"), name.c_str());
	int index = 2;
	while (true)
	{
		wxTreeItemId child;
		wxTreeItemIdValue cookie;
		child = pTree->GetFirstChild(parent, cookie);
		bool found = false;
		while (child.IsOk())
		{
			wxString name = pTree->GetItemText(child);
			int cmp = name.CmpNoCase(newName);
			if (!cmp)
			{
				found = true;
				break;
			}

			child = pTree->GetNextChild(parent, cookie);
		}
		if (!found)
			break;

		newName = wxString::Format(_("Copy (%d) of %s"), index++, name.c_str());
	}

	wxTreeItemId newItem;
	if (data->m_type == CSiteManagerItemData::SITE)
	{
		CSiteManagerItemData_Site* newData = new CSiteManagerItemData_Site(*(CSiteManagerItemData_Site *)data);
		newData->connected_item = -1;
		newItem = pTree->AppendItem(parent, newName, 2, 2, newData);

		wxTreeItemIdValue cookie;
		for (wxTreeItemId child = pTree->GetFirstChild(item, cookie); child.IsOk(); child = pTree->GetNextChild(item, cookie))
		{
			CSiteManagerItemData* pData = new CSiteManagerItemData(*(CSiteManagerItemData *)pTree->GetItemData(child));
			pTree->AppendItem(newItem, pTree->GetItemText(child), 3, 3, pData);
		}
		if (pTree->IsExpanded(item))
			pTree->Expand(newItem);
	}
	else
	{
		CSiteManagerItemData* newData = new CSiteManagerItemData(*data);
		newItem = pTree->AppendItem(parent, newName, 3, 3, newData);
	}
	pTree->SortChildren(parent);
	pTree->EnsureVisible(newItem);
	pTree->SelectItem(newItem);
	pTree->EditLabel(newItem);
}

bool CSiteManager::LoadDefaultSites()
{
	const wxString& defaultsDir = wxGetApp().GetDefaultsDir();
	if (defaultsDir == _T(""))
		return false;

	wxFileName name(defaultsDir, _T("fzdefaults.xml"));
	CXmlFile file(name);

	TiXmlElement* pDocument = file.Load();
	if (!pDocument)
		return false;

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return false;

	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return false;

	int style = pTree->GetWindowStyle();
	pTree->SetWindowStyle(style | wxTR_HIDE_ROOT);
	wxTreeItemId root = pTree->AddRoot(_T(""), 0, 0);

	m_predefinedSites = pTree->AppendItem(root, _("Predefined Sites"), 0, 0);
	pTree->SetItemImage(m_predefinedSites, 1, wxTreeItemIcon_Expanded);
	pTree->SetItemImage(m_predefinedSites, 1, wxTreeItemIcon_SelectedExpanded);

	wxString lastSelection = COptions::Get()->GetOption(OPTION_SITEMANAGER_LASTSELECTED);
	if (lastSelection[0] == '1')
	{
		if (lastSelection == _T("1"))
			pTree->SelectItem(m_predefinedSites);
		else
			lastSelection = lastSelection.Mid(1);
	}
	else
		lastSelection = _T("");
	CSiteManagerXmlHandler_Tree handler(pTree, m_predefinedSites, lastSelection, true);

	Load(pElement, &handler);

	return true;
}

bool CSiteManager::IsPredefinedItem(wxTreeItemId item)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	wxASSERT(pTree);
	if (!pTree)
		return false;

	while (item)
	{
		if (item == m_predefinedSites)
			return true;
		item = pTree->GetItemParent(item);
	}

	return false;
}

class CSiteManagerXmlHandler_Menu : public CSiteManagerXmlHandler
{
public:
	CSiteManagerXmlHandler_Menu(wxMenu* pMenu, std::map<int, struct CSiteManager::_menu_data> *idMap, bool predefined)
		: m_pMenu(pMenu), m_idMap(idMap)
	{
		m_added_site = false;

		if (predefined)
			path = _T("1");
		else
			path = _T("0");
	}

	unsigned int GetInsertIndex(wxMenu* pMenu, const wxString& name)
	{
		unsigned int i;
		for (i = 0; i < pMenu->GetMenuItemCount(); i++)
		{
			const wxMenuItem* const pItem = pMenu->FindItemByPosition(i);
			if (!pItem)
				continue;

			// Use same sorting as site tree in site manager
#ifdef __WXMSW__
			if (pItem->GetLabel().CmpNoCase(name) > 0)
				break;
#else
			if (pItem->GetLabel() > name)
				break;
#endif
		}

		return i;
	}

	virtual bool AddFolder(const wxString& name, bool)
	{
		m_parents.push_back(m_pMenu);
		m_childNames.push_back(name);
		m_paths.push_back(path);

		wxString str(name);
		str.Replace(_T("\\"), _T("\\\\"));
		str.Replace(_T("/"), _T("\\/"));
		path += _T("/") + str;

		m_pMenu = new wxMenu;

		return true;
	}

	virtual bool AddSite(CSiteManagerItemData_Site* data)
	{
		wxString newName(data->m_server.GetName());
		int i = GetInsertIndex(m_pMenu, newName);
		newName.Replace(_T("&"), _T("&&"));
		wxMenuItem* pItem = m_pMenu->Insert(i, wxID_ANY, newName);

		struct CSiteManager::_menu_data menu_data;
		menu_data.data = data;
		menu_data.path = data->m_server.GetName();
		menu_data.path.Replace(_T("\\"), _T("\\\\"));
		menu_data.path.Replace(_T("/"), _T("\\/"));
		menu_data.path = path + _T("/") + menu_data.path;

		(*m_idMap)[pItem->GetId()] = menu_data;

		m_added_site = true;

		return true;
	}

	virtual bool AddBookmark(const wxString& name, CSiteManagerItemData* data)
	{
		delete data;
		return true;
	}

	// Go up a level
	virtual bool LevelUp()
	{
		if (m_added_site)
		{
			m_added_site = false;
			return true;
		}

		if (m_parents.empty() || m_childNames.empty())
			return false;

		wxMenu* pChild = m_pMenu;
		m_pMenu = m_parents.back();
		if (pChild->GetMenuItemCount())
		{
			wxString name = m_childNames.back();
			int i = GetInsertIndex(m_pMenu, name);
			name.Replace(_T("&"), _T("&&"));

			wxMenuItem* pItem = new wxMenuItem(m_pMenu, wxID_ANY, name, _T(""), wxITEM_NORMAL, pChild);
			m_pMenu->Insert(i, pItem);
		}
		else
			delete pChild;
		m_childNames.pop_back();
		m_parents.pop_back();

		path = m_paths.back();
		m_paths.pop_back();

		return true;
	}

protected:
	wxMenu* m_pMenu;

	std::map<int, struct CSiteManager::_menu_data> *m_idMap;

	std::list<wxMenu*> m_parents;
	std::list<wxString> m_childNames;

	bool m_added_site;

	wxString path;
	std::list<wxString> m_paths;
};

wxMenu* CSiteManager::GetSitesMenu()
{
	ClearIdMap();

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	wxMenu* predefinedSites = GetSitesMenu_Predefied(m_idMap);

	CXmlFile file(_T("sitemanager"));
	TiXmlElement* pDocument = file.Load();
	if (!pDocument)
	{
		wxMessageBox(file.GetError(), _("Error loading xml file"), wxICON_ERROR);

		if (!predefinedSites)
			return predefinedSites;

		wxMenu *pMenu = new wxMenu;
		wxMenuItem* pItem = pMenu->Append(wxID_ANY, _("No sites available"));
		pItem->Enable(false);
		return pMenu;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
	{
		if (!predefinedSites)
			return predefinedSites;

		wxMenu *pMenu = new wxMenu;
		wxMenuItem* pItem = pMenu->Append(wxID_ANY, _("No sites available"));
		pItem->Enable(false);
		return pMenu;
	}

	wxMenu* pMenu = new wxMenu;
	CSiteManagerXmlHandler_Menu handler(pMenu, &m_idMap, false);

	bool res = Load(pElement, &handler);
	if (!res || !pMenu->GetMenuItemCount())
	{
		delete pMenu;
		pMenu = 0;
	}

	if (pMenu)
	{
		if (!predefinedSites)
			return pMenu;

		wxMenu* pRootMenu = new wxMenu;
		pRootMenu->AppendSubMenu(predefinedSites, _("Predefined Sites"));
		pRootMenu->AppendSubMenu(pMenu, _("My Sites"));

		return pRootMenu;
	}

	if (predefinedSites)
		return predefinedSites;

	pMenu = new wxMenu;
	wxMenuItem* pItem = pMenu->Append(wxID_ANY, _("No sites available"));
	pItem->Enable(false);
	return pMenu;
}

void CSiteManager::ClearIdMap()
{
	for (std::map<int, struct _menu_data>::iterator iter = m_idMap.begin(); iter != m_idMap.end(); iter++)
		delete iter->second.data;

	m_idMap.clear();
}

wxMenu* CSiteManager::GetSitesMenu_Predefied(std::map<int, struct _menu_data> &idMap)
{
	const wxString& defaultsDir = wxGetApp().GetDefaultsDir();
	if (defaultsDir == _T(""))
		return 0;

	wxFileName name(defaultsDir, _T("fzdefaults.xml"));
	CXmlFile file(name);

	TiXmlElement* pDocument = file.Load();
	if (!pDocument)
		return 0;

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return 0;

	wxMenu* pMenu = new wxMenu;
	CSiteManagerXmlHandler_Menu handler(pMenu, &idMap, true);

	if (!Load(pElement, &handler))
	{
		delete pMenu;
		return 0;
	}

	if (!pMenu->GetMenuItemCount())
	{
		delete pMenu;
		return 0;
	}

	return pMenu;
}

CSiteManagerItemData_Site* CSiteManager::GetSiteById(int id, wxString& path)
{
	std::map<int, struct _menu_data>::iterator iter = m_idMap.find(id);

	CSiteManagerItemData_Site *pData;
	if (iter != m_idMap.end())
	{
		pData = iter->second.data;
		iter->second.data = 0;
		path = iter->second.path;
	}
	else
		pData = 0;

	ClearIdMap();

	return pData;
}

void CSiteManager::OnBeginDrag(wxTreeEvent& event)
{
	if (!Verify())
	{
		event.Veto();
		return;
	}

	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
	{
		event.Veto();
		return;
	}

	wxTreeItemId item = event.GetItem();
	if (!item.IsOk())
	{
		event.Veto();
		return;
	}

	const bool predefined = IsPredefinedItem(item);
	const bool root = item == pTree->GetRootItem() || item == m_ownSites;
	if (root)
	{
		event.Veto();
		return;
	}

	CSiteManagerDataObject obj;

	wxDropSource source(this);
	source.SetData(obj);

	m_dropSource = item;

	source.DoDragDrop(predefined ? wxDrag_CopyOnly : wxDrag_DefaultMove);

	m_dropSource = wxTreeItemId();
}

struct itempair
{
	wxTreeItemId source;
	wxTreeItemId target;
};

bool CSiteManager::MoveItems(wxTreeItemId source, wxTreeItemId target, bool copy)
{
	if (source == target)
		return false;

	if (IsPredefinedItem(target))
		return false;

	if (IsPredefinedItem(source) && !copy)
		return false;

    wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);

	CSiteManagerItemData *pTargetData = (CSiteManagerItemData *)pTree->GetItemData(target);
	CSiteManagerItemData *pSourceData = (CSiteManagerItemData *)pTree->GetItemData(source);
	if (pTargetData)
	{
		if (pTargetData->m_type == CSiteManagerItemData::BOOKMARK)
			return false;
		if (!pSourceData || pSourceData->m_type != CSiteManagerItemData::BOOKMARK)
			return false;
	}
	else if (pSourceData && pSourceData->m_type == CSiteManagerItemData::BOOKMARK)
		return false;

	wxTreeItemId item = target;
	while (item != pTree->GetRootItem())
	{
		if (item == source)
			return false;
		item = pTree->GetItemParent(item);
	}

	if (!copy && pTree->GetItemParent(source) == target)
		return false;

	wxString sourceName = pTree->GetItemText(source);

	wxTreeItemId child;
	wxTreeItemIdValue cookie;
	child = pTree->GetFirstChild(target, cookie);

	while (child.IsOk())
	{
		wxString childName = pTree->GetItemText(child);

		if (!sourceName.CmpNoCase(childName))
		{
			wxMessageBox(_("An item with the same name as the dragged item already exists at the target location."), _("Failed to copy or move sites"), wxICON_INFORMATION);
			return false;
		}

		child = pTree->GetNextChild(target, cookie);
	}

	std::list<itempair> work;
	itempair pair;
	pair.source = source;
	pair.target = target;
	work.push_back(pair);

	std::list<wxTreeItemId> expand;

	while (!work.empty())
	{
		itempair pair = work.front();
		work.pop_front();

		wxString name = pTree->GetItemText(pair.source);

		CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(pair.source));

		wxTreeItemId newItem = pTree->AppendItem(pair.target, name, data ? 2 : 0);
		if (!data)
		{
			pTree->SetItemImage(newItem, 1, wxTreeItemIcon_Expanded);
			pTree->SetItemImage(newItem, 1, wxTreeItemIcon_SelectedExpanded);

			if (pTree->IsExpanded(pair.source))
				expand.push_back(newItem);
		}
		else if (data->m_type == CSiteManagerItemData::SITE)
		{
			CSiteManagerItemData_Site* newData = new CSiteManagerItemData_Site(*(CSiteManagerItemData_Site *)data);
			newData->connected_item = -1;
			pTree->SetItemData(newItem, newData);
		}
		else
		{
			pTree->SetItemImage(newItem, 3, wxTreeItemIcon_Normal);
			pTree->SetItemImage(newItem, 3, wxTreeItemIcon_Selected);

			CSiteManagerItemData* newData = new CSiteManagerItemData(*data);
			pTree->SetItemData(newItem, newData);
		}

		wxTreeItemId child;
		wxTreeItemIdValue cookie;
		child = pTree->GetFirstChild(pair.source, cookie);
		while (child.IsOk())
		{
			itempair newPair;
			newPair.source = child;
			newPair.target = newItem;
			work.push_back(newPair);

			child = pTree->GetNextChild(pair.source, cookie);
		}

		pTree->SortChildren(pair.target);
	}

	if (!copy)
	{
		wxTreeItemId parent = pTree->GetItemParent(source);
		if (pTree->GetChildrenCount(parent) == 1)
			pTree->Collapse(parent);

		pTree->Delete(source);
	}

	for (std::list<wxTreeItemId>::iterator iter = expand.begin(); iter != expand.end(); iter++)
		pTree->Expand(*iter);

	pTree->Expand(target);

	return true;
}

void CSiteManager::OnChar(wxKeyEvent& event)
{
	if (event.GetKeyCode() != WXK_F2)
	{
		event.Skip();
		return;
	}

	wxCommandEvent cmdEvent;
	OnRename(cmdEvent);
}

void CSiteManager::CopyAddServer(const CServer& server)
{
	if (!Verify())
		return;

	AddNewSite(m_ownSites, server, true);
}

wxString CSiteManager::FindFirstFreeName(const wxTreeItemId &parent, const wxString& name)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	wxASSERT(pTree);

	wxString newName = name;
	int index = 2;
	while (true)
	{
		wxTreeItemId child;
		wxTreeItemIdValue cookie;
		child = pTree->GetFirstChild(parent, cookie);
		bool found = false;
		while (child.IsOk())
		{
			wxString name = pTree->GetItemText(child);
			int cmp = name.CmpNoCase(newName);
			if (!cmp)
			{
				found = true;
				break;
			}

			child = pTree->GetNextChild(parent, cookie);
		}
		if (!found)
			break;

		newName = name + wxString::Format(_T(" %d"), index++);
	}

	return newName;
}

void CSiteManager::AddNewSite(wxTreeItemId parent, const CServer& server, bool connected /*=false*/)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxString name = FindFirstFreeName(parent, _("New site"));

	CSiteManagerItemData_Site* pData = new CSiteManagerItemData_Site(server);
	if (connected)
		pData->connected_item = 0;

	wxTreeItemId newItem = pTree->AppendItem(parent, name, 2, 2, pData);
	pTree->SortChildren(parent);
	pTree->EnsureVisible(newItem);
	pTree->SelectItem(newItem);
#ifdef __WXMAC__
	// Need to trigger dirty processing of generic tree control.
	// Else edit control will be hidden behind item
	pTree->OnInternalIdle();
#endif
	pTree->EditLabel(newItem);
}

void CSiteManager::AddNewBookmark(wxTreeItemId parent)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxString name = FindFirstFreeName(parent, _("New bookmark"));

	wxTreeItemId newItem = pTree->AppendItem(parent, name, 3, 3, new CSiteManagerItemData(CSiteManagerItemData::BOOKMARK));
	pTree->SortChildren(parent);
	pTree->EnsureVisible(newItem);
	pTree->SelectItem(newItem);
	pTree->EditLabel(newItem);
}

void CSiteManager::RememberLastSelected()
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	wxString path;
	while (item != pTree->GetRootItem() && item != m_ownSites && item != m_predefinedSites)
	{
		wxString name = pTree->GetItemText(item);
		name.Replace(_T("\\"), _T("\\\\"));
		name.Replace(_T("/"), _T("\\/"));

		path = name + _T("/") + path;

		item = pTree->GetItemParent(item);
	}

	if (item == m_predefinedSites)
		path = _T("1") + path;
	else
		path = _T("0") + path;

	COptions::Get()->SetOption(OPTION_SITEMANAGER_LASTSELECTED, path);
}

bool CSiteManager::UnescapeSitePath(wxString path, std::list<wxString>& result)
{
	result.clear();

	wxString name;
	const wxChar *p = path;

	// Undo escapement
	bool lastBackslash = false;
	while (*p)
	{
		const wxChar& c = *p;
		if (c == '\\')
		{
			if (lastBackslash)
			{
				name += _T("\\");
				lastBackslash = false;
			}
			else
				lastBackslash = true;
		}
		else if (c == '/')
		{
			if (lastBackslash)
			{
				name += _T("/");
				lastBackslash = 0;
			}
			else
			{
				if (!name.IsEmpty())
					result.push_back(name);
				name.clear();
			}
		}
		else
			name += *p;
		p++;
	}
	if (lastBackslash)
		return false;
	if (name != _T(""))
		result.push_back(name);

	return !result.empty();
}

CSiteManagerItemData_Site* CSiteManager::GetSiteByPath(wxString sitePath)
{
	wxChar c = sitePath[0];
	if (c != '0' && c != '1')
	{
		wxMessageBox(_("Site path has to begin with 0 or 1."), _("Invalid site path"));
		return 0;
	}

	sitePath = sitePath.Mid(1);

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file;
	TiXmlElement* pDocument = 0;

	if (c == '0')
		pDocument = file.Load(_T("sitemanager"));
	else
	{
		const wxString& defaultsDir = wxGetApp().GetDefaultsDir();
		if (defaultsDir == _T(""))
		{
			wxMessageBox(_("Site does not exist."), _("Invalid site path"));
			return 0;
		}
		wxFileName name(defaultsDir, _T("fzdefaults.xml"));
		pDocument = file.Load(name);
	}

	if (!pDocument)
	{
		wxMessageBox(file.GetError(), _("Error loading xml file"), wxICON_ERROR);

		return 0;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
	{
		wxMessageBox(_("Site does not exist."), _("Invalid site path"));
		return 0;
	}

	std::list<wxString> segments;
	if (!CSiteManager::UnescapeSitePath(sitePath, segments))
	{
		wxMessageBox(_("Site path is malformed."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement* pChild = GetElementByPath(pElement, segments);
	if (!pChild)
	{
		wxMessageBox(_("Site does not exist."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement* pBookmark;
	if (!strcmp(pChild->Value(), "Bookmark"))
	{
		pBookmark = pChild;
		pChild = pChild->Parent()->ToElement();
	}
	else
		pBookmark = 0;

	CSiteManagerItemData_Site* data = ReadServerElement(pChild);

	if (!data)
	{
		wxMessageBox(_("Could not read server item."), _("Invalid site path"));
		return 0;
	}

	if (pBookmark)
	{
		TiXmlHandle handle(pBookmark);

		wxString localPath;
		CServerPath remotePath;
		TiXmlText* localDir = handle.FirstChildElement("LocalDir").FirstChild().Text();
		if (localDir)
			localPath = ConvLocal(localDir->Value());
		TiXmlText* remoteDir = handle.FirstChildElement("RemoteDir").FirstChild().Text();
		if (remoteDir)
			remotePath.SetSafePath(ConvLocal(remoteDir->Value()));
		if (!localPath.empty() && !remotePath.IsEmpty())
		{
			data->m_sync = GetTextElementBool(pBookmark, "SyncBrowsing", false);
		}
		else
			data->m_sync = false;

		data->m_localDir = localPath;
		data->m_remoteDir = remotePath;
	}

	return data;
}

void CSiteManager::OnContextMenu(wxTreeEvent& event)
{
	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_SITEMANAGER"));
	if (!pMenu)
		return;

	m_contextMenuItem = event.GetItem();

	PopupMenu(pMenu);
	delete pMenu;
}

void CSiteManager::OnExportSelected(wxCommandEvent& event)
{
	wxFileDialog dlg(this, _("Select file for exported sites"), _T(""), 
					_T("sites.xml"), _T("XML files (*.xml)|*.xml"), 
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dlg.ShowModal() != wxID_OK)
		return;

	wxFileName fn(dlg.GetPath());
	CXmlFile xml(fn);

	TiXmlElement* exportRoot = xml.CreateEmpty();

	TiXmlElement* pServers = exportRoot->LinkEndChild(new TiXmlElement("Servers"))->ToElement();
	SaveChild(pServers, m_contextMenuItem);

	wxString error;
	if (!xml.Save(&error))
	{
		wxString msg = wxString::Format(_("Could not write \"%s\", the selected sites could not be exported: %s"), fn.GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
	}
}

void CSiteManager::OnBookmarkBrowse(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return;

	CSiteManagerItemData* data = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!data || data->m_type != CSiteManagerItemData::BOOKMARK)
		return;

	wxDirDialog dlg(this, _("Choose the local directory"), XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->GetValue(), wxDD_NEW_DIR_BUTTON);
	if (dlg.ShowModal() != wxID_OK)
		return;
	
    XRCCTRL(*this, "ID_BOOKMARK_LOCALDIR", wxTextCtrl)->SetValue(dlg.GetPath());
}

void CSiteManager::OnNewBookmark(wxCommandEvent& event)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return;

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk() || IsPredefinedItem(item))
		return;

	CSiteManagerItemData *pData = (CSiteManagerItemData *)pTree->GetItemData(item);
	if (!pData)
		return;
	if (pData->m_type == CSiteManagerItemData::BOOKMARK)
		item = pTree->GetItemParent(item);

	if (!Verify())
		return;

	AddNewBookmark(item);
}

bool CSiteManager::GetBookmarks(wxString sitePath, std::list<wxString> &bookmarks)
{
	wxChar c = sitePath[0];
	if (c != '0' && c != '1')
		return false;

	sitePath = sitePath.Mid(1);

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file;
	TiXmlElement* pDocument = 0;

	if (c == '0')
		pDocument = file.Load(_T("sitemanager"));
	else
	{
		const wxString& defaultsDir = wxGetApp().GetDefaultsDir();
		if (defaultsDir == _T(""))
			return false;
		pDocument = file.Load(wxFileName(defaultsDir, _T("fzdefaults.xml")));
	}

	if (!pDocument)
	{
		wxMessageBox(file.GetError(), _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return false;

	std::list<wxString> segments;
	if (!CSiteManager::UnescapeSitePath(sitePath, segments))
	{
		wxMessageBox(_("Site path is malformed."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement* pChild = GetElementByPath(pElement, segments);
	if (!pChild || strcmp(pChild->Value(), "Server"))
		return 0;

	// Bookmarks
	for (TiXmlElement* pBookmark = pChild->FirstChildElement("Bookmark"); pBookmark; pBookmark = pBookmark->NextSiblingElement("Bookmark"))
	{
		TiXmlHandle handle(pBookmark);

		wxString name = GetTextElement_Trimmed(pBookmark, "Name");
		if (name.empty())
			continue;

		wxString localPath;
		CServerPath remotePath;
		TiXmlText* localDir = handle.FirstChildElement("LocalDir").FirstChild().Text();
		if (localDir)
			localPath = ConvLocal(localDir->Value());
		TiXmlText* remoteDir = handle.FirstChildElement("RemoteDir").FirstChild().Text();
		if (remoteDir)
			remotePath.SetSafePath(ConvLocal(remoteDir->Value()));

		if (localPath.empty() && remotePath.IsEmpty())
			continue;

		bookmarks.push_back(name);		
	}

	return true;
}

wxString CSiteManager::GetSitePath(wxTreeItemId item)
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return _T("");

	CSiteManagerItemData* pData = reinterpret_cast<CSiteManagerItemData* >(pTree->GetItemData(item));
	if (!pData)
		return _T("");

	if (pData->m_type == CSiteManagerItemData::BOOKMARK)
		item = pTree->GetItemParent(item);

	wxString path;
	while (item)
	{
		if (item == m_predefinedSites)
			return _T("1/") + path;
		else if (item == m_ownSites)
			return _T("0/") + path;
		wxString segment = pTree->GetItemText(item);
		segment.Replace(_T("\\"), _T("\\\\"));
		segment.Replace(_T("/"), _T("\\/"));
		if (path != _T(""))
			path = segment + _T("/") + path;
		else
			path = segment;

		item = pTree->GetItemParent(item);
	}

	return _T("0/") + path;
}

wxString CSiteManager::GetSitePath()
{
	wxTreeCtrl *pTree = XRCCTRL(*this, "ID_SITETREE", wxTreeCtrl);
	if (!pTree)
		return _T("");

	wxTreeItemId item = pTree->GetSelection();
	if (!item.IsOk())
		return _T("");

	return GetSitePath(item);
}

wxString CSiteManager::AddServer(CServer server)
{
	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file;
	TiXmlElement* pDocument = file.Load(_T("sitemanager"));

	if (!pDocument)
	{
		wxString msg = file.GetError() + _T("\n") + _("The server could not be added.");
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return _T("");
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return _T("");

	std::list<wxString> names;
	for (TiXmlElement* pChild = pElement->FirstChildElement("Server"); pChild; pChild = pChild->NextSiblingElement("Server"))
	{
		wxString name = GetTextElement(pChild, "Name");
		if (name.empty())
			continue;

		names.push_back(name);
	}

	wxString name = _("New site");
	int i = 1;

	while (true)
	{
		std::list<wxString>::const_iterator iter;
		for (iter = names.begin(); iter != names.end(); iter++)
		{
			if (*iter == name)
				break;
		}
		if (iter == names.end())
			break;

		name = _("New site") + wxString::Format(_T(" %d"), ++i);
	}

	server.SetName(name);

	TiXmlElement* pServer = pElement->LinkEndChild(new TiXmlElement("Server"))->ToElement();
	SetServer(pServer, server);

	char* utf8 = ConvUTF8(name);
	if (utf8)
	{
		pServer->LinkEndChild(new TiXmlText(utf8));
		delete [] utf8;
	}

	wxString error;
	if (!file.Save(&error))
	{
		if (COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE) == 2)
			return _T("");

		wxString msg = wxString::Format(_("Could not write \"%s\", any changes to the Site Manager could not be saved: %s"), file.GetFileName().GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
		return _T("");
	}
	
	name.Replace(_T("\\"), _T("\\\\"));
	name.Replace(_T("/"), _T("\\/"));

	return _T("0/") + name;
}

TiXmlElement* CSiteManager::GetElementByPath(TiXmlElement* pNode, std::list<wxString> &segments)
{
	while (!segments.empty())
	{
		const wxString segment = segments.front();
		segments.pop_front();

		TiXmlElement* pChild;
		for (pChild = pNode->FirstChildElement(); pChild; pChild = pChild->NextSiblingElement())
		{
			const char* value = pChild->Value();
			if (strcmp(value, "Server") && strcmp(value, "Folder") && strcmp(value, "Bookmark"))
				continue;

			wxString name = GetTextElement_Trimmed(pChild, "Name");
			if (name.empty())
				name = GetTextElement_Trimmed(pChild);
			if (name.empty())
				continue;

			if (name == segment)
				break;
		}
		if (!pChild)
			return 0;

		pNode = pChild;
		continue;
	}

	return pNode;
}

bool CSiteManager::AddBookmark(wxString sitePath, const wxString& name, const wxString &local_dir, const CServerPath &remote_dir, bool sync)
{
	if (local_dir.empty() && remote_dir.IsEmpty())
		return false;

	if (sitePath[0] != '0')
		return false;

	sitePath = sitePath.Mid(1);

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file;
	TiXmlElement* pDocument = 0;

	pDocument = file.Load(_T("sitemanager"));

	if (!pDocument)
	{
		wxString msg = file.GetError() + _T("\n") + _("The bookmark could not be added.");
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return false;

	std::list<wxString> segments;
	if (!CSiteManager::UnescapeSitePath(sitePath, segments))
	{
		wxMessageBox(_("Site path is malformed."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement* pChild = GetElementByPath(pElement, segments);
	if (!pChild || strcmp(pChild->Value(), "Server"))
	{
		wxMessageBox(_("Site does not exist."), _("Invalid site path"));
		return 0;
	}

	// Bookmarks
	TiXmlElement *pInsertBefore = 0;
	TiXmlElement* pBookmark;
	for (pBookmark = pChild->FirstChildElement("Bookmark"); pBookmark; pBookmark = pBookmark->NextSiblingElement("Bookmark"))
	{
		TiXmlHandle handle(pBookmark);

		wxString old_name = GetTextElement_Trimmed(pBookmark, "Name");
		if (old_name.empty())
			continue;

		if (name == old_name)
		{
			wxMessageBox(_("Name of bookmark already exists."), _("New bookmark"), wxICON_EXCLAMATION);
			return false;
		}
		if (name < old_name && !pInsertBefore)
			pInsertBefore = pBookmark;
	}

	if (pInsertBefore)
		pBookmark = pChild->InsertBeforeChild(pInsertBefore, TiXmlElement("Bookmark"))->ToElement();
	else
		pBookmark = pChild->LinkEndChild(new TiXmlElement("Bookmark"))->ToElement();
	AddTextElement(pBookmark, "Name", name);
	if (!local_dir.empty())
		AddTextElement(pBookmark, "LocalDir", local_dir);
	if (!remote_dir.IsEmpty())
		AddTextElement(pBookmark, "RemoteDir", remote_dir.GetSafePath());
	if (sync)
		AddTextElementRaw(pBookmark, "SyncBrowsing", "1");

	wxString error;
	if (!file.Save(&error))
	{
		if (COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE) == 2)
			return true;

		wxString msg = wxString::Format(_("Could not write \"%s\", the selected sites could not be exported: %s"), file.GetFileName().GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
	}

	return true;
}

bool CSiteManager::ClearBookmarks(wxString sitePath)
{
	if (sitePath[0] != '0')
		return false;

	sitePath = sitePath.Mid(1);

	// We have to synchronize access to sitemanager.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_SITEMANAGER);

	CXmlFile file;
	TiXmlElement* pDocument = 0;

	pDocument = file.Load(_T("sitemanager"));

	if (!pDocument)
	{
		wxString msg = file.GetError() + _T("\n") + _("The bookmarks could not be cleared.");
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	TiXmlElement* pElement = pDocument->FirstChildElement("Servers");
	if (!pElement)
		return false;

	std::list<wxString> segments;
	if (!CSiteManager::UnescapeSitePath(sitePath, segments))
	{
		wxMessageBox(_("Site path is malformed."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement* pChild = GetElementByPath(pElement, segments);
	if (!pChild || strcmp(pChild->Value(), "Server"))
	{
		wxMessageBox(_("Site does not exist."), _("Invalid site path"));
		return 0;
	}

	TiXmlElement *pBookmark = pChild->FirstChildElement("Bookmark");
	while (pBookmark)
	{
		pChild->RemoveChild(pBookmark);
		pBookmark = pChild->FirstChildElement("Bookmark");
	}

	wxString error;
	if (!file.Save(&error))
	{
		if (COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE) == 2)
			return true;

		wxString msg = wxString::Format(_("Could not write \"%s\", the selected sites could not be exported: %s"), file.GetFileName().GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
	}

	return true;
}
