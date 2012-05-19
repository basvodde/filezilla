
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/event.h"

wxClassInfo *wxSizeEvent::GetClassInfo() const
{
	FAIL("wxSizeEvent::GetClassInfo");
	return NULL;
}

wxClassInfo *wxNotifyEvent::GetClassInfo() const
{
	FAIL("wxNotifyEvent::GetClassInfo");
	return NULL;
}

wxClassInfo *wxNavigationKeyEvent::GetClassInfo() const
{
	FAIL("wxNavigationKeyEvent::GetClassInfo");
	return NULL;
}


