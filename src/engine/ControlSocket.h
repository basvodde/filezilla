#ifndef __CONTROLSOCKET_H__
#define __CONTROLSOCKET_H__

#define OPERATION_NONE
#define OPERATION_

class COpData
{
public:
	COpData();
	virtual ~COpData();

	int opState;
	enum Command opId;

	COpData *pNextOpData;
};

class CControlSocket : protected wxEvtHandler, public wxSocketClient, public CLogging
{
public:
	CControlSocket(CFileZillaEngine *pEngine);
	virtual ~CControlSocket();

	virtual int Connect(const CServer &server);
	virtual int ContinueConnect();
	virtual int Disconnect();
	virtual void Cancel();
	virtual bool List(CServerPath path = CServerPath(), wxString subDir = _T("")) = 0;

	enum Command GetCurrentCommandId() const;

	int DoClose(int nErrorCode = FZ_REPLY_DISCONNECTED);

	virtual void TransferEnd(int reason) = 0;
	
protected:
	virtual void OnSocketEvent(wxSocketEvent &event);
	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive(wxSocketEvent &event);
	virtual void OnSend(wxSocketEvent &event);
	virtual void OnClose(wxSocketEvent &event);
	virtual int ResetOperation(int nErrorCode);
	virtual bool Send(const char *buffer, int len);

	wxString ConvertDomainName(wxString domain);

	COpData *m_pCurOpData;
	int m_nOpState;
	CFileZillaEngine *m_pEngine;
	CServer *m_pCurrentServer;

	CServerPath m_CurrentPath;
	
	char *m_pSendBuffer;
	int m_nSendBufferLen;

	DECLARE_EVENT_TABLE();
};

#endif
