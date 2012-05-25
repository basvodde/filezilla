
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/dnd.h"


wxDragResult wxDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
	FAIL("wxDropTarget::OnDragOver");
	return wxDragResult();
}

bool wxDropTarget::OnDrop(wxCoord x, wxCoord y)
{
	FAIL("wxDropTarget::OnDrop");
	return true;
}

wxDragResult wxDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
	FAIL("wxDropTarget::OnData");
	return wxDragResult();
}

bool wxDropTarget::GetData()
{
	FAIL("wxDropTarget::GetData");
	return true;
}

wxDropTarget::wxDropTarget(wxDataObject *dataObject)
{
	FAIL("wxDropTarget::wxDropTarget");
}

wxDragResult wxDropSource::DoDragDrop(int flags)
{
	FAIL("wxDropSource::DoDragDrop");
	return wxDragResult();
}

wxDropSource::wxDropSource( wxWindow *win, const wxCursor &cursorCopy, const wxCursor &cursorMove, const wxCursor &cursorStop)
{
	FAIL("wxDropSource::DoDragDrop");
}

wxDropSource::~wxDropSource()
{
	FAIL("wxDropSource::DoDragDrop");
}
