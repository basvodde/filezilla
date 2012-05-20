

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
