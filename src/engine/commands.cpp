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

CListCommand::CListCommand(bool refresh /*=false*/)
	: m_refresh(refresh)
{
}

CListCommand::CListCommand(CServerPath path, wxString subDir /*=_T("")*/, bool refresh /*=false*/)
	: m_refresh(refresh)
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
	return new CListCommand(m_path, m_subDir, m_refresh);
}

bool CListCommand::Refresh() const
{
	return m_refresh;
}

CFileTransferCommand::CFileTransferCommand()
{
}

CFileTransferCommand::CFileTransferCommand(const wxString &localFile, const CServerPath& remotePath, const wxString &remoteFile, bool download)
{
	m_localFile = localFile;
	m_remotePath = remotePath;
	m_remoteFile = remoteFile;
	m_download = download;
}

CFileTransferCommand::~CFileTransferCommand()
{
}

enum Command CFileTransferCommand::GetId() const
{
	return cmd_transfer;
}

bool CFileTransferCommand::IsChainable() const
{
	return false;
}

CCommand* CFileTransferCommand::Clone() const
{
	return new CFileTransferCommand(m_localFile, m_remotePath, m_remoteFile, m_download);
}

wxString CFileTransferCommand::GetLocalFile() const
{
	return m_localFile;
}

CServerPath CFileTransferCommand::GetRemotePath() const
{
	return m_remotePath;
}

wxString CFileTransferCommand::GetRemoteFile() const
{
	return m_remoteFile;
}

bool CFileTransferCommand::Download() const
{
	return m_download;
}
