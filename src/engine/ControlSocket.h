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

	bool waitForAsyncRequest;

	COpData *pNextOpData;
};

class CTransferStatus;
class CControlSocket : protected wxEvtHandler, public wxSocketClient, public CLogging
{
public:
	CControlSocket(CFileZillaEngine *pEngine);
	virtual ~CControlSocket();

	virtual int Connect(const CServer &server);
	virtual int ContinueConnect();
	virtual int Disconnect();
	virtual void Cancel();
	virtual int List(CServerPath path = CServerPath(), wxString subDir = _T(""), bool refresh = false) = 0;
	virtual int FileTransfer(const wxString localFile, const CServerPath &remotePath, const wxString &remoteFile, bool download) = 0;
	virtual int RawCommand(const wxString& command = _T("")) = 0;
	virtual int Delete(const CServerPath& path = CServerPath(), const wxString& file = _T("")) = 0;

	enum Command GetCurrentCommandId() const;

	int DoClose(int nErrorCode = FZ_REPLY_DISCONNECTED);

	virtual void TransferEnd(int reason) = 0;

	virtual bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification) = 0;

	void InitTransferStatus(wxFileOffset totalSize, wxFileOffset startOffset);
	void UpdateTransferStatus(wxFileOffset transferredBytes);
	void ResetTransferStatus();
	bool GetTransferStatus(CTransferStatus &status, bool &changed);

	const CServer* GetCurrentServer() const;
	
protected:
	virtual void OnSocketEvent(wxSocketEvent &event);
	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive(wxSocketEvent &event);
	virtual void OnSend(wxSocketEvent &event);
	virtual void OnClose(wxSocketEvent &event);
	virtual int ResetOperation(int nErrorCode);
	virtual bool Send(const char *buffer, int len);

	void SendDirectoryListing(CDirectoryListing* pListing);

	wxString ConvertDomainName(wxString domain);

	COpData *m_pCurOpData;
	int m_nOpState;
	CFileZillaEngine *m_pEngine;
	CServer *m_pCurrentServer;

	CServerPath m_CurrentPath;
	
	char *m_pSendBuffer;
	int m_nSendBufferLen;

	CTransferStatus *m_pTransferStatus;
	int m_transferStatusSendState;
	
	bool m_onConnectCalled;

	DECLARE_EVENT_TABLE();
};

#endif
