
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/aui/auibook.h"

bool wxAuiNotebook::SetFont(const wxFont& font)
{
	FAIL("wxAuiNotebook::SetFont");
	return true;
}

void wxAuiDefaultTabArt::DrawButton(wxDC& dc, wxWindow* wnd, const wxRect& in_rect, int bitmap_id,
		int button_state, int orientation, wxRect* out_rect)
{
	FAIL("wxAuiDefaultTabArt::DrawButton");
}
