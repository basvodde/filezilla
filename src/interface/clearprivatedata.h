#ifndef __CLEARPRIVATEDATA_H__
#define __CLEARPRIVATEDATA_H__

#include "dialogex.h"

class CMainFrame;
class CClearPrivateDataDialog : public wxDialogEx
{
public:

	static CClearPrivateDataDialog* Create(CMainFrame* pMainFrame) { return new CClearPrivateDataDialog(pMainFrame); }
	void Show();

	void Delete();

protected:
	CClearPrivateDataDialog(CMainFrame* pMainFrame);
	~CClearPrivateDataDialog() { }

	bool ClearReconnect();

	void RemoveXmlFile(const wxString& name);

	CMainFrame* const m_pMainFrame;

	wxTimer m_timer;

	DECLARE_EVENT_TABLE();
	void OnTimer(wxTimerEvent& event);
};

#endif //__CLEARPRIVATEDATA_H__

