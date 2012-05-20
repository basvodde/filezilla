
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/dcclient.h"

wxWindowDC::wxWindowDC()
{
	FAIL("wxWindowDC::wxWindowDC");
}

void wxWindowDC::DoGetSize( int *width, int *height ) const
{
	FAIL("wxWindowDC::DoGetSize");
}

wxBitmap wxWindowDC::DoGetAsBitmap(const wxRect *subrect) const
{
	FAIL("wxWindowDC::DoGetAsBitmap");
	return wxBitmap();
}

wxClassInfo *wxWindowDC::GetClassInfo() const
{
	FAIL("wxWindowDC::GetClassInfo");
	return NULL;
}

wxWindowDC::~wxWindowDC()
{
	FAIL("wxWindowDC::~wxWindowDC");
}

wxClassInfo *wxPaintDC::GetClassInfo() const
{
	FAIL("wxPaintDC::GetClassInfo");
	return NULL;
}

wxPaintDC::wxPaintDC()
{
	FAIL("wxPaintDC::wxPaintDC");
}

wxPaintDC::wxPaintDC(wxWindow *win)
{
	FAIL("wxPaintDC::wxPaintDC");
}

wxPaintDC::~wxPaintDC()
{
	FAIL("wxPaintDC::~wxPaintDC");
}
