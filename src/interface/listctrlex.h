#ifndef __LISTCTRLEX_H__
#define __LISTCTRLEX_H__

class wxListCtrlEx : public wxListCtrl
{
public:
	wxListCtrlEx(wxWindow *parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxLC_ICON,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxListCtrlNameStr);
	~wxListCtrlEx();

	void ScrollTopItem(int item);

protected:
	virtual void OnPostScroll();
	virtual void OnPreEmitPostScrollEvent();
	void EmitPostScrollEvent();

private:
#ifndef __WXMSW__
	wxWindow* GetMainWindow();
#endif

	DECLARE_EVENT_TABLE();
	void OnPostScrollEvent(wxCommandEvent& event);
	void OnScrollEvent(wxScrollWinEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnSelectionChanged(wxListEvent& event);
};

#endif //__LISTCTRLEX_H__
