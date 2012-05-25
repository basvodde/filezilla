
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/defs.h"
#include "wx/utils.h"

long wxNewId()
{
	FAIL("wxNewId");
	return 0;
}

wxString wxStripMenuCodes(const wxString& str, int flags)
{
	FAIL("wxStripMenuCodes");
	return L"";
}

bool wxGetKeyState(wxKeyCode key)
{
	FAIL("wxGetKeyState");
	return true;
}

wxMouseState wxGetMouseState()
{
	FAIL("wxGetMouseState");
	return wxMouseState();
}
