#include "FileZilla.h"

CCommand::CCommand()
{
}

CCommand::~CCommand()
{
}

CConnectCommand::CConnectCommand(const CServer &server)
{
	m_Server = server;
}

CConnectCommand::~CConnectCommand()
{
}

enum Command CConnectCommand::GetId() const
{
	return cmd_connect;
}

bool CConnectCommand::IsChainable() const
{
	return false;
}

CCommand *CConnectCommand::Clone() const
{
	return new CConnectCommand(GetServer());
}

const CServer CConnectCommand::GetServer() const
{
	return m_Server;
}

enum Command CDisconnectCommand::GetId() const
{
	return cmd_disconnect;
}

bool CDisconnectCommand::IsChainable() const
{
	return false;
}

CCommand *CDisconnectCommand::Clone() const
{
	return new CDisconnectCommand();
}

enum Command CCancelCommand::GetId() const
{
	return cmd_cancel;
}

bool CCancelCommand::IsChainable() const
{
	return false;
}

CCommand *CCancelCommand::Clone() const
{
	return new CCancelCommand();
}

CListCommand::CListCommand()
{
}

CListCommand::CListCommand(CServerPath path, wxString subDir /*=_T("")*/)
{
	m_path = path;
	m_subDir = subDir;
}

CListCommand::~CListCommand()
{
}

CServerPath CListCommand::GetPath() const
{
	return m_path;
}

wxString CListCommand::GetSubDir() const
{
	return m_subDir;
}

enum Command CListCommand::GetId() const
{
	return cmd_list;
}

bool CListCommand::IsChainable() const
{
	return false;
}

CCommand *CListCommand::Clone() const
{
	return new CListCommand(m_path, m_subDir);
}
