#ifndef __COMMANDQUEUE_H__
#define __COMMANDQUEUE_H__

class CFileZillaEngine;
class CNotification;
class CMainFrame;

DECLARE_EVENT_TYPE(fzEVT_GRANTEXCLUSIVEENGINEACCESS, -1)

class CCommandQueue
{
public:
	CCommandQueue(CFileZillaEngine *pEngine, CMainFrame* pMainFrame);
	~CCommandQueue();

	void ProcessCommand(CCommand *pCommand);
	void ProcessNextCommand();
	bool Idle() const;
	bool Cancel();
	void Finish(COperationNotification *pNotification);

	void RequestExclusiveEngine(bool requestExclusive);

	CFileZillaEngine* GetEngineExclusive(int requestId);
	void ReleaseEngine();
	bool EngineLocked() const { return m_exclusiveEngineLock; }

protected:
	void GrantExclusiveEngineRequest();

	CFileZillaEngine *m_pEngine;
	CMainFrame* m_pMainFrame;
	bool m_exclusiveEngineRequest;
	bool m_exclusiveEngineLock;
	int m_requestId;

	std::list<CCommand *> m_CommandList;
};

#endif //__COMMANDQUEUE_H__

