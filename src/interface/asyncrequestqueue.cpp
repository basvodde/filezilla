#include "FileZilla.h"
#include "asyncrequestqueue.h"
#include "fileexistsdlg.h"
#include "Mainfrm.h"

CAsyncRequestQueue::CAsyncRequestQueue(CMainFrame *pMainFrame)
{
	m_pMainFrame = pMainFrame;
}

CAsyncRequestQueue::~CAsyncRequestQueue()
{
}

void CAsyncRequestQueue::AddRequest(CFileZillaEngine *pEngine, CAsyncRequestNotification *pNotification)
{
	t_queueEntry entry;
	entry.pEngine = pEngine;
	entry.pNotification = pNotification;

	m_requestList.push_back(entry);

	if (m_requestList.size() == 1)
		ProcessNextRequest();
}

void CAsyncRequestQueue::ProcessNextRequest()
{
	while (!m_requestList.empty())
	{
		t_queueEntry &entry = m_requestList.front();
		
		if (entry.pNotification->GetRequestID() == reqId_fileexists)
		{
			CFileExistsDlg dlg(reinterpret_cast<CFileExistsNotification *>(entry.pNotification));
			dlg.Create(m_pMainFrame);
			dlg.ShowModal();

			entry.pEngine->SetAsyncRequestReply(entry.pNotification);
			delete entry.pNotification;
		}

		m_requestList.pop_front();
	}
}
