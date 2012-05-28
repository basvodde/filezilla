
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "local_filesys.h"


bool CLocalFileSystem::BeginFindFiles(wxString path, bool dirs_only)
{
	FAIL("CLocalFileSystem::BeginFindFiles");
	return true;
}

CLocalFileSystem::CLocalFileSystem()
{
	FAIL("CLocalFileSystem::CLocalFileSystem");
}

CLocalFileSystem::~CLocalFileSystem()
{
	FAIL("CLocalFileSystem::~CLocalFileSystem");
}

enum CLocalFileSystem::local_fileType CLocalFileSystem::GetFileInfo(const wxString& path, bool &isLink, wxLongLong* size, wxDateTime* modificationTime, int* mode)
{
	FAIL("CLocalFileSystem::GetFileInfo");
	return CLocalFileSystem::unknown;
}

enum CLocalFileSystem::local_fileType CLocalFileSystem::GetFileType(const wxString& path)
{
	FAIL("CLocalFileSystem::GetFileInfo");
	return CLocalFileSystem::unknown;
}

bool CLocalFileSystem::GetNextFile(wxString& name, bool &isLink, bool &is_dir, wxLongLong* size, wxDateTime* modificationTime, int* mode)
{
	FAIL("CLocalFileSystem::GetNextFile");
	return true;
}

bool CLocalFileSystem::RecursiveDelete(const wxString& path, wxWindow* parent)
{
	FAIL("CLocalFileSystem::RecursiveDelete");
	return true;
}

bool CLocalFileSystem::RecursiveDelete(std::list<wxString> dirsToVisit, wxWindow* parent)
{
	FAIL("CLocalFileSystem::RecursiveDelete");
	return true;

}

const wxChar CLocalFileSystem::path_separator = L'L';
