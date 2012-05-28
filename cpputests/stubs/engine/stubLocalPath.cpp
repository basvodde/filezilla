
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "local_path.h"

const wxChar CLocalPath::path_separator = L'l';


CLocalPath::CLocalPath(const CLocalPath &path)
{
	FAIL("CLocalPath::CLocalPath");
}

CLocalPath::CLocalPath(const wxString& path, wxString* file)
{
	FAIL("CLocalPath::CLocalPath");
}

bool CLocalPath::SetPath(const wxString& path, wxString* file)
{
	FAIL("CLocalPath::SetPath");
	return true;
}

bool CLocalPath::empty() const
{
	FAIL("CLocalPath::empty");
	return true;
}

void CLocalPath::clear()
{
	FAIL("CLocalPath::clear");
}

bool CLocalPath::ChangePath(const wxString& path)
{
	FAIL("CLocalPath::ChangePath");
	return true;
}

void CLocalPath::AddSegment(const wxString& segment)
{
	FAIL("CLocalPath::AddSegment");
}

bool CLocalPath::HasParent() const
{
	FAIL("CLocalPath::HasParent");
	return true;
}

bool CLocalPath::HasLogicalParent() const
{
	FAIL("CLocalPath::HasLogicalParent");
	return true;
}

CLocalPath CLocalPath::GetParent(wxString* last_segment) const
{
	FAIL("CLocalPath::GetParent");
	return *this;
}

bool CLocalPath::MakeParent(wxString* last_segment)
{
	FAIL("CLocalPath::MakeParent");
	return true;
}

wxString CLocalPath::GetLastSegment() const
{
	FAIL("CLocalPath::GetLastSegment");
	return L"";
}

bool CLocalPath::IsSubdirOf(const CLocalPath &path) const
{
	FAIL("CLocalPath::IsSubdirOf");
	return true;
}

bool CLocalPath::IsParentOf(const CLocalPath &path) const
{
	FAIL("CLocalPath::IsParentOf");
	return true;
}

bool CLocalPath::IsWriteable() const
{
	FAIL("CLocalPath::IsWriteable");
	return true;
}

bool CLocalPath::Exists(wxString *error) const
{
	FAIL("CLocalPath::Exists");
	return true;
}

bool CLocalPath::operator==(const CLocalPath& op) const
{
	FAIL("CLocalPath::operator==");
	return true;
}

bool CLocalPath::operator!=(const CLocalPath& op) const
{
	FAIL("CLocalPath::operator~=");
	return true;
}

void CLocalPath::Coalesce()
{
	FAIL("CLocalPath::Coalesce");
}

