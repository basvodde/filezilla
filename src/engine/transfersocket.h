#ifndef __TRANSFERSOCKET_H__
#define __TRANSFERSOCKET_H__

class CFileZillaEngine;
class CFtpControlSocket;
class CDirectoryListingParser;

enum TransferMode
{
	list,
	upload,
	download
};

class CTransferSocket : public wxEvtHandler
{
public:
	CTransferSocket(CFileZillaEngine *pEngine, CFtpControlSocket *pControlSocket, enum TransferMode transferMode);
	virtual ~CTransferSocket();

	wxString SetupActiveTransfer();
	bool SetupPassiveTransfer(wxString host, int port);

	void SetActive();

	CDirectoryListingParser *m_pDirectoryListingParser;

	bool m_binaryMode;

protected:
	void TransferEnd(int reason);

	virtual void OnSocketEvent(wxSocketEvent &event);
	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive();
	virtual void OnSend();
	virtual void OnClose(wxSocketEvent &event);

	// Create a socket server
	wxSocketServer* CreateSocketServer();

	DECLARE_EVENT_TABLE();

	wxSocketBase *m_pSocket; // Will point to either client or server socket

	// Only one of the following will be non-zero
	wxSocketClient *m_pSocketClient;
	wxSocketServer *m_pSocketServer;

	CFileZillaEngine *m_pEngine;
	CFtpControlSocket *m_pControlSocket;

	bool m_bActive;
	bool m_transferEnd; // Set to true if TransferEnd was called

	enum TransferMode m_transferMode;

	char *m_pTransferBuffer;
	int m_transferBufferPos;
};

#endif
