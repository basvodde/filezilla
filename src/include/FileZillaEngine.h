#ifndef __FILEZILLAENGINE_H__
#define __FILEZILLAENGINE_H__

class CControlSocket;
class CFileZillaEngine  
{
	friend class CControlSocket;
public:
	CFileZillaEngine();
	virtual ~CFileZillaEngine();

	int Init(wxEvtHandler *pEventHandler);

	int Command(const CCommand &command);

	bool IsBusy() const;
	bool IsConnected() const;

	void AddNotification(CNotification *pNotification);
	CNotification *GetNextNotification();

	const CCommand *GetCurrentCommand() const;
	enum Command GetCurrentCommandId() const;

protected:
	int Connect(const CConnectCommand &command);
	int Disconnect(const CDisconnectCommand &command);

	int ResetOperation(int nErrorCode);

	wxEvtHandler *m_pEventHandler;
	CControlSocket *m_pControlSocket;

	CCommand *m_pCurrentCommand;

	std::list<CNotification *> m_NotificationList;

	bool m_bIsInCommand; //true if Command is on the callstack
};

#endif
