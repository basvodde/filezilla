#ifndef __SITEMANAGER_H__
#define __SITEMANAGER_H__

class COptions;
class TiXmlElement;
class CSiteManager: public wxDialog
{	
	DECLARE_EVENT_TABLE();

public:
	/// Constructors
	CSiteManager(COptions* pOptions);

	/// Creation
	bool Create(wxWindow* parent);

	/// Creates the controls and sizers
	void CreateControls();
	
	void DisplayServer(const CServer &server);
	void GetServer(CServer &server);
	
protected:
	bool Verify();
	bool Load(TiXmlElement *pElement = 0, wxTreeItemId treeId = wxTreeItemId());
	bool Save(TiXmlElement *pElement = 0, wxTreeItemId treeId = wxTreeItemId());
	
	virtual void OnOK(wxCommandEvent& event);
	virtual void OnCancel(wxCommandEvent& event);
	virtual void OnNewFolder(wxCommandEvent& event);
	virtual void OnRename(wxCommandEvent& event);
	virtual void OnDelete(wxCommandEvent& event);
	virtual void OnBeginLabelEdit(wxTreeEvent& event);
	virtual void OnEndLabelEdit(wxTreeEvent& event);
	
	COptions* m_pOptions;
};

#endif //__SITEMANAGER_H__
