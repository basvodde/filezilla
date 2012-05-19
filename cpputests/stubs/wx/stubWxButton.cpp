
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/button.h"

wxClassInfo *wxButton::GetClassInfo() const
{
	FAIL("wxButton::GetClassInfo");
	return NULL;
}

wxInt32 wxButton::MacControlHit( WXEVENTHANDLERREF handler , WXEVENTREF event )
{
	FAIL("wxButton::MacControlHit");
	return 0;
}

void wxButton::SetDefault()
{
	FAIL("wxButton::SetDefault");
}

wxSize wxButton::DoGetBestSize() const
{
	FAIL("wxButton::DoGetBestSize");
	return wxSize();
}

void wxButton::Command(wxCommandEvent& event)
{
	FAIL("wxButton::Command");
}
