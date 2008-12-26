#ifndef __SITEMANAGER_H__
#define __SITEMANAGER_H__

class CSiteManagerItemData : public wxTreeItemData
{
public:
	enum type
	{
		SITE,
		BOOKMARK
	};

	CSiteManagerItemData(enum type item_type)
		: m_type(item_type)
	{
	}

	virtual ~CSiteManagerItemData()
	{
	}

	wxString m_localDir;
	CServerPath m_remoteDir;

	enum type m_type;
};

class CSiteManagerItemData_Site : public CSiteManagerItemData
{
public:
	CSiteManagerItemData_Site(const CServer& server = CServer())
		: CSiteManagerItemData(SITE), m_server(server)
	{
	}

	CServer m_server;
	wxString m_comments;
};

#include "dialogex.h"

class TiXmlElement;
class CInterProcessMutex;
class CSiteManagerXmlHandler;
class CWindowStateManager;
class CSiteManagerDropTarget;
class CSiteManager: public wxDialogEx
{
	friend class CSiteManagerDropTarget;

	DECLARE_EVENT_TABLE();

public:
	/// Constructors
	CSiteManager();
	virtual ~CSiteManager();

	// Creation. If pServer is set, it will cause a new item to be created.
	bool Create(wxWindow* parent, const CServer* pServer = 0);

	bool GetServer(CSiteManagerItemData_Site& data);
	wxString GetSitePath();
	static bool GetBookmarks(wxString sitePath, std::list<wxString> &bookmarks);

	static wxMenu* GetSitesMenu();
	static void ClearIdMap();

	// This function also clears the Id map
	static CSiteManagerItemData_Site* GetSiteById(int id, wxString &path);
	static CSiteManagerItemData_Site* GetSiteByPath(wxString sitePath);

	static bool UnescapeSitePath(wxString path, std::list<wxString>& result);

protected:
	// Creates the controls and sizers
	void CreateControls(wxWindow* parent);

	bool Verify();
	bool UpdateItem();
	bool UpdateServer(CSiteManagerItemData_Site &server, const wxString& name);
	bool UpdateBookmark(CSiteManagerItemData &bookmark, const CServer& server);
	bool Load();
	static bool Load(TiXmlElement *pElement, CSiteManagerXmlHandler* pHandler);
	bool Save(TiXmlElement *pElement = 0, wxTreeItemId treeId = wxTreeItemId());
	bool SaveChild(TiXmlElement *pElement, wxTreeItemId child);
	void SetCtrlState();
	bool LoadDefaultSites();

	bool IsPredefinedItem(wxTreeItemId item);

	static CSiteManagerItemData_Site* ReadServerElement(TiXmlElement *pElement);

	wxString FindFirstFreeName(const wxTreeItemId &parent, const wxString& name);

	void AddNewSite(wxTreeItemId parent, const CServer& server);
	void CopyAddServer(const CServer& server);

	void AddNewBookmark(wxTreeItemId parent);

	void RememberLastSelected();

	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnConnect(wxCommandEvent& event);
	void OnNewSite(wxCommandEvent& event);
	void OnNewFolder(wxCommandEvent& event);
	void OnRename(wxCommandEvent& event);
	void OnDelete(wxCommandEvent& event);
	void OnBeginLabelEdit(wxTreeEvent& event);
	void OnEndLabelEdit(wxTreeEvent& event);
	void OnSelChanging(wxTreeEvent& event);
	void OnSelChanged(wxTreeEvent& event);
	void OnLogontypeSelChanged(wxCommandEvent& event);
	void OnRemoteDirBrowse(wxCommandEvent& event);
	void OnItemActivated(wxTreeEvent& event);
	void OnLimitMultipleConnectionsChanged(wxCommandEvent& event);
	void OnCharsetChange(wxCommandEvent& event);
	void OnProtocolSelChanged(wxCommandEvent& event);
	void OnBeginDrag(wxTreeEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnCopySite(wxCommandEvent& event);
	void OnContextMenu(wxTreeEvent& event);
	void OnExportSelected(wxCommandEvent& event);
	void OnNewBookmark(wxCommandEvent& event);
	void OnBookmarkBrowse(wxCommandEvent& event);

	CInterProcessMutex* m_pSiteManagerMutex;

	wxTreeItemId m_predefinedSites;
	wxTreeItemId m_ownSites;

	wxTreeItemId m_dropSource;

	wxTreeItemId m_contextMenuItem;

	bool MoveItems(wxTreeItemId source, wxTreeItemId target, bool copy);

	// Initialized by GetSitesMenu
public:
	struct _menu_data
	{
		wxString path;
		CSiteManagerItemData_Site* data;
	};
protected:
	static std::map<int, struct _menu_data> m_idMap;

	// The map maps event id's to sites
	static wxMenu* GetSitesMenu_Predefied(std::map<int, struct _menu_data> &idMap);

	CWindowStateManager* m_pWindowStateManager;

	wxNotebook *m_pNotebook_Site;
	wxNotebook *m_pNotebook_Bookmark;
};

#endif //__SITEMANAGER_H__
