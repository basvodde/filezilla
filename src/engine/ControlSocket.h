#ifndef __CONTROLSOCKET_H__
#define __CONTROLSOCKET_H__

class CControlSocket : public wxSocketClient, wxEvtHandler
{
public:
	CControlSocket();
	virtual ~CControlSocket();

protected:
	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive(wxSocketEvent &event);
	virtual void OnSend(wxSocketEvent &event);
	virtual void OnClose(wxSocketEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
