
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/defs.h"
#include "wx/geometry.h"
#include "wx/display.h"

wxDisplay::wxDisplay(unsigned n)
{
	FAIL("wxDisplay::wxDisplay");
}

wxDisplay::~wxDisplay()
{
	FAIL("wxDisplay::~wxDisplay");
}

wxRect wxDisplay::GetGeometry() const
{
	FAIL("wxDisplay::GetGeometry");
	return wxRect();
}

int wxDisplay::GetFromWindow(wxWindow *window)
{
	FAIL("wxDisplay::GetFromWindow");
	return 0;
}

unsigned wxDisplay::GetCount()
{
	FAIL("wxDisplay::GetCount");
	return 0;
}

wxRect wxDisplay::GetClientArea() const
{
	FAIL("wxDisplay::GetClientArea");
	return wxRect();
}


