#ifndef __STATUSVIEW_H__
#define __STATUSVIEW_H__

class CStatusView :	public wxWindow
{
public:
	CStatusView(wxWindow* parent, wxWindowID id);
	virtual ~CStatusView();

	void AddToLog(CLogmsgNotification *pNotification);
	void AddToLog(enum MessageType messagetype, wxString message);


protected:
	void InitDefAttr();

	int m_nLineCount;
	wxString m_Content;
	wxTextCtrl *m_pTextCtrl;
	wxTextAttr m_defAttr;

	void OnSize(wxSizeEvent &event);

	DECLARE_EVENT_TABLE();
	void OnContextMenu(wxContextMenuEvent& event);
	void OnClear(wxCommandEvent& event);
	void OnCopy(wxCommandEvent& event);
};

#endif

