
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/scrolwin.h"

wxScrollHelper::wxScrollHelper(wxWindow *winToScroll)
{
	FAIL("wxScrollHelper::wxScrollHelper");
}

wxScrollHelper::~wxScrollHelper()
{
	FAIL("wxScrollHelper::~wxScrollHelper");
}

void wxScrollHelper::SetScrollRate( int xstep, int ystep )
{
	FAIL("wxScrollHelper::SetScrollRate");
}

wxSize wxScrollHelper::ScrollGetWindowSizeForVirtualSize(const wxSize& size) const
{
	FAIL("wxScrollHelper::ScrollGetWindowSizeForVirtualSize");
	return wxSize();
}

void wxScrollHelper::DoCalcScrolledPosition(int x, int y, int *xx, int *yy) const
{
	FAIL("wxScrollHelper::DoCalcScrolledPosition");
}

void wxScrollHelper::DoCalcUnscrolledPosition(int x, int y, int *xx, int *yy) const
{
	FAIL("wxScrollHelper::DoCalcUnscrolledPosition");
}

void wxScrollHelper::AdjustScrollbars(void)
{
	FAIL("wxScrollHelper::AdjustScrollbars");
}

int wxScrollHelper::CalcScrollInc(wxScrollWinEvent& event)
{
	FAIL("wxScrollHelper::CalcScrollInc");
	return 0;
}

void wxScrollHelper::SetTargetWindow(wxWindow *target)
{
	FAIL("wxScrollHelper::SetTargetWindow");
}

wxWindow *wxScrollHelper::GetTargetWindow() const
{
	FAIL("wxScrollHelper::GetTargetWindow");
	return NULL;
}

void wxScrollHelper::DoPrepareDC(wxDC& dc)
{
	FAIL("wxScrollHelper::DoPrepareDC");
}

bool wxScrollHelper::ScrollLayout()
{
	FAIL("wxScrollHelper::ScrollLayout");
	return true;
}

void wxScrollHelper::SetScrollbars(int pixelsPerUnitX, int pixelsPerUnitY, int noUnitsX, int noUnitsY,
		int xPos, int yPos, bool noRefresh)
{
	FAIL("wxScrollHelper::SetScrollbars");
}

void wxScrollHelper::Scroll(int x, int y)
{
	FAIL("wxScrollHelper::Scroll");
}

void wxScrollHelper::ScrollDoSetVirtualSize(int x, int y)
{
	FAIL("wxScrollHelper::ScrollDoSetVirtualSize");
}

void wxScrollHelper::GetScrollPixelsPerUnit(int *pixelsPerUnitX, int *pixelsPerUnitY) const
{
	FAIL("wxScrollHelper::GetScrollPixelsPerUnit");
}

void wxScrollHelper::EnableScrolling(bool x_scrolling, bool y_scrolling)
{
	FAIL("wxScrollHelper::EnableScrolling");
}

void wxScrollHelper::GetViewStart(int *x, int *y) const
{
	FAIL("wxScrollHelper::GetViewStart");
}

bool wxScrollHelper::SendAutoScrollEvents(wxScrollWinEvent& event) const
{
	FAIL("wxScrollHelper::SendAutoScrollEvents");
	return true;
}

wxSize wxScrollHelper::ScrollGetBestVirtualSize() const
{
	FAIL("wxScrollHelper::ScrollGetBestVirtualSize");
	return wxSize();
}

wxScrolledWindow::~wxScrolledWindow()
{
	FAIL("wxScrolledWindow::~wxScrolledWindow");
}

wxClassInfo *wxScrolledWindow::GetClassInfo() const
{
	FAIL("wxScrolledWindow::GetClassInfo");
	return NULL;
}

const wxEventTable wxScrolledWindow::sm_eventTable = wxEventTable();
wxEventHashTable wxScrolledWindow::sm_eventHashTable (wxScrolledWindow::sm_eventTable);

const wxEventTable* wxScrolledWindow::GetEventTable() const
{
	FAIL("wxScrolledWindow::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxScrolledWindow::GetEventHashTable() const
{
	FAIL("wxScrolledWindow::GetEventHashTable");
	return sm_eventHashTable;
}

bool wxScrolledWindow::Create(wxWindow *parent, wxWindowID winid, const wxPoint& pos,
		const wxSize& size, long style, const wxString& name)
{
	FAIL("wxScrolledWindow::Create");
	return true;
}

