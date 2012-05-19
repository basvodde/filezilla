
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/listbase.h"

wxClassInfo *wxListEvent::GetClassInfo() const
{
	FAIL("wxListEvent::GetClassInfo");
	return NULL;
}

wxClassInfo *wxListItem::GetClassInfo() const
{
	FAIL("wxListItem::GetClassInfo");
	return NULL;
}

