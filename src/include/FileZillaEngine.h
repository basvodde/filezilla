#ifndef __FILEZILLAENGINE_H__
#define __FILEZILLAENGINE_H__

class CControlSocket;
class CFileZillaEngine  
{
public:
	CFileZillaEngine();
	virtual ~CFileZillaEngine();

	int Init(wxEvtHandler *pEventHandler);

	int Command(const CCommand &command);
	int Connect(const CConnectCommand &command);

	bool IsBusy() const;
	bool IsConnected() const;

	void AddNotification(CNotification *pNotification);
	CNotification *GetNextNotification();

protected:
	wxEvtHandler *m_pEventHandler;
	CControlSocket *m_pControlSocket;

	CCommand *m_pCurrentCommand;

	std::list<CNotification *> m_NotificationList;

};

#endif
