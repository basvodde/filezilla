

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/defs.h"
#include "wx/containr.h"

wxControlContainer::wxControlContainer(wxWindow *winParent)
{
	FAIL("wxControlContainer::wxControlContainer");
}

