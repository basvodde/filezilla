#ifndef __FILEZILLAENGINE_H__
#define __FILEZILLAENGINE_H__

enum EngineNotificationType
{
	engineCancel,
	engineHostresolve,
	engineTransferEnd
};

class CControlSocket;
class CAsyncHostResolver;
class wxFzEngineEvent;
class CFileZillaEngine : protected wxEvtHandler
{
	friend class CControlSocket;
	friend class CAsyncHostResolver;
	friend class CTransferSocket; // Only calls SendEvent(engineTransferEnd)
public:
	CFileZillaEngine();
	virtual ~CFileZillaEngine();

	int Init(wxEvtHandler *pEventHandler, COptionsBase *pOptions);

	int Command(const CCommand &command);

	bool IsBusy() const;
	bool IsConnected() const;

	void AddNotification(CNotification *pNotification);
	CNotification *GetNextNotification();

	const CCommand *GetCurrentCommand() const;
	enum Command GetCurrentCommandId() const;

	COptionsBase *GetOptions() const;

	bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification);
	int GetNextAsyncRequestNumber();

protected:
	bool SendEvent(enum EngineNotificationType eventType, int data = 0);
	void OnEngineEvent(wxFzEngineEvent &event);

	int Connect(const CConnectCommand &command);
	int Disconnect(const CDisconnectCommand &command);
	int Cancel(const CCancelCommand &command);
	int List(const CListCommand &command);
	int FileTransfer(const CFileTransferCommand &command);

	int ResetOperation(int nErrorCode);

	wxEvtHandler *m_pEventHandler;
	CControlSocket *m_pControlSocket;

	CCommand *m_pCurrentCommand;

	std::list<CNotification *> m_NotificationList;
	std::list<CAsyncHostResolver *> m_HostResolverThreads;

	bool m_bIsInCommand; //true if Command is on the callstack
	int m_nControlSocketError;

	COptionsBase *m_pOptions;

	unsigned int m_asyncRequestCounter; // Initialized to random value, increased by one on each request

	DECLARE_EVENT_TABLE();
};

#endif
