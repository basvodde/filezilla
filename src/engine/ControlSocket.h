#ifndef __CONTROLSOCKET_H__
#define __CONTROLSOCKET_H__

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

	virtual bool Send(const char *buffer, int len);

protected:
	virtual void OnSocketEvent(wxSocketEvent &event);
	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive(wxSocketEvent &event);
	virtual void OnSend(wxSocketEvent &event);
	virtual void OnClose(wxSocketEvent &event);
	virtual int ResetOperation(int nErrorCode);

	DECLARE_EVENT_TABLE()

	COpData *m_pCurOpData;
	int m_nOpState;
	CFileZillaEngine *m_pEngine;
	CServer *m_pCurrentServer;

	char *m_pSendBuffer;
	int m_nSendBufferLen;
};

#endif
