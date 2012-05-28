
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/cursor.h"

wxCursor::wxCursor()
{
}

wxCursor::~wxCursor()
{
}

bool wxCursor::IsOk() const
{
	FAIL("wxCursor::IsOk");
	return true;
}

wxClassInfo *wxCursor::GetClassInfo() const
{
	FAIL("wxCursor::GetClassInfo");
	return NULL;
}

