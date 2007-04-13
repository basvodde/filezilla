#ifndef __TRANSFERSOCKET_H__
#define __TRANSFERSOCKET_H__

#include "iothread.h"
#include "backend.h"

class CFileZillaEnginePrivate;
class CFtpControlSocket;
class CDirectoryListingParser;

enum TransferMode
{
	list,
	upload,
	download,
	resumetest
};

class CIOThread;
class CTlsSocket;
class CTransferSocket : public wxEvtHandler
{
public:
	CTransferSocket(CFileZillaEnginePrivate *pEngine, CFtpControlSocket *pControlSocket, enum TransferMode transferMode);
	virtual ~CTransferSocket();

	wxString SetupActiveTransfer(const wxString& ip);
	bool SetupPassiveTransfer(wxString host, int port);

	void SetActive();

	CDirectoryListingParser *m_pDirectoryListingParser;

	bool m_binaryMode;

protected:
	bool CheckGetNextWriteBuffer();
	bool CheckGetNextReadBuffer();
	void FinalizeWrite();

	void TransferEnd(int reason);

	bool InitTls(const CTlsSocket* pPrimaryTlsSocket);

	virtual void OnSocketEvent(wxSocketEvent &event);
	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive();
	virtual void OnSend();
	virtual void OnClose(wxSocketEvent &event);
	virtual void OnIOThreadEvent(CIOThreadEvent& event);

	// Create a socket server
	wxSocketServer* CreateSocketServer();

	DECLARE_EVENT_TABLE();

	wxSocketBase *m_pSocket; // Will point to either client or server socket

	// Will be set only while creating active mode connections
	wxSocketServer *m_pSocketServer;

	CFileZillaEnginePrivate *m_pEngine;
	CFtpControlSocket *m_pControlSocket;

	bool m_bActive;
	bool m_transferEnd; // Set to true if TransferEnd was called

	enum TransferMode m_transferMode;

	char *m_pTransferBuffer;
	int m_transferBufferLen;

	// Set to true if OnClose got called
	// We now have to read all available data in the socket, ignoring any
	// speed limits
	bool m_onCloseCalled;

	bool m_postponedReceive;
	bool m_postponedSend;
	void TriggerPostponedEvents();

	CBackend* m_pBackend;
	
	CTlsSocket* m_pTlsSocket;
	bool m_shutdown;
};

#endif
