

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/checkbox.h"

IMPLEMENT_DYNAMIC_CLASS(wxCheckBox, wxCheckBoxBase);

const wxChar wxCheckBoxNameStr[100] = L"";

void wxCheckBox::SetValue(bool)
{
	FAIL("wxCheckBox::SetValue");
}

bool wxCheckBox::Create(wxWindow *parent, wxWindowID id, const wxString& label, const wxPoint& pos,
		const wxSize& size, long style, const wxValidator& validator, const wxString& name)
{
	FAIL("wxCheckBox::Create");
	return true;
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

void wxCheckBox::DoSet3StateValue(wxCheckBoxState val)
{
	FAIL("wxCheckBox::DoSet3StateValue");
}

wxCheckBoxState wxCheckBox::DoGet3StateValue() const
{
	FAIL("wxCheckBox::DoGet3StateValue");
	return wxCheckBoxState();
}


