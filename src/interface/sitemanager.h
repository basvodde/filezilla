#ifndef __SITEMANAGER_H__
#define __SITEMANAGER_H__

#include <wx/treectrl.h>

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
		m_sync = false;
	}

	virtual ~CSiteManagerItemData()
	{
	}

	wxString m_localDir;
	CServerPath m_remoteDir;

	enum type m_type;

	bool m_sync;
};

class CSiteManagerItemData_Site : public CSiteManagerItemData
{
public:
	CSiteManagerItemData_Site(const CServer& server = CServer())
		: CSiteManagerItemData(SITE), m_server(server)
	{
		connected_item = -1;
	}

	CServer m_server;
	wxString m_comments;

	// Needed to keep track of currently connected sites so that
	// bookmarks and bookmark path can be updated in response to
	// changes done here
	int connected_item;
};

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

class TiXmlElement;
class CSiteManagerXmlHandler;
class CSiteManagerDialog;
class CSiteManager
{
	friend class CSiteManagerDialog;
public:
	// This function also clears the Id map
	static CSiteManagerItemData_Site* GetSiteById(int id, wxString &path);
	static CSiteManagerItemData_Site* GetSiteByPath(wxString sitePath);

	static bool GetBookmarks(wxString sitePath, std::list<wxString> &bookmarks);

	static wxString AddServer(CServer server);
	static bool AddBookmark(wxString sitePath, const wxString& name, const wxString &local_dir, const CServerPath &remote_dir, bool sync);
	static bool ClearBookmarks(wxString sitePath);

	static wxMenu* GetSitesMenu();
	static void ClearIdMap();

	static bool UnescapeSitePath(wxString path, std::list<wxString>& result);

	// Initialized by GetSitesMenu
	struct _menu_data
	{
		wxString path;
		CSiteManagerItemData_Site* data;
	};
protected:
	static bool Load(TiXmlElement *pElement, CSiteManagerXmlHandler* pHandler);
	static CSiteManagerItemData_Site* ReadServerElement(TiXmlElement *pElement);

	static TiXmlElement* GetElementByPath(TiXmlElement* pNode, std::list<wxString> &segments);

	static std::map<int, struct _menu_data> m_idMap;

	// The map maps event id's to sites
	static wxMenu* GetSitesMenu_Predefined(std::map<int, struct _menu_data> &idMap);
};

#endif //__SITEMANAGER_H__
