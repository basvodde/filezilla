#pragma once

class CStatusView :	public wxWindow
{
public:
	CStatusView(wxWindow* parent, wxWindowID id);
	virtual ~CStatusView();

	void AddToLog(enum MessageType messagetype, wxString message);

protected:
	wxString m_Content;
	wxHtmlWindow *m_pHtmlWindow;

	void OnSize(wxSizeEvent &event);

	DECLARE_EVENT_TABLE();
};
