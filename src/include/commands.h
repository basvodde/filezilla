#ifndef __COMMANDS_H__
#define __COMMANDS_H__

enum Command
{
	cmd_none = 0,
	cmd_connect,
	cmd_disconnect,
	cmd_cancel,
	cmd_list,
	cmd_recList, // Recursive list
	cmd_transfer,
	cmd_delete,
	cmd_removeDir,
	cmd_makeDir,
	cmd_rename,
	cmd_chmod,
	cmd_raw
};

#define FZ_REPLY_OK				(0x0000)
#define FZ_REPLY_WOULDBLOCK		(0x0001)
#define FZ_REPLY_ERROR			(0x0002)
#define FZ_REPLY_CRITICALERROR	(0x0004 | FZ_REPLY_ERROR)
#define FZ_REPLY_CANCELED		(0x0008 | FZ_REPLY_ERROR)
#define FZ_REPLY_SYNTAXERROR	(0x0010 | FZ_REPLY_ERROR)
#define FZ_REPLY_NOTCONNECTED	(0x0020 | FZ_REPLY_ERROR)
#define FZ_REPLY_DISCONNECTED	(0x0040)
#define FZ_REPLY_INTERNALERROR	(0x0080)
#define FZ_REPLY_BUSY			(0x0100 | FZ_REPLY_ERROR)
#define FZ_REPLY_ALREADYCONNECTED	(0x0200 | FZ_REPLY_ERROR) // Will be returned by connect if already connected

class CCommand
{
public:
	CCommand();
	virtual ~CCommand();
	virtual enum Command GetId() const = 0;
	virtual bool IsChainable() const = 0;
	virtual CCommand *Clone() const = 0;
};

class CConnectCommand : public CCommand
{
public:
	CConnectCommand(const CServer &server);
	virtual ~CConnectCommand();
	virtual enum Command GetId() const;
	virtual bool IsChainable() const;
	virtual CCommand *Clone() const;

	const CServer GetServer() const;
protected:
	CServer m_Server;
};

class CDisconnectCommand : public CCommand
{
public:
	virtual enum Command GetId() const;
	virtual bool IsChainable() const;
	virtual CCommand *Clone() const;
};

class CCancelCommand : public CCommand
{
public:
	virtual enum Command GetId() const;
	virtual bool IsChainable() const;
	virtual CCommand *Clone() const;
};

class CListCommand : public CCommand
{
public:
	CListCommand();
	CListCommand(CServerPath path, wxString subDir = _T(""));
	virtual ~CListCommand();
	virtual enum Command GetId() const;
	virtual bool IsChainable() const;
	virtual CCommand *Clone() const;
	
	CServerPath GetPath() const;
	wxString GetSubDir() const;

protected:
	CServerPath m_path;
	wxString m_subDir;
};

#endif

