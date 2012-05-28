
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "commands.h"


CRenameCommand::CRenameCommand(const CServerPath& fromPath, const wxString& fromFile,
		const CServerPath& toPath, const wxString& toFile)
{
	FAIL("CRenameCommand::CRenameCommand");
}

CRemoveDirCommand::CRemoveDirCommand(const CServerPath& path, const wxString& subdDir)
{
	FAIL("CRemoveDirCommand::CRemoveDirCommand");
}


CServerPath CListCommand::GetPath() const
{
	FAIL("CListCommand::GetPath");
	return CServerPath();
}

wxString CListCommand::GetSubDir() const
{
	FAIL("CListCommand::GetSubDir");
	return L"";
}

CListCommand::CListCommand(int flags)
{
	FAIL("CListCommand::CListCommand");
}

CListCommand::CListCommand(CServerPath path, wxString subDir, int flags)
{
	FAIL("CListCommand::CListCommand");
}

CChmodCommand::CChmodCommand(const CServerPath& path, const wxString& file, const wxString& permission)
{
	FAIL("CChmodCommand::CChmodCommand");
}

CFileTransferCommand::CFileTransferCommand(const wxString &localFile, const CServerPath& remotePath, const wxString &remoteFile, bool download, const t_transferSettings& m_transferSettings)
{
	FAIL("CFileTransferCommand::CFileTransferCommand");
}

CDeleteCommand::CDeleteCommand(const CServerPath& path, const std::list<wxString>& files)
{
	FAIL("CDeleteCommand::CDeleteCommand");
}

CConnectCommand::CConnectCommand(const CServer &server, bool retry_conncting)
{
	FAIL("CConnectCommand::CConnectCommand");
}

CRawCommand::CRawCommand(const wxString &command)
{
	FAIL("CRawCommand::CRawCommand");
}

CMkdirCommand::CMkdirCommand(const CServerPath& path)
{
	FAIL("CMkdirCommand::CMkdirCommand");
}
