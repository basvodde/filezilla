#ifndef __STATUSBAR_H__
#define __STATUSBAR_H__

class CStatusBar : public wxStatusBar
{
public:
	CStatusBar(wxTopLevelWindow* parent);

	// Adds a child window that gets repositioned on window resize
	// field >= 0: Actual field
	// -1 = last field, -2 = second-last field and so on
	void AddChild(int field, wxWindow* pChild, int cx);

protected:
	struct t_statbar_child
	{
		int field;
		wxWindow* pChild;
		int cx;
	};

	std::list<struct t_statbar_child> m_children;

	wxTopLevelWindow* m_pParent;
#ifdef __WXMSW__
	bool m_parentWasMaximized;
#endif

	DECLARE_EVENT_TABLE();
	void OnSize(wxSizeEvent& event);
};

#endif //__STATUSBAR_H__
