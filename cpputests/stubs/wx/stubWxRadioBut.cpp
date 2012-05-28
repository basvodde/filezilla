
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/radiobut.h"

IMPLEMENT_DYNAMIC_CLASS(wxRadioButton, wxControl);

wxRadioButton::~wxRadioButton()
{
	FAIL("wxRadioButton::~wxRadioButton");
}

bool wxRadioButton::Create(wxWindow *parent, wxWindowID id, const wxString& label, const wxPoint& pos,
		const wxSize& size, long style, const wxValidator& validator, const wxString& name)
{
	FAIL("wxRadioButton::Create");
	return true;
}

void wxRadioButton::SetValue(bool val)
{
	FAIL("wxRadioButton::SetValue");
}

bool wxRadioButton::GetValue() const
{
	FAIL("wxRadioButton::GetValue");
	return true;
}

wxInt32 wxRadioButton::MacControlHit( WXEVENTHANDLERREF handler , WXEVENTREF event )
{
	FAIL("wxRadioButton::MacControlHit");
	return 0;
}

void wxRadioButton::Command(wxCommandEvent& event)
{
	FAIL("wxRadioButton::Command");
}

wxRadioButton * wxRadioButton::AddInCycle(wxRadioButton *cycle)
{
	FAIL("wxRadioButton::AddInCycle");
	return NULL;
}

void wxRadioButton::RemoveFromCycle()
{
	FAIL("wxRadioButton::RemoveFromCycle");
}

