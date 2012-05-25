
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/spinctrl.h"


wxSpinCtrl::~wxSpinCtrl()
{
	FAIL("wxSpinCtrl::~wxSpinCtrl");
}

wxSize wxSpinCtrl::DoGetBestSize() const
{
	FAIL("wxSpinCtrl::DoGetBestSize");
	return wxSize();
}

void wxSpinCtrl::DoMoveWindow(int x, int y, int width, int height)
{
	FAIL("wxSpinCtrl::DoMoveWindow");
}

bool wxSpinCtrl::Enable(bool enable)
{
	FAIL("wxSpinCtrl::Enable");
	return true;
}

bool wxSpinCtrl::Show(bool show)
{
	FAIL("wxSpinCtrl::Show");
	return true;
}

const wxEventTable wxSpinCtrl::sm_eventTable = wxEventTable();
wxEventHashTable wxSpinCtrl::sm_eventHashTable (wxSpinCtrl::sm_eventTable);

const wxEventTable* wxSpinCtrl::GetEventTable() const
{
	FAIL("wxSpinCtrl::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxSpinCtrl::GetEventHashTable() const
{
	FAIL("wxSpinCtrl::GetEventHashTable");
	return sm_eventHashTable;
}

void wxSpinCtrl::OnChildFocus(wxChildFocusEvent& event)
{
	FAIL("wxSpinCtrl::OnChildFocus");
}

void wxSpinCtrl::SetFocusIgnoringChildren()
{
	FAIL("wxSpinCtrl::SetFocusIgnoringChildren");
}

void wxSpinCtrl::SetFocus()
{
	FAIL("wxSpinCtrl::SetFocus");
}

void wxSpinCtrl::RemoveChild(wxWindowBase *child)
{
	FAIL("wxSpinCtrl::RemoveChild");
}

bool wxSpinCtrl::AcceptsFocus() const
{
	FAIL("wxSpinCtrl::AcceptsFocus");
	return true;
}

wxClassInfo *wxSpinCtrl::GetClassInfo() const
{
	FAIL("wxSpinCtrl::GetClassInfo");
	return NULL;
}

int wxSpinCtrl::GetValue() const
{
	FAIL("wxSpinCtrl::GetValue");
	return 0;
}

void wxSpinCtrl::SetValue(int val)
{
	FAIL("wxSpinCtrl::SetValue");
}



