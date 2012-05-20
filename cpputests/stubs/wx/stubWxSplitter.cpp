
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/splitter.h"

const wxEventTable wxSplitterWindow::sm_eventTable = wxEventTable();
wxEventHashTable wxSplitterWindow::sm_eventHashTable (wxSplitterWindow::sm_eventTable);

const wxEventTable* wxSplitterWindow::GetEventTable() const
{
	FAIL("wxSplitterWindow::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxSplitterWindow::GetEventHashTable() const
{
	FAIL("wxSplitterWindow::GetEventHashTable");
	return sm_eventHashTable;
}

void wxSplitterWindow::OnChildFocus(wxChildFocusEvent& event)
{
	FAIL("wxSplitterWindow::OnChildFocus");
}

void wxSplitterWindow::SetFocusIgnoringChildren()
{
	FAIL("wxSplitterWindow::SetFocusIgnoringChildren");
}

void wxSplitterWindow::SetFocus()
{
	FAIL("wxSplitterWindow::SetFocus");
}

void wxSplitterWindow::RemoveChild(wxWindowBase *child)
{
	FAIL("wxSplitterWindow::RemoveChild");
}

bool wxSplitterWindow::AcceptsFocus() const
{
	FAIL("wxSplitterWindow::AcceptsFocus");
	return true;
}

void wxSplitterWindow::SetSashGravity(double gravity)
{
	FAIL("wxSplitterWindow::SetSashGravity");
}

wxSplitterWindow::~wxSplitterWindow()
{
	FAIL("wxSplitterWindow::~wxSplitterWindow");
}

void wxSplitterWindow::OnEnterSash()
{
	FAIL("wxSplitterWindow::OnEnterSash");
}

void wxSplitterWindow::OnLeaveSash()
{
	FAIL("wxSplitterWindow::OnLeaveSash");
}

wxSize wxSplitterWindow::DoGetBestSize() const
{
	FAIL("wxSplitterWindow::DoGetBestSize");
	return wxSize();
}

int wxSplitterWindow::OnSashPositionChanging(int newSashPosition)
{
	FAIL("wxSplitterWindow::OnSashPositionChanging");
	return 0;
}

bool wxSplitterWindow::OnSashPositionChange(int newSashPosition)
{
	FAIL("wxSplitterWindow::OnSashPositionChange");
	return true;
}

void wxSplitterWindow::OnUnsplit(wxWindow *removed)
{
	FAIL("wxSplitterWindow::OnUnsplit");
}

void wxSplitterWindow::OnDoubleClickSash(int x, int y)
{
	FAIL("wxSplitterWindow::OnDoubleClickSash");
}

wxClassInfo *wxSplitterWindow::GetClassInfo() const
{
	FAIL("wxSplitterWindow::GetClassInfo");
	return NULL;
}

void wxSplitterWindow::OnInternalIdle()
{
	FAIL("wxSplitterWindow::OnInternalIdle");
}

bool wxSplitterWindow::DoSplit(wxSplitMode mode, wxWindow *window1, wxWindow *window2, int sashPosition)
{
	FAIL("wxSplitterWindow::DoSplit");
	return true;
}

void wxSplitterWindow::DrawSashTracker(int x, int y)
{
	FAIL("wxSplitterWindow::DrawSashTracker");
}

bool wxSplitterWindow::SashHitTest(int x, int y, int tolerance)
{
	FAIL("wxSplitterWindow::SashHitTest");
	return true;
}

void wxSplitterWindow::SizeWindows()
{
	FAIL("wxSplitterWindow::SizeWindows");
}

void wxSplitterWindow::DrawSash(wxDC& dc)
{
	FAIL("wxSplitterWindow::DrawSash");
}



