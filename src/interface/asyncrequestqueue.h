#ifndef __ASYNCREQUESTQUEUE_H__

#define __ASYNCREQUESTQUEUE_H__

class CMainFrame;
class CAsyncRequestQueue
{
public:
	CAsyncRequestQueue(CMainFrame *pMainFrame);
	~CAsyncRequestQueue();

	void AddRequest(CFileZillaEngine *pEngine, CAsyncRequestNotification *pNotification);
	void ClearPending(const CFileZillaEngine* pEngine);
	void RecheckDefaults();

protected:
	CMainFrame *m_pMainFrame;

	void ProcessNextRequest();
	bool ProcessDefaults(CFileZillaEngine *pEngine, CAsyncRequestNotification *pNotification);

	struct t_queueEntry
	{
		CFileZillaEngine *pEngine;
		CAsyncRequestNotification *pNotification;
	};
	std::list<t_queueEntry> m_requestList;
};

#endif //__ASYNCREQUESTQUEUE_H__
