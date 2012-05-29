
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/palette.h"

wxPalette::wxPalette()
{
}

wxPalette::~wxPalette()
{
}

wxClassInfo *wxPalette::GetClassInfo() const
{
	FAIL("wxPalette::GetClassInfo");
	return NULL;
}

int wxPalette::GetColoursCount() const
{
	FAIL("wxPalette::GetColoursCount");
	return 1;
}
