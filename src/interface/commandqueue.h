#ifndef __COMMANDQUEUE_H__
#define __COMMANDQUEUE_H__

class CFileZillaEngine;
class CNotification;

class CCommandQueue
{
public:
	CCommandQueue(CFileZillaEngine *pEngine);
	~CCommandQueue();

	void ProcessCommand(CCommand *pCommand);
	void ProcessNextCommand();
	bool Idle() const;
	void Cancel();
	void Finish(CNotification *pNotification);

protected:
	CFileZillaEngine *m_pEngine;

	std::list<CCommand *> m_CommandList;
};

#endif //__COMMANDQUEUE_H__

