#ifndef __VIEWHEADER_H__
#define __VIEWHEADER_H__

class CComboBoxEx;
class CViewHeader : public wxWindow
{
	friend class CComboBoxEx;
public:
	CViewHeader(wxWindow* pParent, const wxString& label);

	// Reparents the control.
	// Calls wxWindow::Reparent under WXMSW and WXGTK. On other systems,
	// a new instance is created with the same content (but with different 
	// parent) and the provided pointer to pViewHeader will be modified.
	static void Reparent(CViewHeader** pViewHeader, wxWindow* parent);

	void SetLabel(const wxString& label);

protected:
	CComboBoxEx* m_pComboBox;
	bool m_alreadyInPaint;

	DECLARE_EVENT_TABLE();
	void OnSize(wxSizeEvent& event);

	void OnPaint(wxPaintEvent& event);
#ifdef __WXMSW__
	void OnComboPaint(wxPaintEvent& event);
	void OnComboMouseEvent(wxMouseEvent& event);
	bool m_bLeftMousePressed;
#endif //__WXMSW__

	wxString m_label;
	int m_cbOffset;
	int m_labelHeight;
};

#endif //__VIEWHEADER_H__
