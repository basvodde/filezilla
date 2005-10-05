#ifndef __FTPCONTROLSOCKET_H__
#define __FTPCONTROLSOCKET_H__

#include "logging_private.h"
#include "ControlSocket.h"

#define RECVBUFFERSIZE 4096
#define MAXLINELEN 2000

class CTransferSocket;
class CFtpControlSocket : public CControlSocket
{
	friend class CTransferSocket;
public:
	CFtpControlSocket(CFileZillaEnginePrivate *pEngine);
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

	virtual int FileTransfer(const wxString localFile, const CServerPath &remotePath,
							 const wxString &remoteFile, bool download,
							 const CFileTransferCommand::t_transferSettings& transferSettings);
	int FileTransferParseResponse();
	int FileTransferSend(int prevResult = FZ_REPLY_OK);

	virtual int RawCommand(const wxString& command = _T(""));

	virtual int Delete(const CServerPath& path = CServerPath(), const wxString& file = _T(""));
	
	virtual int RemoveDir(const CServerPath& path = CServerPath(), const wxString& subDir = _T(""));

	virtual int Mkdir(const CServerPath& path, CServerPath start = CServerPath());
	virtual int MkdirParseResponse();
	virtual int MkdirSend();

	virtual int Rename(const CRenameCommand& command);
	virtual int RenameParseResponse();
	virtual int RenameSend(int prevResult = FZ_REPLY_OK);

	virtual int Chmod(const CChmodCommand& command);
	virtual int ChmodParseResponse();
	virtual int ChmodSend(int prevResult = FZ_REPLY_OK);
	
	virtual void OnConnect(wxSocketEvent &event);
	virtual void OnReceive(wxSocketEvent &event);

	virtual bool Send(wxString str);

	// Parse the latest reply line from the server
	void ParseLine(wxString line);

	// Parse the actual response and delegate it to the handlers.
	// It's the last line in a multi-line response.
	void ParseResponse();

	virtual int SendNextCommand(int prevResult = FZ_REPLY_OK);

	int GetReplyCode() const;

	int Logon();
	int LogonParseResponse();
	int LogonSend();

	wxString m_Response;
	wxString m_MultilineResponseCode;

	CTransferSocket *m_pTransferSocket;

	// Some servers keep track of the offset specified by REST between sessions
	// So we always sent a REST 0 for a normal transfer following a restarted one
	bool m_sentRestartOffset;

	char m_receiveBuffer[RECVBUFFERSIZE];
	int m_bufferLen;

	// List of features
	bool m_hasCLNT;
	bool m_hasUTF8;
};

class CFtpFileTransferOpData : public CFileTransferOpData
{
public:
	CFtpFileTransferOpData();
	virtual ~CFtpFileTransferOpData();

	bool bPasv;
	bool bTriedPasv;
	bool bTriedActive;

	bool binary;

	int port;
	wxString host;

	bool tryAbsolutePath;

	bool resume;

	wxFile *pFile;
	
	int transferEndReason;

	CFileTransferCommand::t_transferSettings transferSettings;
};

#endif
