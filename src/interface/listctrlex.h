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

	// Ensure that the given item is the first in the list
	void ScrollTopItem(int item);

	void EnablePrefixSearch(bool enable) { m_prefixSearch_enabled = enable; }

	void SaveSetItemCount(long count);

protected:
	virtual void OnPostScroll();
	virtual void OnPreEmitPostScrollEvent();
	void EmitPostScrollEvent();

	virtual wxString GetItemText(int item, unsigned int column) { return _T(""); }

	virtual wxString OnGetItemText(long item, long column) const;

private:
	// Keyboard prefix search
	void HandlePrefixSearch(wxChar character);
	int FindItemWithPrefix(const wxString& searchPrefix, int start);

#ifndef __WXMSW__
	wxScrolledWindow* GetMainWindow();
#endif

	DECLARE_EVENT_TABLE();
	void OnPostScrollEvent(wxCommandEvent& event);
	void OnScrollEvent(wxScrollWinEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnSelectionChanged(wxListEvent& event);
	void OnKeyDown(wxKeyEvent& event);

	bool m_prefixSearch_enabled;
	wxDateTime m_prefixSearch_lastKeyPress;
	wxString m_prefixSearch_prefix;
};

#endif //__LISTCTRLEX_H__
