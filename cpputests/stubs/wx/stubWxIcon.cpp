
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/icon.h"

wxIcon::wxIcon()
{
	FAIL("wxIcon::wxIcon");
}

wxIcon::~wxIcon()
{
	FAIL("wxIcon::~wxIcon");
}

wxClassInfo *wxIcon::GetClassInfo() const
{
	FAIL("wxIcon::GetClassInfo");
	return NULL;
}

