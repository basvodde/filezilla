#ifndef __COMMANDS_H__
#define __COMMANDS_H__

enum Command
{
	cmd_none = 0,
	cmd_connect,
	cmd_disconnect,
	cmd_cancel,
	cmd_list,
	cmd_transfer,
	cmd_delete,
	cmd_removedir,
	cmd_makeDir,
	cmd_rename,
	cmd_chmod,
	cmd_raw,
	cmd_private // only used internally
};

#define FZ_REPLY_OK				(0x0000)
#define FZ_REPLY_WOULDBLOCK		(0x0001)
#define FZ_REPLY_ERROR			(0x0002)
#define FZ_REPLY_CRITICALERROR	(0x0004)
#define FZ_REPLY_CANCELED		(0x0008 | FZ_REPLY_ERROR)
#define FZ_REPLY_SYNTAXERROR	(0x0010 | FZ_REPLY_ERROR)
#define FZ_REPLY_NOTCONNECTED	(0x0020 | FZ_REPLY_ERROR)
#define FZ_REPLY_DISCONNECTED	(0x0040)
#define FZ_REPLY_INTERNALERROR	(0x0080)
#define FZ_REPLY_BUSY			(0x0100 | FZ_REPLY_ERROR)
#define FZ_REPLY_ALREADYCONNECTED	(0x0200 | FZ_REPLY_ERROR) // Will be returned by connect if already connected

// Small macro to simplify command class declaration
// Basically all this macro does, is to declare the class and add the required
// standard functions to it.
#define DECLARE_COMMAND(name, id, chainable) \
	class name : public CCommand \
	{ \
	public: \
		virtual enum Command GetId() const { return id; } \
		virtual bool IsChainable() const { return chainable; } \
		virtual CCommand* Clone() const { return new name(*this); }

class CCommand
{
public:
	CCommand() { }
	virtual ~CCommand() { }
	virtual enum Command GetId() const = 0;
	virtual bool IsChainable() const = 0;
	virtual CCommand *Clone() const = 0;
};

DECLARE_COMMAND(CConnectCommand, cmd_connect, false)
	CConnectCommand(const CServer &server);
	virtual ~CConnectCommand() { }

	const CServer GetServer() const;
protected:
	CServer m_Server;
};

DECLARE_COMMAND(CDisconnectCommand, cmd_disconnect, false)
};

DECLARE_COMMAND(CCancelCommand, cmd_cancel, false)
};

DECLARE_COMMAND(CListCommand, cmd_list, false)
	CListCommand(bool refresh = false);
	CListCommand(CServerPath path, wxString subDir = _T(""), bool refresh = false);
	virtual ~CListCommand() { }
	
	CServerPath GetPath() const;
	wxString GetSubDir() const;

	bool Refresh() const;

protected:
	CServerPath m_path;
	wxString m_subDir;
	bool m_refresh;
};

DECLARE_COMMAND(CFileTransferCommand, cmd_transfer, false)
	CFileTransferCommand();
	CFileTransferCommand(const wxString &localFile, const CServerPath& remotePath, const wxString &remoteFile, bool download);
	virtual ~CFileTransferCommand() { }

	wxString GetLocalFile() const;
	CServerPath GetRemotePath() const;
	wxString GetRemoteFile() const;
	bool Download() const;

protected:
	wxString m_localFile;
	CServerPath m_remotePath;
	wxString m_remoteFile;
	bool m_download;
};

DECLARE_COMMAND(CRawCommand, cmd_raw, false)
	CRawCommand();
	CRawCommand(const wxString &command);
	virtual ~CRawCommand() { }

	wxString GetCommand() const;

protected:
	wxString m_command;
};

DECLARE_COMMAND(CDeleteCommand, cmd_delete, true)
	CDeleteCommand(const CServerPath& path, const wxString& file);
	virtual ~CDeleteCommand() { }

	CServerPath GetPath() const { return m_path; }
	wxString GetFile() const { return m_file; }

protected:

	CServerPath m_path;
	wxString m_file;
};

DECLARE_COMMAND(CRemoveDirCommand, cmd_removedir, true)
	CRemoveDirCommand(const CServerPath& path, const wxString& subdDir);
	virtual ~CRemoveDirCommand() { }

	CServerPath GetPath() const { return m_path; }
	wxString GetSubDir() const { return m_subDir; }

protected:

	CServerPath m_path;
	wxString m_subDir;
};

#endif
