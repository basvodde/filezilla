
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "serverpath.h"

CServerPathData::CServerPathData()
{
	FAIL("CServerPathData::CServerPathData");
}

CServerPath::CServerPath()
{
	FAIL("CServerPath::CServerPath")
}

CServerPath::CServerPath(wxString path, ServerType type)
{
	FAIL("CServerPath::CServerPath")
}

CServerPath::CServerPath(const CServerPath &path, wxString subdir)
{
	FAIL("CServerPath::CServerPath")
}

CServerPath::CServerPath(CServerPath const&)
{
	FAIL("CServerPath::CServerPath")
}

CServerPath::~CServerPath()
{
	FAIL("CServerPath::~CServerPath")
}

bool CServerPath::AddSegment(const wxString& segment)
{
	FAIL("CServerPath::AddSegment");
	return true;
}

bool CServerPath::ChangePath(wxString subdir)
{
	FAIL("CServerPath::ChangePath");
	return true;
}

bool CServerPath::ChangePath(wxString &subdir, bool isFile)
{
	FAIL("CServerPath::ChangePath");
	return true;
}

void CServerPath::Clear()
{
	FAIL("CServerPath::Clear");
}

wxString CServerPath::FormatFilename(const wxString &filename, bool omitPath) const
{
	FAIL("CServerPath::FormatFilename");
	return L"";
}

wxString CServerPath::GetLastSegment() const
{
	FAIL("CServerPath::GetLastSegment");
	return L"";
}

CServerPath CServerPath::GetParent() const
{
	FAIL("CServerPath::GetParent");
	return *this;
}

wxString CServerPath::GetPath() const
{
	FAIL("CServerPath::GetPath");
	return L"";
}

wxString CServerPath::GetSafePath() const
{
	FAIL("CServerPath::GetSafePath");
	return L"";
}

enum ServerType CServerPath::GetType() const
{
	FAIL("CServerPath::GetType");
	return DEFAULT;
}

bool CServerPath::HasParent() const
{
	FAIL("CServerPath::HasParent");
	return true;
}

bool CServerPath::IsSubdirOf(const CServerPath &path, bool cmpNoCase) const
{
	FAIL("CServerPath::IsSubdirOf");
	return true;
}

bool CServerPath::IsParentOf(const CServerPath &path, bool cmpNoCase) const
{
	FAIL("CServerPath::IsParentOf");
	return true;
}

bool CServerPath::SetPath(wxString newPath)
{
	FAIL("CServerPath::SetPath");
	return true;
}

bool CServerPath::SetPath(wxString &newPath, bool isFile)
{
	FAIL("CServerPath::SetPath");
	return true;
}

bool CServerPath::SetSafePath(const wxString& path, bool coalesce)
{
	FAIL("CServerPath::SetSafePath");
	return true;
}

bool CServerPath::SetType(enum ServerType type)
{
	FAIL("CServerPath::SetType");
	return true;
}

bool CServerPath::operator==(const CServerPath &op) const
{
	FAIL("CServerPath::operator==");
	return true;
}

bool CServerPath::operator!=(const CServerPath &op) const
{
	FAIL("CServerPath::operator!=");
	return true;
}

bool CServerPath::operator<(const CServerPath &op) const
{
	FAIL("CServerPath::operator<");
	return true;
}

