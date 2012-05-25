
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/panel.h"

wxPanel::~wxPanel()
{
	FAIL("wxPanel::~wxPanel");
}

void wxPanel::OnChildFocus(wxChildFocusEvent& event)
{
	FAIL("wxPanel::OnChildFocus");
}

void wxPanel::SetFocusIgnoringChildren()
{
	FAIL("wxPanel::SetFocusIgnoringChildren");
}

void wxPanel::SetFocus()
{
	FAIL("wxPanel::SetFocus");
}

void wxPanel::RemoveChild(wxWindowBase *child)
{
	FAIL("wxPanel::RemoveChild");
}

bool wxPanel::AcceptsFocus() const
{
	FAIL("wxPanel::AcceptsFocus");
	return true;
}

void wxPanel::InitDialog()
{
	FAIL("wxPanel::InitDialog");
}

const wxEventTable wxPanel::sm_eventTable = wxEventTable();
wxEventHashTable wxPanel::sm_eventHashTable (wxPanel::sm_eventTable);

const wxEventTable* wxPanel::GetEventTable() const
{
	FAIL("wxPanel::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxPanel::GetEventHashTable() const
{
	FAIL("wxPanel::GetEventHashTable");
	return sm_eventHashTable;
}

wxClassInfo *wxPanel::GetClassInfo() const
{
	FAIL("wxPanel::GetClassInfo");
	return NULL;
}

bool wxPanel::Create(wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size,
		long style, const wxString& name)
{
	FAIL("wxPanel::Create");
	return true;
}

void wxPanel::Init()
{
	FAIL("wxPanel::Init");
}
