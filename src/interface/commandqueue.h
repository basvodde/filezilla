#ifndef __COMMANDQUEUE_H__
#define __COMMANDQUEUE_H__

class CFileZillaEngine;
class CNotification;
class CMainFrame;

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

protected:
	CFileZillaEngine *m_pEngine;
	CMainFrame* m_pMainFrame;

	std::list<CCommand *> m_CommandList;
};

#endif //__COMMANDQUEUE_H__

