#ifndef __CONTROLSOCKET_H__
#define __CONTROLSOCKET_H__

enum EngineNotificationType
{
	engineCancel
};

class wxFzEngineEvent : public wxEvent
{
public:
	wxFzEngineEvent(int id, enum EngineNotificationType eventType);
	virtual wxEvent *Clone() const;

	enum EngineNotificationType m_eventType;
};

extern const wxEventType fzEVT_ENGINE_NOTIFICATION;
typedef void (wxEvtHandler::*fzEngineEventFunction)(wxFzEngineEvent&);

#define EVT_FZ_ENGINE_NOTIFICATION(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        fzEVT_ENGINE_NOTIFICATION, id, -1, \
        (wxObjectEventFunction)(fzEngineEventFunction) wxStaticCastEvent( fzEngineEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

#define OPERATION_NONE
#define OPERATION_

class COpData
{
public:
	COpData();
	virtual ~COpData();
};

class CControlSocket : protected wxEvtHandler, public wxSocketClient, public CLogging
{
public:
	CControlSocket(CFileZillaEngine *pEngine);
	virtual ~CControlSocket();

	virtual int Connect(const CServer &server);
	virtual int Disconnect();

	enum Command GetCurrentCommandId() const;

	int DoClose(int nErrorCode = FZ_REPLY_DISCONNECTED);
	
	bool SendEvent(enum EngineNotificationType eventType);

protected:
	virtual void OnSocketEvent(wxSocketEvent &event);
	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive(wxSocketEvent &event);
	virtual void OnSend(wxSocketEvent &event);
	virtual void OnClose(wxSocketEvent &event);
	virtual int ResetOperation(int nErrorCode);
	virtual bool Send(const char *buffer, int len);
	void OnEngineEvent(wxFzEngineEvent &event);

	wxString ConvertDomainName(wxString domain);

	DECLARE_EVENT_TABLE()

	COpData *m_pCurOpData;
	int m_nOpState;
	CFileZillaEngine *m_pEngine;
	CServer *m_pCurrentServer;

	char *m_pSendBuffer;
	int m_nSendBufferLen;
};

#endif
