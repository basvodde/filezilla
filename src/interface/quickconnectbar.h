#ifndef __QUICKCONNECTBAR_H__
#define __QUICKCONNECTBAR_H__

class CMainFrame;
class CQuickconnectBar : public wxPanel
{
public:
	CQuickconnectBar();
	virtual~CQuickconnectBar();

	bool Create(CMainFrame* pParent);

	void ClearFields();

protected:
	// Only valid while menu is being displayed
	std::list<CServer> m_recentServers;

	DECLARE_EVENT_TABLE();
	void OnQuickconnect(wxCommandEvent& event);
	void OnQuickconnectDropdown(wxCommandEvent& event);
	void OnMenu(wxCommandEvent& event);
	void OnKeyboardNavigation(wxNavigationKeyEvent& event);

	wxTextCtrl* m_pHost;
	wxTextCtrl* m_pUser;
	wxTextCtrl* m_pPass;
	wxTextCtrl* m_pPort;

	CMainFrame *m_pMainFrame;
};


#endif //__QUICKCONNECTBAR_H__
