#ifndef __TLSSOCKET_H__
#define __TLSSOCKET_H__

#define ssize_t long
#include <gnutls/gnutls.h>
#include "backend.h"

class CControlSocket;
class CTlsSocket : protected wxEvtHandler, public CBackend
{
public:

	CTlsSocket(CControlSocket* pOwner);
	~CTlsSocket();

	bool Init();
	void Uninit();

	void SetSocket(wxSocketBase* pSocket, wxEvtHandler* pEvtHandler);
	int Handshake();

	virtual void Read(void *buffer, unsigned int len);
	virtual void Write(const void *buffer, unsigned int len);
	virtual bool Error() const { return !m_lastSuccessful; }
	virtual unsigned int LastCount() const { return m_lastCount; }
	virtual int LastError() const { return m_lastError; }
	virtual void Peek(void *buffer, unsigned int len);

	void Shutdown();
	
protected:

	void ContinueShutdown();

	enum TlsState
	{
		noconn,
		handshake,
		conn,
		closing,
		closed
	} m_tlsState;

	CControlSocket* m_pOwner;

	bool m_initialized;
	gnutls_session_t m_session;

	gnutls_certificate_credentials_t m_certCredentials;

	void LogError(int code);
	void PrintAlert();
	
	// Failure logs the error, uninits the session and sends a close event
	void Failure(int code);

	static ssize_t PushFunction(gnutls_transport_ptr_t ptr, const void* data, size_t len);
	static ssize_t PullFunction(gnutls_transport_ptr_t ptr, void* data, size_t len);
	ssize_t PushFunction(const void* data, size_t len);
	ssize_t PullFunction(void* data, size_t len);

	void TriggerEvents();

	DECLARE_EVENT_TABLE();
	void OnSocketEvent(wxSocketEvent& event);
	void OnRead();
	void OnSend();

	bool m_canReadFromSocket;
	bool m_canWriteToSocket;

	bool m_canTriggerRead;
	bool m_canTriggerWrite;

	bool m_socketClosed;

	wxSocketBase* m_pSocket;
	wxEvtHandler* m_pEvtHandler;

	// Used by LastError() and LastCount()
	int m_lastError;
	unsigned int m_lastCount;
	bool m_lastSuccessful;

	// Due to the strange gnutls_record_send semantics, call it again
	// with 0 data and 0 length after GNUTLS_E_AGAIN and store the number
	// of bytes written. These bytes get skipped on next write from the
	// application.
	// This avoids the rule to call it again with the -same- data after
	// GNUTLS_E_AGAIN.
	void CheckResumeFailedWrite();
	bool m_lastWriteFailed;
	unsigned int m_writeSkip;

	// Peek data
	char* m_peekData;
	unsigned int m_peekDataLen;
};

#endif //__TLSSOCKET_H__
