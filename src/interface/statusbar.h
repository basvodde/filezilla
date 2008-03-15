#ifndef __STATUSBAR_H__
#define __STATUSBAR_H__

class wxStatusBarEx : public wxStatusBar
{
public:
	wxStatusBarEx(wxTopLevelWindow* parent);
	virtual ~wxStatusBarEx();

	// Adds a child window that gets repositioned on window resize
	// field >= 0: Actual field
	// -1 = last field, -2 = second-last field and so on
	//
	// cx is the horizontal offset inside the field.
	// Children are always centered vertically.
	void AddChild(int field, wxWindow* pChild, int cx);

	// We override these for two reasons:
	// - wxWidgets does not provide a function to get the field widths back
	// - fixup for last field. Under MSW it has a gripper if window is not 
	//   maximized, under GTK2 it always has a gripper. These grippers overlap
	//   last field.
	virtual void SetFieldsCount(int number = 1, const int* widths = NULL);
	virtual void SetStatusWidths(int n, const int *widths);

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

	int* m_columnWidths;

	DECLARE_EVENT_TABLE();
	void OnSize(wxSizeEvent& event);
};

class CStatusBar : public wxStatusBarEx
{
public:
	CStatusBar(wxTopLevelWindow* parent);
};

#endif //__STATUSBAR_H__
