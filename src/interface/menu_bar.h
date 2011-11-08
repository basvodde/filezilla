#ifndef __MENU_BAR_H__
#define __MENU_BAR_H__

#include <option_change_event_handler.h>
#include "state.h"

class CMainFrame;
class CMenuBar : public wxMenuBar, public CStateEventHandler, public COptionChangeEventHandler
{
public:
	CMenuBar();
	virtual ~CMenuBar();

	static CMenuBar* Load(CMainFrame* pMainFrame);

	bool ShowItem(int id);
	bool HideItem(int id);

	void UpdateBookmarkMenu();
	void ClearBookmarks();

	std::list<int> m_bookmark_menu_ids;
	std::map<int, wxString> m_bookmark_menu_id_map_global;
	std::map<int, wxString> m_bookmark_menu_id_map_site;

	void UpdateMenubarState();
protected:
	CMainFrame* m_pMainFrame;

	virtual void OnStateChange(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2);
	virtual void OnOptionChanged(int option);

	DECLARE_DYNAMIC_CLASS(CMenuBar);

	DECLARE_EVENT_TABLE();
	void OnMenuEvent(wxCommandEvent& event);

	std::map<wxMenu*, std::map<int, wxMenuItem*> > m_hidden_items;
};

#endif //__MENU_BAR_H__
