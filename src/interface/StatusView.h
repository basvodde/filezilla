#pragma once

class CStatusView :	public wxWindow
{
public:
	CStatusView(wxWindow* parent, wxWindowID id);
	virtual ~CStatusView();

protected:
	wxHtmlWindow *m_pHtmlWindow;

	void OnSize(wxSizeEvent &event);

	DECLARE_EVENT_TABLE();
};
