
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/clipbrd.h"


wxClipboard *wxClipboardBase::Get()
{
	FAIL("wxClipboardBase::Get");
	return NULL;
}
