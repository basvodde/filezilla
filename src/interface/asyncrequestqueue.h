#ifndef __ASYNCREQUESTQUEUE_H__

#define __ASYNCREQUESTQUEUE_H__

class CMainFrame;
class CAsyncRequestQueue
{
public:
	CAsyncRequestQueue(CMainFrame *pMainFrame);
	~CAsyncRequestQueue();

	void AddRequest(CFileZillaEngine *pEngine, CAsyncRequestNotification *pNotification);

protected:
	CMainFrame *m_pMainFrame;

	void ProcessNextRequest();

	struct t_queueEntry
	{
		CFileZillaEngine *pEngine;
		CAsyncRequestNotification *pNotification;
	};
	std::list<t_queueEntry> m_requestList;
};

#endif //__ASYNCREQUESTQUEUE_H__
