#ifndef __STATUSVIEW_H__
#define __STATUSVIEW_H__

class CStatusView :	public wxWindow
{
public:
	CStatusView(wxWindow* parent, wxWindowID id);
	virtual ~CStatusView();

	void AddToLog(CLogmsgNotification *pNotification);
	void AddToLog(enum MessageType messagetype, wxString message);

	void InitDefAttr();

protected:

	int m_nLineCount;
	wxString m_Content;
	wxTextCtrl *m_pTextCtrl;

	void OnSize(wxSizeEvent &event);

	DECLARE_EVENT_TABLE();
	void OnContextMenu(wxContextMenuEvent& event);
	void OnClear(wxCommandEvent& event);
	void OnCopy(wxCommandEvent& event);

	std::list<int> m_lineLengths;

	struct t_attributeCache
	{
		wxString prefix;
		int len;
		wxTextAttr attr;
	} m_attributeCache[MessageTypeCount];

	bool m_rtl;
};

#endif

