#ifndef __FILEZILLAENGINEPRIVATE_H__
#define __FILEZILLAENGINEPRIVATE_H__

enum EngineNotificationType
{
	engineCancel,
	engineTransferEnd
};

#include "asynchostresolver.h"

class wxFzEngineEvent;
class CControlSocket;
class CAsyncHostResolver;
class CFileZillaEnginePrivate : public wxEvtHandler
{
public:
	// Resend all modified directory listings if they are available in the cache.
	// This function affects all engines.
	void ResendModifiedListings();

	int ResetOperation(int nErrorCode);	
	void SetActive(bool recv);

	// Add new pending notification
	void AddNotification(CNotification *pNotification);

	unsigned int GetNextAsyncRequestNumber();

	// Event handling
	bool SendEvent(enum EngineNotificationType eventType, int data = 0);

	bool IsBusy() const;
	bool IsConnected() const;

	const CCommand *GetCurrentCommand() const;
	enum Command GetCurrentCommandId() const;

	COptionsBase *GetOptions() { return m_pOptions; }

	// Since host resolving may block, we create a thread in which
	// hosts are resolved. We have to store the resolvers in the engine object,
	// instead of the socket object, else it won't be possible to quickly abort
	// connection attempts, as removing the thread may take some time.
	void AddNewAsyncHostResolver(CAsyncHostResolver* pThread);

	void SendDirectoryListing(CDirectoryListing* pListing);

protected:
	CFileZillaEnginePrivate();
	virtual ~CFileZillaEnginePrivate();

	// Command handlers, only called by CFileZillaEngine::Command
	int Connect(const CConnectCommand &command);
	int Disconnect(const CDisconnectCommand &command);
	int Cancel(const CCancelCommand &command);
	int List(const CListCommand &command);
	int FileTransfer(const CFileTransferCommand &command);
	int RawCommand(const CRawCommand& command);
	int Delete(const CDeleteCommand& command);
	int RemoveDir(const CRemoveDirCommand& command);
	int Mkdir(const CMkdirCommand& command);
	int Rename(const CRenameCommand& command);
	int Chmod(const CChmodCommand& command);

	DECLARE_EVENT_TABLE();
	void OnEngineEvent(wxFzEngineEvent &event);
	void OnAsyncHostResolver(fzAsyncHostResolveEvent& event);

	wxEvtHandler *m_pEventHandler;

	static std::list<CFileZillaEnginePrivate*> m_engineList;

	// Indicicates if data has been received/sent and whether to send any notifications
	static int m_activeStatusSend;
	static int m_activeStatusRecv;

	// Remember last path used in a dirlisting.
	CServerPath m_lastListDir;
	CTimeEx m_lastListTime;

	CControlSocket *m_pControlSocket;

	CCommand *m_pCurrentCommand;

	std::list<CNotification*> m_NotificationList;
	bool m_maySendNotificationEvent;

	std::list<CAsyncHostResolver*> m_HostResolverThreads;

	bool m_bIsInCommand; //true if Command is on the callstack
	int m_nControlSocketError;

	COptionsBase *m_pOptions;

	unsigned int m_asyncRequestCounter;

	// Used to synchronize access to the notification list
	wxCriticalSection m_lock;
};

#endif //__FILEZILLAENGINEPRIVATE_H__
