#ifndef __FTPCONTROLSOCKET_H__
#define __FTPCONTROLSOCKET_H__

#include "logging_private.h"
#include "ControlSocket.h"
#include "externalipresolver.h"

#define RECVBUFFERSIZE 4096
#define MAXLINELEN 2000

class CTransferSocket;
class CFtpTransferOpData;
class CRawTransferOpData;
class CTlsSocket;
class CFtpControlSocket : public CRealControlSocket
{
	friend class CTransferSocket;
public:
	CFtpControlSocket(CFileZillaEnginePrivate *pEngine);
	virtual ~CFtpControlSocket();
	virtual void TransferEnd();

	virtual bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification);

protected:
	
	virtual int ResetOperation(int nErrorCode);

	virtual int Connect(const CServer &server);
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
	int FileTransferTestResumeCapability();

	virtual int RawCommand(const wxString& command);
	int RawCommandSend();
	int RawCommandParseResponse();

	virtual int Delete(const CServerPath& path, const wxString& file);
	int DeleteSend(int prevResult = FZ_REPLY_OK);
	int DeleteParseResponse();
	
	virtual int RemoveDir(const CServerPath& path, const wxString& subDir);
	int RemoveDirSend(int prevResult = FZ_REPLY_OK);
	int RemoveDirParseResponse();

	virtual int Mkdir(const CServerPath& path);
	virtual int MkdirParseResponse();
	virtual int MkdirSend();

	virtual int Rename(const CRenameCommand& command);
	virtual int RenameParseResponse();
	virtual int RenameSend(int prevResult = FZ_REPLY_OK);

	virtual int Chmod(const CChmodCommand& command);
	virtual int ChmodParseResponse();
	virtual int ChmodSend(int prevResult = FZ_REPLY_OK);

	virtual int Transfer(const wxString& cmd, CFtpTransferOpData* oldData);
	virtual int TransferParseResponse();
	virtual int TransferSend(int prevResult = FZ_REPLY_OK);
	
	virtual void OnConnect();
	virtual void OnReceive();

	virtual bool Send(wxString str, bool maskArgs = false);

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
	
	bool ParsePasvResponse(CRawTransferOpData* pData);

	// Some servers are broken. Instead of an empty listing, some MVS servers
	// for example they return "550 no members found"
	// Other servers return "550 No files found."
	bool IsMisleadingListResponse() const;

	int GetExternalIPAddress(wxString& address);

	// Checks if listing2 is a subset of listing1. Compares only filenames.
	bool CheckInclusion(const CDirectoryListing& listing1, const CDirectoryListing& listing2);

	wxString m_Response;
	wxString m_MultilineResponseCode;
	std::list<wxString> m_MultilineResponseLines;

	CTransferSocket *m_pTransferSocket;

	// Some servers keep track of the offset specified by REST between sessions
	// So we always sent a REST 0 for a normal transfer following a restarted one
	bool m_sentRestartOffset;

	char m_receiveBuffer[RECVBUFFERSIZE];
	int m_bufferLen;
	int m_repliesToSkip; // Set to the amount of pending replies if cancelling an action

	int m_pendingReplies;

	CExternalIPResolver* m_pIPResolver;

	CTlsSocket* m_pTlsSocket;
	bool m_protectDataChannel;

	int m_lastTypeBinary;

	int m_pendingTransferEndEvents;

	DECLARE_EVENT_TABLE();
	void OnExternalIPAddress(fzExternalIPResolveEvent& event);
};

class CIOThread;

class CFtpTransferOpData
{
public:
	CFtpTransferOpData();

	enum TransferEndReason transferEndReason;
	bool tranferCommandSent;

	wxLongLong resumeOffset;
	bool binary;
};

class CFtpFileTransferOpData : public CFileTransferOpData, public CFtpTransferOpData
{
public:
	CFtpFileTransferOpData();
	virtual ~CFtpFileTransferOpData();

	CIOThread *pIOThread;
	bool fileDidExist;
};

class CRawTransferOpData : public COpData
{
public:
	CRawTransferOpData();
	wxString cmd;

	CFtpTransferOpData* pOldData;

	bool bPasv;
	bool bTriedPasv;
	bool bTriedActive;

	wxString host;
	int port;
};

#endif
