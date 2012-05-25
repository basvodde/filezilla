

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/cursor.h"
#include "wx/gdicmn.h"

wxStockGDI* wxStockGDI::ms_instance = NULL;

wxCursor wxNullCursor;
const wxSize wxDefaultSize;
wxColour wxNullColour;
const wxPoint wxDefaultPosition;
const wxChar wxPanelNameStr[100] = L"";

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

const wxPen* wxStockGDI::GetPen(Item item)
{
	FAIL("wxStockGDI::wxPen");
	return NULL;
}

bool wxColourDisplay()
{
	FAIL("wxColourDisplay");
	return true;
}

wxRect& wxRect::Inflate(wxCoord dx, wxCoord dy)
{
	FAIL("wxRect::Inflate");
	return *this;
}

wxRect& wxRect::Union(const wxRect& rect)
{
	FAIL("wxRect::Union");
	return *this;
}

