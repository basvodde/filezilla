#include "FileZilla.h"

CCommand::~CCommand()
{
}

CConnectCommand::CConnectCommand(const CServer &server)
{
	m_Server = server;
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
