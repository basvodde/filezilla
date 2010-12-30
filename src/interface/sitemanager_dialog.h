#ifndef __SITEMANAGER_DIALOG_H__
#define __SITEMANAGER_DIALOG_H__

#include "dialogex.h"
#include "sitemanager.h"

class CInterProcessMutex;
class CWindowStateManager;
class CSiteManagerDropTarget;
class CSiteManagerDialog: public wxDialogEx
{
	friend class CSiteManagerDropTarget;

	DECLARE_EVENT_TABLE();

public:
	struct _connected_site
	{
		CServer server;
		wxString old_path;
		wxString new_path;
	};

	/// Constructors
	CSiteManagerDialog();
	virtual ~CSiteManagerDialog();

	// Creation. If pServer is set, it will cause a new item to be created.
	bool Create(wxWindow* parent, std::vector<_connected_site> *connected_sites, const CServer* pServer = 0);

	bool GetServer(CSiteManagerItemData_Site& data);
	wxString GetSitePath();

protected:
	// Creates the controls and sizers
	void CreateControls(wxWindow* parent);

	bool Verify();
	bool UpdateItem();
	bool UpdateServer(CSiteManagerItemData_Site &server, const wxString& name);
	bool UpdateBookmark(CSiteManagerItemData &bookmark, const CServer& server);
	bool Load();
	bool Save(TiXmlElement *pElement = 0, wxTreeItemId treeId = wxTreeItemId());
	bool SaveChild(TiXmlElement *pElement, wxTreeItemId child);
	void SetCtrlState();
	bool LoadDefaultSites();

	void SetProtocol(ServerProtocol protocol);
	ServerProtocol GetProtocol() const;

	bool IsPredefinedItem(wxTreeItemId item);

	wxString FindFirstFreeName(const wxTreeItemId &parent, const wxString& name);

	void AddNewSite(wxTreeItemId parent, const CServer& server, bool connected = false);
	void CopyAddServer(const CServer& server);

	void AddNewBookmark(wxTreeItemId parent);

	void RememberLastSelected();

	wxString GetSitePath(wxTreeItemId item);

	void MarkConnectedSites();
	void MarkConnectedSite(int connected_site);

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

protected:
	CWindowStateManager* m_pWindowStateManager;

	wxNotebook *m_pNotebook_Site;
	wxNotebook *m_pNotebook_Bookmark;

	std::vector<_connected_site> *m_connected_sites;

	bool m_is_deleting;
};

#endif //__SITEMANAGER_DIALOG_H__