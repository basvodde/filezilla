#ifndef __CONTROLSOCKET_H__
#define __CONTROLSOCKET_H__

#define OPERATION_NONE
#define OPERATION_

class COpData
{
};

class CControlSocket : protected wxEvtHandler, public wxSocketClient, public CLogging
{
public:
	CControlSocket(CFileZillaEngine *pEngine);
	virtual ~CControlSocket();

	virtual int Connect(const CServer &server);
	virtual void Disconnect();

protected:
	virtual void OnSocketEvent(wxSocketEvent &event);
	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive(wxSocketEvent &event);
	virtual void OnSend(wxSocketEvent &event);
	virtual void OnClose(wxSocketEvent &event);

	DECLARE_EVENT_TABLE()

	COpData *m_pCurOpData;
	int m_nOpState;
};

#endif
