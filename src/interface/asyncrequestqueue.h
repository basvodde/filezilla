#ifndef __ASYNCREQUESTQUEUE_H__

#define __ASYNCREQUESTQUEUE_H__

class CMainFrame;
class CQueueView;
class CVerifyCertDialog;
class CAsyncRequestQueue : public wxEvtHandler
{
public:
	CAsyncRequestQueue(CMainFrame *pMainFrame);
	~CAsyncRequestQueue();

	bool AddRequest(CFileZillaEngine *pEngine, CAsyncRequestNotification *pNotification);
	void ClearPending(const CFileZillaEngine* pEngine);
	void RecheckDefaults();

	void SetQueue(CQueueView *pQueue);

protected:
	CMainFrame *m_pMainFrame;
	CQueueView *m_pQueueView;
	CVerifyCertDialog *m_pVerifyCertDlg;

	void ProcessNextRequest();
	bool ProcessDefaults(CFileZillaEngine *pEngine, CAsyncRequestNotification *pNotification);

	struct t_queueEntry
	{
		CFileZillaEngine *pEngine;
		CAsyncRequestNotification *pNotification;
	};
	std::list<t_queueEntry> m_requestList;

	DECLARE_EVENT_TABLE();
	void OnProcessQueue(wxCommandEvent &event);
};

#endif //__ASYNCREQUESTQUEUE_H__
