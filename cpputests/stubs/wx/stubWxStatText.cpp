
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/stattext.h"

wxClassInfo *wxStaticText::GetClassInfo() const
{
	FAIL("wxStaticText::GetClassInfo");
	return NULL;
}

wxSize wxStaticText::DoGetBestSize() const
{
	FAIL("wxStaticText::DoGetBestSize");
	return wxSize();
}
