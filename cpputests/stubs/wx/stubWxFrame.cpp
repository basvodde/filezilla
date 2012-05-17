
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/frame.h"

void wxFrame::DoGetClientSize(int *width, int *height) const
{
	FAIL("wxFrame::DoGetClientSize");
}
