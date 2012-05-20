
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/statbmp.h"

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

