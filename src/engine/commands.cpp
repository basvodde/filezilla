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
	return Connect;
}

bool CConnectCommand::IsChainable() const
{
	return false;
}
CServer CConnectCommand::GetServer() const
{
	return m_Server;
}

enum Command CDisconnectCommand::GetId() const
{
	return Disconnect;
}

bool CDisconnectCommand::IsChainable() const
{
	return false;
}

enum Command CCancelCommand::GetId() const
{
	return Cancel;
}

bool CCancelCommand::IsChainable() const
{
	return false;
}
