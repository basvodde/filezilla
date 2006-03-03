#ifndef __COMMANDS_H__
#define __COMMANDS_H__

// See below for actual commands and their parameters

// Command IDs
// -----------
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
	cmd_mkdir,
	cmd_rename,
	cmd_chmod,
	cmd_raw,
	cmd_private // only used internally
};

// Reply codes
// -----------
#define FZ_REPLY_OK				(0x0000)
#define FZ_REPLY_WOULDBLOCK		(0x0001)
#define FZ_REPLY_ERROR			(0x0002)
#define FZ_REPLY_CRITICALERROR	(0x0004 | FZ_REPLY_ERROR) // If there is no point to retry an operation, this
														  // code is returned.
#define FZ_REPLY_CANCELED		(0x0008 | FZ_REPLY_ERROR)
#define FZ_REPLY_SYNTAXERROR	(0x0010 | FZ_REPLY_ERROR)
#define FZ_REPLY_NOTCONNECTED	(0x0020 | FZ_REPLY_ERROR)
#define FZ_REPLY_DISCONNECTED	(0x0040)
#define FZ_REPLY_INTERNALERROR	(0x0080 | FZ_REPLY_ERROR) // If you get this reply, the error description will be 
														  // given by the last Debug_Warning log message. This
														  // should not happen unless there is a bug in FileZilla 3.
#define FZ_REPLY_BUSY			(0x0100 | FZ_REPLY_ERROR)
#define FZ_REPLY_ALREADYCONNECTED	(0x0200 | FZ_REPLY_ERROR) // Will be returned by connect if already connected
#define FZ_REPLY_PASSWORDFAILED	0x0400 // Will be returned if PASS fails with 5yz reply code.

// Small macro to simplify command class declaration
// Basically all this macro does, is to declare the class and add the required
// standard functions to it.
#define DECLARE_COMMAND(name, id) \
	class name : public CCommand \
	{ \
	public: \
		virtual enum Command GetId() const { return id; } \
		virtual CCommand* Clone() const { return new name(*this); }

// --------------- //
// Actual commands //
// --------------- //

class CCommand
{
public:
	CCommand() { }
	virtual ~CCommand() { }
	virtual enum Command GetId() const = 0;
	virtual CCommand *Clone() const = 0;
};

DECLARE_COMMAND(CConnectCommand, cmd_connect)
	CConnectCommand(const CServer &server);

	const CServer GetServer() const;
protected:
	CServer m_Server;
};

DECLARE_COMMAND(CDisconnectCommand, cmd_disconnect)
};

DECLARE_COMMAND(CCancelCommand, cmd_cancel)
};

DECLARE_COMMAND(CListCommand, cmd_list)
    // Without a given directory, the current directory will be listed.
    // Directories can either be given as absolute path or as
    // pair of an absolute path and the very last path segments.
	// Set refresh to true to get a directory listing even if a cache
	// lookup can be made after finding out true remote directory.
	CListCommand(bool refresh = false);
	CListCommand(CServerPath path, wxString subDir = _T(""), bool refresh = false);
	
	CServerPath GetPath() const;
	wxString GetSubDir() const;

	bool Refresh() const;

protected:
	CServerPath m_path;
	wxString m_subDir;
	bool m_refresh;
};

DECLARE_COMMAND(CFileTransferCommand, cmd_transfer)

	struct t_transferSettings
	{
		bool binary;
	};

	// For uploads, set download to false.
	CFileTransferCommand(const wxString &localFile, const CServerPath& remotePath, const wxString &remoteFile, bool download, const t_transferSettings& m_transferSettings);

	wxString GetLocalFile() const;
	CServerPath GetRemotePath() const;
	wxString GetRemoteFile() const;
	bool Download() const;
	const t_transferSettings& GetTransferSettings() const { return m_transferSettings; }

protected:
	wxString m_localFile;
	CServerPath m_remotePath;
	wxString m_remoteFile;
	bool m_download;
	t_transferSettings m_transferSettings;
};

DECLARE_COMMAND(CRawCommand, cmd_raw)
	CRawCommand(const wxString &command);

	wxString GetCommand() const;

protected:
	wxString m_command;
};

DECLARE_COMMAND(CDeleteCommand, cmd_delete)
	CDeleteCommand(const CServerPath& path, const wxString& file);
	
	CServerPath GetPath() const { return m_path; }
	wxString GetFile() const { return m_file; }

protected:

	CServerPath m_path;
	wxString m_file;
};

DECLARE_COMMAND(CRemoveDirCommand, cmd_removedir)
    // Directories can either be given as absolute path or as
    // pair of an absolute path and the very last path segments.
	CRemoveDirCommand(const CServerPath& path, const wxString& subdDir);

	CServerPath GetPath() const { return m_path; }
	wxString GetSubDir() const { return m_subDir; }

protected:

	CServerPath m_path;
	wxString m_subDir;
};

DECLARE_COMMAND(CMkdirCommand, cmd_mkdir)
	CMkdirCommand(const CServerPath& path);

	CServerPath GetPath() const { return m_path; }

protected:

	CServerPath m_path;
};

DECLARE_COMMAND(CRenameCommand, cmd_rename)
	CRenameCommand(const CServerPath& fromPath, const wxString& fromFile,
				   const CServerPath& toPath, const wxString& toFile);

	CServerPath GetFromPath() const { return m_fromPath; }
	CServerPath GetToPath() const { return m_toPath; }
	wxString GetFromFile() const { return m_fromFile; }
	wxString GetToFile() const { return m_toFile; }

protected:
	CServerPath m_fromPath;
	CServerPath m_toPath;
	wxString m_fromFile;
	wxString m_toFile;
};

DECLARE_COMMAND(CChmodCommand, cmd_chmod)
	// The permission string should be given in a format understandable by the server.
	// Most likely it's the defaut octal representation used by the unix chmod command,
    // i.e. chmod 755 foo.bar
	CChmodCommand(const CServerPath& path, const wxString& file, const wxString& permission);

	CServerPath GetPath() const { return m_path; }
	wxString GetFile() const { return m_file; }
	wxString GetPermission() const { return m_permission; }

protected:
	CServerPath m_path;
	wxString m_file;
	wxString m_permission;
};

#endif
