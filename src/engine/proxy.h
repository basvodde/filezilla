#ifndef __PROXY_H__
#define __PROXY_H__

#include "backend.h"
#include "socket.h"

class CProxySocket : protected wxEvtHandler, public CBackend
{
public:
	CProxySocket(wxEvtHandler* pEvtHandler, CSocket* pSocket, CControlSocket* pOwner);
	virtual ~CProxySocket();

	enum ProxyState
	{
		noconn,
		handshake,
		conn,
	};

	enum ProxyType
	{
		unknown,
		HTTP,
		SOCKS5,
		
		proxytype_count
	};

	int Handshake(enum ProxyType type, const wxString& host, unsigned int port, const wxString& user, const wxString& pass);

	enum ProxyState GetState() const { return m_proxyState; }

	// Past the initial handshake, proxies are transparent.
	// Class users should detach socket and use a normal socket backend instead.
	virtual void OnRateAvailable(enum CRateLimiter::rate_direction direction) {};
	virtual void Read(void *data, unsigned int len) {}
	virtual void Write(const void *data, unsigned int len) {}
	virtual bool Error() const { return 0; }
	virtual int LastError() const { return 0; }
	virtual unsigned int LastCount() const { return 0; }
	virtual void Peek(void *buffer, unsigned int len) {}

	void Detach();
	bool Detached() const { return m_pSocket == 0; }

	enum ProxyType GetProxyType() const { return m_proxyType; }
	wxString GetUser() const { return m_user; }
	wxString GetPass() const { return m_pass; }

protected:
	CSocket* m_pSocket;
	CControlSocket* m_pOwner;

	enum ProxyType m_proxyType;
	wxString m_host;
	int m_port;
	wxString m_user;
	wxString m_pass;

	enum ProxyState m_proxyState;

	int m_handshakeState;

	char* m_pSendBuffer;
	int m_sendBufferLen;

	char* m_pRecvBuffer;
	int m_recvBufferPos;
	int m_recvBufferLen;

	DECLARE_EVENT_TABLE();
	void OnSocketEvent(CSocketEvent& event);
	void OnReceive();
	void OnSend();

	bool m_can_write;
	bool m_can_read;
};

#endif //__PROXY_H__
