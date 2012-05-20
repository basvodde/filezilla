

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/gdicmn.h"

const wxBrush* wxStockGDI::GetBrush(Item item)
{
	FAIL("wxStockGDI::GetBrush");
	return NULL;
}

const wxColour* wxStockGDI::GetColour(Item item)
{
	FAIL("wxStockGDI::GetColour");
	return NULL;
}

bool wxColourDisplay()
{
	FAIL("wxColourDisplay");
	return true;
}
