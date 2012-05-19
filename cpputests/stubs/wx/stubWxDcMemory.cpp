
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/dcmemory.h"

wxClassInfo *wxMemoryDC::GetClassInfo() const
{
	FAIL("wxMemoryDC::GetClassInfo");
	return NULL;
}

void wxMemoryDC::DoSelect(const wxBitmap& bitmap)
{
	FAIL("wxMemoryDC::DoSelect");
}

void wxMemoryDC::DoGetSize( int *width, int *height ) const
{
	FAIL("wxMemoryDC::DoGetSize");
}

wxMemoryDC::~wxMemoryDC()
{
	FAIL("wxMemoryDC::~wxMemoryDC");
}
