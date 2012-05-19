
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/splitter.h"

void wxSplitterWindow::SetSashGravity(double gravity)
{
	FAIL("wxSplitterWindow::SetSashGravity");
}
