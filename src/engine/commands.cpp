#include "FileZilla.h"

CConnectCommand::CConnectCommand(const CServer &server)
{
	m_Server = server;
}

const CServer CConnectCommand::GetServer() const
{
	return m_Server;
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

CServerPath CListCommand::GetPath() const
{
	return m_path;
}

wxString CListCommand::GetSubDir() const
{
	return m_subDir;
}

bool CListCommand::Refresh() const
{
	return m_refresh;
}

CFileTransferCommand::CFileTransferCommand()
{
}

CFileTransferCommand::CFileTransferCommand(const wxString &localFile, const CServerPath& remotePath,
										   const wxString &remoteFile, bool download,
										   const CFileTransferCommand::t_transferSettings& transferSettings)
{
	m_localFile = localFile;
	m_remotePath = remotePath;
	m_remoteFile = remoteFile;
	m_download = download;
	m_transferSettings = transferSettings;
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

CRawCommand::CRawCommand(const wxString &command)
{
	m_command = command;
}

wxString CRawCommand::GetCommand() const
{
	return m_command;
}

CDeleteCommand::CDeleteCommand(const CServerPath& path, const wxString& file)
{
	m_path = path;
	m_file = file;
}

CRemoveDirCommand::CRemoveDirCommand(const CServerPath& path, const wxString& subDir)
{
	m_path = path;
	m_subDir = subDir;
}

CMkdirCommand::CMkdirCommand(const CServerPath& path)
{
	m_path = path;
}

CRenameCommand::CRenameCommand(const CServerPath& fromPath, const wxString& fromFile,
							   const CServerPath& toPath, const wxString& toFile)
{
	m_fromPath = fromPath;
	m_toPath = toPath;
	m_fromFile = fromFile;
	m_toFile = toFile;
}

CChmodCommand::CChmodCommand(const CServerPath& path, const wxString& file, const wxString& permission)
{
	m_path = path;
	m_file = file;
	m_permission = permission;
}
