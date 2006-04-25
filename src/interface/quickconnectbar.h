#ifndef __QUICKCONNECTBAR_H__
#define __QUICKCONNECTBAR_H__

class CMainFrame;
class CQuickconnectBar : public wxPanel
{
public:
	CQuickconnectBar();
	virtual~CQuickconnectBar();

	bool Create(CMainFrame* pMainFrame);

protected:
	CMainFrame* m_pMainFrame;

	// Only valid while menu is being displayed
	std::list<CServer> m_recentServers;

	DECLARE_EVENT_TABLE();
	void OnQuickconnect(wxCommandEvent& event);
	void OnQuickconnectDropdown(wxCommandEvent& event);
};


#endif //__QUICKCONNECTBAR_H__
