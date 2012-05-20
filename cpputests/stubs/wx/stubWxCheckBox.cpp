

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/checkbox.h"

void wxCheckBox::SetValue(bool)
{
	FAIL("wxCheckBox::SetValue");
}

bool wxCheckBox::GetValue() const
{
	FAIL("wxCheckBox::GetValue");
	return true;
}

wxInt32 wxCheckBox::MacControlHit( WXEVENTHANDLERREF handler , WXEVENTREF event )
{
	FAIL("wxCheckBox::MacControlHit");
	return 0;
}

void wxCheckBox::Command(wxCommandEvent& event)
{
	FAIL("wxCheckBox::Command");
}

wxClassInfo *wxCheckBox::GetClassInfo() const
{
	FAIL("wxCheckBox::GetClassInfo");
	return NULL;
}

void wxCheckBox::DoSet3StateValue(wxCheckBoxState val)
{
	FAIL("wxCheckBox::DoSet3StateValue");
}

wxCheckBoxState wxCheckBox::DoGet3StateValue() const
{
	FAIL("wxCheckBox::DoGet3StateValue");
	return wxCheckBoxState();
}


