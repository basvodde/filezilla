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

enum Command CCancelCommand::GetId() const
{
	return cmd_cancel;
}

bool CCancelCommand::IsChainable() const
{
	return false;
}
