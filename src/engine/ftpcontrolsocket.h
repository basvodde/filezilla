#ifndef __FTPCONTROLSOCKET_H__
#define __FTPCONTROLSOCKET_H__

#include "logging_private.h"
#include "ControlSocket.h"

class CTransferSocket;
class CFtpControlSocket : public CControlSocket
{
	friend class CTransferSocket;
public:
	CFtpControlSocket(CFileZillaEngine *pEngine);
	virtual ~CFtpControlSocket();
	virtual void TransferEnd(int reason);

	virtual bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification);

protected:
	virtual int ResetOperation(int nErrorCode);

	virtual int List(CServerPath path = CServerPath(), wxString subDir = _T(""), bool refresh = false);
	int ListParseResponse();
	int ListSend(int prevResult = FZ_REPLY_OK);

	int ChangeDir(CServerPath path = CServerPath(), wxString subDir = _T(""));
	int ChangeDirParseResponse();
	int ChangeDirSend();

	virtual int FileTransfer(const wxString localFile, const CServerPath &remotePath, const wxString &remoteFile, bool download);
	int FileTransferParseResponse();
	int FileTransferSend(int prevResult = FZ_REPLY_OK);

	virtual int RawCommand(const wxString& command = _T(""));

	virtual int Delete(const CServerPath& path = CServerPath(), const wxString& file = _T(""));
	
	virtual int RemoveDir(const CServerPath& path = CServerPath(), const wxString& subDir = _T(""));
	
	bool ParsePwdReply(wxString reply);

	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive(wxSocketEvent &event);

	virtual bool Send(wxString str);

	void ParseResponse();
	int SendNextCommand(int prevResult = FZ_REPLY_OK);

	int GetReplyCode() const;

	int Logon();
	int LogonParseResponse();
	int LogonSend();

	int CheckOverwriteFile();

	wxString m_ReceiveBuffer;
	wxString m_MultilineResponseCode;

	CTransferSocket *m_pTransferSocket;
};

class CFileTransferOpData : public COpData
{
public:
	CFileTransferOpData();
	virtual ~CFileTransferOpData();

	// Transfer data
	wxString localFile, remoteFile;
	CServerPath remotePath;
	bool download;

	bool bPasv;
	bool bTriedPasv;
	bool bTriedActive;

	bool binary;

	int port;
	wxString host;

	bool tryAbsolutePath;

	bool resume;

	wxDateTime fileTime;
	wxFileOffset fileSize;

	wxFile *pFile;
	
	int transferEndReason;
};


#endif

