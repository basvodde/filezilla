#ifndef __COMMANDS_H__
#define __COMMANDS_H__

enum Command
{
	Connect = 0,
	Disconnect,
	Cancel,
	List,
	RecList, // Recursive list
	Transfer,
	Delete,
	RemoveDir,
	MakeDir,
	Rename,
	Chmod,
	Raw
};

#define FZ_REPLY_OK				(0x0001)
#define FZ_REPLY_ERROR			(0x0002)
#define FZ_REPLY_CRITICALERROR	(0x0004 | FZ_REPLY_ERROR)
#define FZ_REPLY_CANCELED		(0x0008 | FZ_REPLY_ERROR)
#define FZ_REPLY_SYNTAXERROR	(0x0010 | FZ_REPLY_ERROR)
#define FZ_REPLY_NOTCONNECTED	(0x0020 | FZ_REPLY_ERROR)
#define FZ_REPLY_DISCONNECTED	(0x0040)
#define FZ_REPLY_INTERNALERROR	(0x0080)

class CCommand
{
public:
	virtual ~CCommand();
	virtual enum Command GetId() const = 0;
	virtual bool IsChainable() const = 0;
};

class CConnectCommand : public CCommand
{
public:
	CConnectCommand(const CServer &server);
	virtual enum Command GetId() const;
	virtual bool IsChainable() const;
	CServer GetServer() const;
protected:
	CServer m_Server;
};

class CDisconnectCommand : public CCommand
{
public:
	virtual enum Command GetId() const;
	virtual bool IsChainable() const;
};

class CCancelCommand : public CCommand
{
public:
	virtual enum Command GetId() const;
	virtual bool IsChainable() const;
};

#endif

