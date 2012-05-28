
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/control.h"

IMPLEMENT_ABSTRACT_CLASS(wxControl, wxControlBase);

wxControlBase::~wxControlBase()
{
	FAIL("wxControlBase::~wxControlBase");
}

void wxControlBase::DoUpdateWindowUI(wxUpdateUIEvent& event)
{
	FAIL("wxControlBase::DoUpdateWindowUI");
}

void wxControlBase::Command(wxCommandEvent &event)
{
	FAIL("wxControlBase::Command");
}

void wxControlBase::SetLabel( const wxString &label )
{
	FAIL("wxControlBase::SetLabel");
}

bool wxControlBase::SetFont(const wxFont& font)
{
	FAIL("wxControlBase::SetFont");
	return true;
}

bool wxControl::ProcessCommand(wxCommandEvent& event)
{
	FAIL("wxControl::ProcessCommand");
	return true;
}

wxControl::wxControl()
{
	FAIL("wxControl::wxControl");
}

wxControl::~wxControl()
{
	FAIL("wxControl::~wxControl");
}

