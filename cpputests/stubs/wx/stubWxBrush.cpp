
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/brush.h"

wxBrush::wxBrush()
{
	FAIL("wxBrush::wxBrush");
}

wxBrush::wxBrush(const wxColour& col, int style)
{
	FAIL("wxBrush::wxBrush");
}


wxBrush::~wxBrush()
{
	FAIL("wxBrush::~wxBrush");
}

wxClassInfo *wxBrush::GetClassInfo() const
{
	FAIL("wxBrush::GetClassInfo");
	return NULL;
}

void wxBrush::SetStyle(int style)
{
	FAIL("wxBrush::SetStyle");
}

void wxBrush::MacSetThemeBackground(unsigned long macThemeBackground ,  WXRECTPTR extent)
{
	FAIL("wxBrush::MacSetThemeBackground");
}

int wxBrush::GetStyle() const
{
	FAIL("wxBrush::GetStyle");
	return 0;
}

void wxBrush::SetColour(const wxColour& col)
{
	FAIL("wxBrush::SetColour");
}

void wxBrush::SetStipple(const wxBitmap& stipple)
{
	FAIL("wxBrush::SetStipple");
}

void wxBrush::MacSetTheme(short macThemeBrush)
{
	FAIL("wxBrush::MacSetTheme");
}

void wxBrush::SetColour(unsigned char r, unsigned char g, unsigned char b)
{
	FAIL("wxBrush::SetColour");
}
