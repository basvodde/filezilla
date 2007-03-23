#ifndef __SITEMANAGER_H__
#define __SITEMANAGER_H__

class CSiteManagerItemData : public wxTreeItemData
{
public:
	CSiteManagerItemData(CServer server = CServer())
	{
		m_server = server;
	}

	virtual ~CSiteManagerItemData()
	{
	}

	CServer m_server;
	wxString m_comments;
	wxString m_localDir;
	CServerPath m_remoteDir;
	wxString m_name; // Only filled by CSiteManager::GetServer
};

#include "dialogex.h"

class TiXmlElement;
class CInterProcessMutex;
class CSiteManager: public wxDialogEx
{	
	DECLARE_EVENT_TABLE();

public:
	/// Constructors
	CSiteManager();
	virtual ~CSiteManager();

	/// Creation
	bool Create(wxWindow* parent);

	/// Creates the controls and sizers
	void CreateControls();
	
	bool GetServer(CSiteManagerItemData& data);
	
protected:
	bool Verify();
	bool UpdateServer();
	bool Load(TiXmlElement *pElement = 0, wxTreeItemId treeId = wxTreeItemId());
	bool Save(TiXmlElement *pElement = 0, wxTreeItemId treeId = wxTreeItemId());
	void SetCtrlState();
	bool LoadDefaultSites();

	bool IsPredefinedItem(wxTreeItemId item);
	
	virtual void OnOK(wxCommandEvent& event);
	virtual void OnCancel(wxCommandEvent& event);
	virtual void OnConnect(wxCommandEvent& event);
	virtual void OnNewSite(wxCommandEvent& event);
	virtual void OnNewFolder(wxCommandEvent& event);
	virtual void OnRename(wxCommandEvent& event);
	virtual void OnDelete(wxCommandEvent& event);
	virtual void OnBeginLabelEdit(wxTreeEvent& event);
	virtual void OnEndLabelEdit(wxTreeEvent& event);
	virtual void OnSelChanging(wxTreeEvent& event);
	virtual void OnSelChanged(wxTreeEvent& event);
	virtual void OnLogontypeSelChanged(wxCommandEvent& event);
	virtual void OnRemoteDirBrowse(wxCommandEvent& event);
	virtual void OnItemActivated(wxTreeEvent& event);
	virtual void OnLimitMultipleConnectionsChanged(wxCommandEvent& event);
	virtual void OnCharsetChange(wxCommandEvent& event);
	virtual void OnProtocolSelChanged(wxCommandEvent& event);
	void OnCopySite(wxCommandEvent& event);
	
	CInterProcessMutex* m_pSiteManagerMutex;

	wxTreeItemId m_predefinedSites;
	wxTreeItemId m_ownSites;
};

#endif //__SITEMANAGER_H__
