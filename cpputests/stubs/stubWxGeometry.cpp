
#include "CppUTest/TestHarness.h"
#include <wx/geometry.h>

wxRect2DInt& wxRect2DInt::operator = (const wxRect2DInt& rect)
{
	FAIL("wxRect2DInt called");
}
