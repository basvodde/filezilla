#pragma once

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

class CCommand
{
public:
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