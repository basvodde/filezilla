

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/pen.h"

wxPen::wxPen()
{
	FAIL("wxPen::wxPen");
}

wxPen::~wxPen()
{
	FAIL("wxPen::~wxPen");
}

wxClassInfo *wxPen::GetClassInfo() const
{
	FAIL("wxPen::GetClassInfo");
	return NULL;
}

wxPen::wxPen(const wxColour& col, int width, int style)
{
	FAIL("wxPen::wxPen");
}
