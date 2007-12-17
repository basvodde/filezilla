#ifndef __SFTPCONTROLSOCKET_H__
#define __SFTPCONTROLSOCKET_H__

#include "ControlSocket.h"
#include <wx/process.h>

typedef enum
{
	sftpUnknown = -1,
	sftpReply = 0,
	sftpDone,
	sftpError,
	sftpVerbose,
	sftpStatus,
	sftpRecv,
	sftpSend,
	sftpClose,
	sftpRequest,
	sftpListentry,
	sftpRead,
	sftpWrite,
	sftpRequestPreamble,
	sftpRequestInstruction,
	sftpUsedQuotaRecv,
	sftpUsedQuotaSend
} sftpEventTypes;

enum sftpRequestTypes
{
	sftpReqPassword,
	sftpReqHostkey,
	sftpReqHostkeyChanged,
	sftpReqUnknown
};

class CSftpInputThread;
class CSftpControlSocket : public CControlSocket, public CRateLimiterObject
{
public:
	CSftpControlSocket(CFileZillaEnginePrivate* pEngine);
	virtual ~CSftpControlSocket();

	virtual int Connect(const CServer &server);

	virtual int List(CServerPath path = CServerPath(), wxString subDir = _T(""), bool refresh = false);
	virtual int Delete(const CServerPath& path, const std::list<wxString>& files);
	virtual int RemoveDir(const CServerPath& path = CServerPath(), const wxString& subDir = _T(""));
	virtual int Mkdir(const CServerPath& path);
	virtual int Rename(const CRenameCommand& command);
	virtual int Chmod(const CChmodCommand& command);
	virtual void Cancel();

	virtual bool Connected() const { return m_pInputThread != 0; }

	virtual bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification);

	bool SendRequest(CAsyncRequestNotification *pNotification);

	void SetActive(bool recv);

protected:
	// Replaces filename"with"quotes with
	// "filename""with""quotes"
	wxString QuoteFilename(wxString filename);

	virtual int DoClose(int nErrorCode = FZ_REPLY_DISCONNECTED);

	virtual int ResetOperation(int nErrorCode);
	virtual int SendNextCommand();
	virtual int ParseSubcommandResult(int prevResult);

	int ProcessReply(bool successful, const wxString& reply = _T(""));

	int ConnectParseResponse(bool successful, const wxString& reply);

	virtual int FileTransfer(const wxString localFile, const CServerPath &remotePath,
							 const wxString &remoteFile, bool download,
							 const CFileTransferCommand::t_transferSettings& transferSettings);
	int FileTransferSubcommandResult(int prevResult);
	int FileTransferSend();
	int FileTransferParseResponse(bool successful, const wxString& reply);

	int ListSubcommandResult(int prevResult);
	int ListSend();
	int ListParseResponse(bool successful, const wxString& reply);
	int ListParseEntry(const wxString& entry);

	int ChangeDir(CServerPath path = CServerPath(), wxString subDir = _T(""));
	int ChangeDirParseResponse(bool successful, const wxString& reply);
	int ChangeDirSubcommandResult(int prevResult);
	int ChangeDirSend();

	int MkdirParseResponse(bool successful, const wxString& reply);
	int MkdirSend();

	int DeleteParseResponse(bool successful, const wxString& reply);
	int DeleteSend();

	int RemoveDirParseResponse(bool successful, const wxString& reply);

	int ChmodParseResponse(bool successful, const wxString& reply);
	int ChmodSubcommandResult(int prevResult);
	int ChmodSend();

	int RenameParseResponse(bool successful, const wxString& reply);
	int RenameSubcommandResult(int prevResult);
	int RenameSend();

	bool Send(wxString cmd, const wxString& show = _T(""));
	bool AddToStream(const wxString& cmd, bool force_utf8 = false);

	virtual void OnRateAvailable(enum CRateLimiter::rate_direction direction);
	void OnQuotaRequest(enum CRateLimiter::rate_direction direction);

	// see src/putty/wildcard.c
	wxString WildcardEscape(const wxString& file);

	wxProcess* m_pProcess;
	CSftpInputThread* m_pInputThread;
	int m_pid;

	DECLARE_EVENT_TABLE();
	void OnTerminate(wxProcessEvent& event);
	void OnSftpEvent(wxCommandEvent& event);

	// Set to true in destructor of CSftpControlSocket
	// If sftp process dies (or gets killed), OnTerminate will be called. 
	// To avoid a race condition, abort OnTerminate if m_inDestructor is set
	bool m_inDestructor;
	bool m_termindatedInDestructor;

	wxString m_requestPreamble;
	wxString m_requestInstruction;
};

#endif //__SFTPCONTROLSOCKET_H__
