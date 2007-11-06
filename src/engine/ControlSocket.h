#ifndef __CONTROLSOCKET_H__
#define __CONTROLSOCKET_H__

class COpData
{
public:
	COpData(enum Command op_Id);
	virtual ~COpData();

	int opState;
	const enum Command opId;

	bool waitForAsyncRequest;

	COpData *pNextOpData;
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

	wxDateTime fileTime;
	wxFileOffset localFileSize;
	wxFileOffset remoteFileSize;

	bool tryAbsolutePath;
	bool resume;

	CFileTransferCommand::t_transferSettings transferSettings;

	// Set to true when sending the command which
	// starts the actual transfer
	bool transferInitiated;
};

class CMkdirOpData : public COpData
{
public:
	CMkdirOpData()
		: COpData(cmd_mkdir)
	{
	}

	virtual ~CMkdirOpData()
	{
	}

	CServerPath path;
	CServerPath currentPath;
	CServerPath commonParent;
	std::list<wxString> segments;
};

class CChangeDirOpData : public COpData
{
public:
	CChangeDirOpData()
		: COpData(cmd_cwd)
	{
		triedMkd = false;
	}

	virtual ~CChangeDirOpData()
	{
	}

	CServerPath path;
	wxString subDir;
	bool triedMkd;
	CServerPath target;
};

enum TransferEndReason
{
	none,
	successful,
	timeout,
	transfer_failure,					// Error during the transfer, like lost connection lack of diskspace and so on
	pre_transfer_command_failure,		// If a command fails prior to sending the transfer command
	transfer_command_failure_immediate,	// Used if server does not send the 150 reply after the transfer command
	transfer_command_failure,			// Used if the transfer command fails, but after receiving a 150 first
	failure,							// Other unspecific failure
	failed_resumetest
};

#include "logging_private.h"
#include "backend.h"

class CTransferStatus;
class CControlSocket: public wxEvtHandler, public CLogging
{
public:
	CControlSocket(CFileZillaEnginePrivate *pEngine);
	virtual ~CControlSocket();

	virtual int Connect(const CServer &server) = 0;
	virtual int ContinueConnect(const wxIPV4address *address) { return FZ_REPLY_ERROR; }
	virtual int Disconnect();
	virtual void Cancel();
	virtual int List(CServerPath path = CServerPath(), wxString subDir = _T(""), bool refresh = false) { return FZ_REPLY_NOTSUPPORTED; }
	virtual int FileTransfer(const wxString localFile, const CServerPath &remotePath,
							 const wxString &remoteFile, bool download,
							 const CFileTransferCommand::t_transferSettings& transferSettings) { return FZ_REPLY_NOTSUPPORTED; }
	virtual int RawCommand(const wxString& command = _T("")) { return FZ_REPLY_NOTSUPPORTED; }
	virtual int Delete(const CServerPath& path, const std::list<wxString>& files) { return FZ_REPLY_NOTSUPPORTED; }
	virtual int RemoveDir(const CServerPath& path = CServerPath(), const wxString& subDir = _T("")) { return FZ_REPLY_NOTSUPPORTED; }
	virtual int Mkdir(const CServerPath& path) { return FZ_REPLY_NOTSUPPORTED; }
	virtual int Rename(const CRenameCommand& command) { return FZ_REPLY_NOTSUPPORTED; }
	virtual int Chmod(const CChmodCommand& command) { return FZ_REPLY_NOTSUPPORTED; }
	virtual bool Connected() const = 0;

	// If m_pCurrentOpData is zero, this function returns the current command
	// from the engine.
	enum Command GetCurrentCommandId() const;

	virtual void TransferEnd() { }

	virtual bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification) = 0;

	void InitTransferStatus(wxFileOffset totalSize, wxFileOffset startOffset, bool list);
	void SetTransferStatusStartTime();
	void UpdateTransferStatus(wxFileOffset transferredBytes);
	void ResetTransferStatus();
	bool GetTransferStatus(CTransferStatus &status, bool &changed);

	const CServer* GetCurrentServer() const;

	// Conversion function which convert between local and server charset.
	wxString ConvToLocal(const char* buffer);
	wxChar* ConvToLocalBuffer(const char* buffer);
	wxChar* ConvToLocalBuffer(const char* buffer, wxMBConv& conv);
	wxCharBuffer ConvToServer(const wxString& str, bool force_utf8 = false);

	// ---
	// The following two functions control the timeout behaviour:
	// ---
    
	// Call this if data could be sent or retrieved
	void SetAlive();
	
	// Set to true if waiting for data
	void SetWait(bool waiting);

	CFileZillaEnginePrivate* GetEngine() { return m_pEngine; }

	// Only called from the engine, see there for description
	void InvalidateCurrentWorkingDir(const CServerPath& path);

protected:
	wxTimeSpan GetTimezoneOffset();

	virtual int DoClose(int nErrorCode = FZ_REPLY_DISCONNECTED);
	bool m_closed;

	virtual int ResetOperation(int nErrorCode);

	// Called by ResetOperation if there's a queued operation
	virtual int SendNextCommand(int prevResult = FZ_REPLY_OK);

	wxString ConvertDomainName(wxString domain);

	int CheckOverwriteFile();

	bool ParsePwdReply(wxString reply, bool unquoted = false, const CServerPath& defaultPath = CServerPath());

	COpData *m_pCurOpData;
	int m_nOpState;
	CFileZillaEnginePrivate *m_pEngine;
	CServer *m_pCurrentServer;

	CServerPath m_CurrentPath;
	
	CTransferStatus *m_pTransferStatus;
	int m_transferStatusSendState;
	
	wxCSConv *m_pCSConv;
	bool m_useUTF8;

	// Timeout data
	wxTimer m_timer;
	wxStopWatch m_stopWatch;

	// -------------------------
	// Begin cache locking stuff
	// -------------------------

	// Tries to obtain lock. Returns true on success.
	// On failure, caller has to pass control.
	// SendNextCommand will be called once the lock gets available
	// and engine could obtain it.
	bool TryLockCache(const CServerPath& directory);

	// Unlocks the cache. Can be called if not holding the lock
	void UnlockCache();

	// Called from the fzOBTAINLOCK event.
	// Returns true iff engine is the first waiting engine
	// and obtains the lock.
	// On failure, the engine was not waiting for a lock.
	bool ObtainLockFromEvent();

	bool HasLock();

#ifdef __VISUALC__
	// Retarded compiler does not like my code
	public:
#endif
	struct t_lockInfo
	{
		CControlSocket* pControlSocket;
		CServerPath directory;
		bool waiting;
	};
	static std::list<t_lockInfo> m_lockInfoList;
#ifdef __VISUALC__
	protected:
#endif

	const std::list<t_lockInfo>::iterator GetLockStatus();

	// -----------------------
	// End cache locking stuff
	// -----------------------

	bool m_invalidateCurrentPath;

	DECLARE_EVENT_TABLE();
	void OnTimer(wxTimerEvent& event);
	void OnObtainLock(wxCommandEvent& event);
};

class CRealControlSocket : public CControlSocket, public wxSocketClient
{
public:
	CRealControlSocket(CFileZillaEnginePrivate *pEngine);
	virtual ~CRealControlSocket();

	virtual int Connect(const CServer &server);
	virtual int ContinueConnect(const wxIPV4address *address);

	virtual bool Connected() const { return IsConnected(); }

protected:
	virtual int DoClose(int nErrorCode = FZ_REPLY_DISCONNECTED);
	void ResetSocket();

	// Easier functions to get the IP addresses
	virtual wxString GetLocalIP() const;
	virtual wxString GetPeerIP() const;

	DECLARE_EVENT_TABLE();
	virtual void OnSocketEvent(wxSocketEvent &event);
	virtual void OnConnect();
	virtual void OnReceive();
	virtual void OnSend();
	virtual void OnClose();
	
	virtual bool Send(const char *buffer, int len);

	CBackend* m_pBackend;

	char *m_pSendBuffer;
	int m_nSendBufferLen;

	bool m_onConnectCalled;
};

#endif
