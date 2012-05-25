
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/statbmp.h"

const wxChar wxStaticBitmapNameStr[] = L"";

wxStaticBitmapBase::~wxStaticBitmapBase()
{
	FAIL("wxStaticBitmapBase::~wxStaticBitmapBase");
}

wxSize wxStaticBitmapBase::DoGetBestSize() const
{
	FAIL("wxStaticBitmapBase::DoGetBestSize");
	return wxSize();
}

wxClassInfo *wxStaticBitmap::GetClassInfo() const
{
	FAIL("wxStaticBitmap::GetClassInfo");
	return NULL;
}

const wxEventTable wxStaticBitmap::sm_eventTable = wxEventTable();
wxEventHashTable wxStaticBitmap::sm_eventHashTable (wxStaticBitmap::sm_eventTable);

const wxEventTable* wxStaticBitmap::GetEventTable() const
{
	FAIL("wxStaticBitmap::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxStaticBitmap::GetEventHashTable() const
{
	FAIL("wxStaticBitmap::GetEventHashTable");
	return sm_eventHashTable;
}

wxSize wxStaticBitmap::DoGetBestSize() const
{
	FAIL("wxStaticBitmap::DoGetBestSize");
	return wxSize();
}

void wxStaticBitmap::SetBitmap(const wxBitmap& bitmap)
{
	FAIL("wxStaticBitmap::SetBitmap");
}

bool wxStaticBitmap::Create(wxWindow *parent, wxWindowID id,  const wxBitmap& label, const wxPoint& pos,
		const wxSize& size, long style, const wxString& name)
{
	FAIL("wxStaticBitmap::Create");
	return true;
}

