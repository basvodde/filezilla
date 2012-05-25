
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/button.h"


wxClassInfo wxButton::ms_classInfo(NULL, NULL, NULL, 0, NULL);

const wxChar wxButtonNameStr[100] = L"";

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

bool wxButton::Create(wxWindow *parent, wxWindowID id, const wxString& label, const wxPoint& pos,
		const wxSize& size, long style, const wxValidator& validator, const wxString& name)
{
	FAIL("wxButton::Create");
	return true;
}

