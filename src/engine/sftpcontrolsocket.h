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
	sftpWrite
} sftpEventTypes;

enum sftpRequestTypes
{
	sftpReqPassword,
	sftpReqHostkey,
	sftpReqHostkeyChanged,
	sftpReqUnknown
};

class CSftpEvent : public wxEvent
{
public:
	CSftpEvent(sftpEventTypes type, const wxString& text);
	CSftpEvent(sftpEventTypes type, sftpRequestTypes reqType, const wxString& text1, const wxString& text2 = _T(""), const wxString& text3 = _T(""), const wxString& text4 = _T(""));
	virtual ~CSftpEvent() {}

	virtual wxEvent* Clone() const
	{
		return new CSftpEvent(m_type, m_reqType, m_text[0]);
	}

	sftpEventTypes GetType() const { return m_type; }
	sftpRequestTypes GetRequestType() const { return m_reqType; }
	wxString GetText(int index = 0) const { return m_text[index]; }
	int GetNumber() const;

protected:
	wxString m_text[4];
	sftpEventTypes m_type;
	sftpRequestTypes m_reqType;
};

class CSftpInputThread;
class CSftpControlSocket : public CControlSocket
{
public:
	CSftpControlSocket(CFileZillaEnginePrivate* pEngine);
	virtual ~CSftpControlSocket();

	virtual int Connect(const CServer &server);

	virtual int List(CServerPath path = CServerPath(), wxString subDir = _T(""), bool refresh = false);
	virtual int FileTransfer(const wxString localFile, const CServerPath &remotePath,
							 const wxString &remoteFile, bool download,
							 const CFileTransferCommand::t_transferSettings& transferSettings);
	virtual int RawCommand(const wxString& command = _T("")) { return 0; }
	virtual int Delete(const CServerPath& path = CServerPath(), const wxString& file = _T("")) { return 0; }
	virtual int RemoveDir(const CServerPath& path = CServerPath(), const wxString& subDir = _T("")) { return 0; }
	virtual int Mkdir(const CServerPath& path, CServerPath start = CServerPath());
	virtual int Rename(const CRenameCommand& command) { return 0; }
	virtual int Chmod(const CChmodCommand& command) { return 0; }
	virtual void Cancel();

	virtual bool Connected() const { return m_pInputThread != 0; }

	virtual void TransferEnd(int reason) { }

	virtual bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification);

	bool SendRequest(CAsyncRequestNotification *pNotification);

	void SetActive(bool recv);

protected:
	// Replaces filename"with"quotes with
	// "filename""with""quotes"
	wxString QuoteFilename(wxString filename);

	virtual int DoClose(int nErrorCode = FZ_REPLY_DISCONNECTED);

	virtual int ResetOperation(int nErrorCode);
	virtual int SendNextCommand(int prevResult = FZ_REPLY_OK);

	int ProcessReply(bool successful, const wxString& reply = _T(""));

	int FileTransferSend(int prevResult = FZ_REPLY_OK);
	int FileTransferParseResponse(bool successful, const wxString& reply);

	int ListSend(int prevResult = FZ_REPLY_OK);
	int ListParseResponse(bool successful, const wxString& reply);
	int ListParseEntry(const wxString& entry);

	int ChangeDir(CServerPath path = CServerPath(), wxString subDir = _T(""));
	int ChangeDirParseResponse(bool successful, const wxString& reply);
	int ChangeDirSend();

	int MkdirParseResponse(bool successful, const wxString& reply);
	int MkdirSend();

	bool Send(wxString cmd, const wxString& show = _T(""));

	wxProcess* m_pProcess;
	CSftpInputThread* m_pInputThread;
	int m_pid;

	DECLARE_EVENT_TABLE();
	void OnSftpEvent(CSftpEvent& event);
	void OnTerminate(wxProcessEvent& event);

	// Set to true in destructor of CSftpControlSocket
	// If sftp process dies (or gets killed), OnTerminate will be called. 
	// To avoid a race condition, abort OnTerminate if m_inDestructor is set
	bool m_inDestructor;
	bool m_termindatedInDestructor;
};

#endif //__SFTPCONTROLSOCKET_H__
