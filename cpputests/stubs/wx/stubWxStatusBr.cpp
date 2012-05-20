
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/statusbr.h"

wxStatusBarBase::~wxStatusBarBase()
{
	FAIL("wxStatusBarBase::~wxStatusBarBase");
}

void wxStatusBarBase::SetStatusWidths(int n, const int widths[])
{
	FAIL("wxStatusBarBase::SetStatusWidths");
}

void wxStatusBarBase::SetStatusStyles(int n, const int styles[])
{
	FAIL("wxStatusBarBase::SetStatusStyles");
}

void wxStatusBarBase::SetFieldsCount(int number, const int *widths)
{
	FAIL("wxStatusBarBase::SetFieldsCount");
}

wxStatusBarBase::wxStatusBarBase()
{
	FAIL("wxStatusBarBase::wxStatusBarBase");
}

const wxEventTable wxStatusBar::sm_eventTable = wxEventTable();
wxEventHashTable wxStatusBar::sm_eventHashTable (wxStatusBar::sm_eventTable);

const wxEventTable* wxStatusBar::GetEventTable() const
{
	FAIL("wxStatusBar::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxStatusBar::GetEventHashTable() const
{
	FAIL("wxStatusBar::GetEventHashTable");
	return sm_eventHashTable;
}

wxClassInfo *wxStatusBar::GetClassInfo() const
{
	FAIL("wxStatusBar::GetClassInfo");
	return NULL;
}

void wxStatusBar::MacHiliteChanged()
{
	FAIL("wxStatusBar::MacHiliteChanged");
}

void wxStatusBar::DrawFieldText(wxDC& dc, int i)
{
	FAIL("wxStatusBar::DrawFieldText");
}

void wxStatusBar::DrawField(wxDC& dc, int i)
{
	FAIL("wxStatusBar::DrawField");
}

void wxStatusBar::SetStatusText(const wxString& text, int number)
{
	FAIL("wxStatusBar::SetStatusText");
}

wxStatusBar::wxStatusBar(wxWindow *parent, wxWindowID id,long style, const wxString& name)
{
	FAIL("wxStatusBar::wxStatusBar");
}

bool wxStatusBar::Create(wxWindow *parent, wxWindowID id,long style, const wxString& name)
{
	FAIL("wxStatusBar::Create");
	return true;
}

wxStatusBar::wxStatusBar()
{
	FAIL("wxStatusBar::wxStatusBar");
}

wxStatusBar::~wxStatusBar()
{
	FAIL("wxStatusBar::~wxStatusBar");
}

const wxEventTable wxStatusBarGeneric::sm_eventTable = wxEventTable();
wxEventHashTable wxStatusBarGeneric::sm_eventHashTable (wxStatusBarGeneric::sm_eventTable);

const wxEventTable* wxStatusBarGeneric::GetEventTable() const
{
	FAIL("wxStatusBarGeneric::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxStatusBarGeneric::GetEventHashTable() const
{
	FAIL("wxStatusBarGeneric::GetEventHashTable");
	return sm_eventHashTable;
}

wxClassInfo *wxStatusBarGeneric::GetClassInfo() const
{
	FAIL("wxStatusBarGeneric::GetClassInfo");
	return NULL;
}

void wxStatusBarGeneric::InitColours()
{
	FAIL("wxStatusBarGeneric::InitColours");
}

bool wxStatusBarGeneric::GetFieldRect(int i, wxRect& rect) const
{
	FAIL("wxStatusBarGeneric::GetFieldRect");
	return true;
}

void wxStatusBarGeneric::SetMinHeight(int height)
{
	FAIL("wxStatusBarGeneric::SetMinHeight");
}

void wxStatusBarGeneric::Init()
{
	FAIL("wxStatusBarGeneric::Init");
}

wxStatusBarGeneric::~wxStatusBarGeneric()
{
	FAIL("wxStatusBarGeneric::~wxStatusBarGeneric");
}

wxSize wxStatusBarGeneric::DoGetBestSize() const
{
	FAIL("wxStatusBarGeneric::DoGetBestSize");
	return wxSize();
}

wxString wxStatusBarGeneric::GetStatusText(int number) const
{
	FAIL("wxStatusBarGeneric::GetStatusText");
	return L"";
}

void wxStatusBarGeneric::SetFieldsCount(int number, const int *widths)
{
	FAIL("wxStatusBarGeneric::SetFieldsCount");
}

void wxStatusBarGeneric::SetStatusWidths(int n, const int widths_field[])
{
	FAIL("wxStatusBarGeneric::SetStatusWidths");
}

void wxStatusBarGeneric::DrawFieldText(wxDC& dc, int i)
{
	FAIL("wxStatusBarGeneric::DrawFieldText");
}

void wxStatusBarGeneric::DrawField(wxDC& dc, int i)
{
	FAIL("wxStatusBarGeneric::DrawField");
}

void wxStatusBarGeneric::SetStatusText(const wxString& text, int number)
{
	FAIL("wxStatusBarGeneric::SetStatusText");
}


