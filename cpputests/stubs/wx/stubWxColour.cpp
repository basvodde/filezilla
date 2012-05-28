
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/colour.h"

bool wxColourBase::FromString(const wxChar *s)
{
	FAIL("wxColourBase::FromString");
	return true;
}

wxString wxColourBase::GetAsString(long flags) const
{
	FAIL("wxColourBase::GetAsString");
	return L"";
}

wxColour::~wxColour()
{
}

void wxColour::Init()
{
}

bool wxColour::IsOk() const
{
	FAIL("wxColour::IsOk")
	return true;
}

void wxColour::InitRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	FAIL("wxColour::InitRGBA")
}

wxClassInfo *wxColour::GetClassInfo() const
{
	FAIL("wxColour::GetClassInfo");
	return NULL;
}
