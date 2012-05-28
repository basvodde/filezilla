
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "misc.h"

bool IsIpAddress(const wxString& address)
{
	FAIL("IsIpAddress");
	return true;
}

int GetRandomNumber(int low, int high)
{
	FAIL("GetRandomNumber");
	return 0;
}

void MakeLowerAscii(wxString& str)
{
	FAIL("MakeLowerAscii");
}

wxString GetDependencyVersion(enum _dependency dependency)
{
	FAIL("GetDependencyVersion");
	return L"";
}
