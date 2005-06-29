#ifndef __SFTPCONTROLSOCKET_H__
#define __SFTPCONTROLSOCKET_H__

#include "ControlSocket.h"
#include <wx/process.h>

enum sftpEventTypes
{
	sftpReply = 0,
    sftpDone,
    sftpError,
    sftpVerbose,
    sftpStatus,
    sftpRecv,
    sftpSend,
    sftpClose,
    sftpUnknown
};

class CSftpEvent : public wxEvent
{
public:
	CSftpEvent(sftpEventTypes type, const wxString& text);
	virtual ~CSftpEvent() {}

	virtual wxEvent* Clone() const
	{
		return new CSftpEvent(m_type, m_text);
	}

	sftpEventTypes GetType() const { return m_type; }
	wxString GetText() const { return m_text; }

protected:
	wxString m_text;
	sftpEventTypes m_type;
};

class CSftpInputThread;
class CSftpControlSocket : public CControlSocket
{
public:
	CSftpControlSocket(CFileZillaEnginePrivate* pEngine);
	virtual ~CSftpControlSocket();

	virtual int Connect(const CServer &server);

	virtual int List(CServerPath path = CServerPath(), wxString subDir = _T(""), bool refresh = false) { return 0; }
	virtual int FileTransfer(const wxString localFile, const CServerPath &remotePath,
							 const wxString &remoteFile, bool download,
							 const CFileTransferCommand::t_transferSettings& transferSettings) { return 0; }
	virtual int RawCommand(const wxString& command = _T("")) { return 0; }
	virtual int Delete(const CServerPath& path = CServerPath(), const wxString& file = _T("")) { return 0; }
	virtual int RemoveDir(const CServerPath& path = CServerPath(), const wxString& subDir = _T("")) { return 0; }
	virtual int Mkdir(const CServerPath& path, CServerPath start = CServerPath()) { return 0; }
	virtual int Rename(const CRenameCommand& command) { return 0; }
	virtual int Chmod(const CChmodCommand& command) { return 0; }

	virtual bool Connected() const { return m_pInputThread != 0; }

	virtual void TransferEnd(int reason) { }

	virtual bool SetAsyncRequestReply(CAsyncRequestNotification *pNotification) { return false; }

protected:
	bool Send(const char* str);

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
