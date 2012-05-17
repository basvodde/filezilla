
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/gdiobj.h"

wxClassInfo *wxGDIObject::GetClassInfo() const
{
	FAIL("wxGDIObject::GetClassInfo");
	return NULL;
}
