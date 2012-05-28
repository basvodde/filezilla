

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/bookctrl.h"

IMPLEMENT_ABSTRACT_CLASS(wxBookCtrlBase, wxControl);

const wxEventTable wxBookCtrlBase::sm_eventTable = wxEventTable();
wxEventHashTable wxBookCtrlBase::sm_eventHashTable (wxBookCtrlBase::sm_eventTable);

const wxEventTable* wxBookCtrlBase::GetEventTable() const
{
	FAIL("wxBookCtrlBase::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxBookCtrlBase::GetEventHashTable() const
{
	FAIL("wxBookCtrlBase::GetEventHashTable");
	return sm_eventHashTable;
}


wxBookCtrlBase::~wxBookCtrlBase()
{
	FAIL("wxBookCtrlBase::~wxBookCtrlBase");
}

bool wxBookCtrlBase::DeletePage(size_t n)
{
	FAIL("wxBookCtrlBase::DeletePage");
	return true;
}

wxSize wxBookCtrlBase::DoGetBestSize() const
{
	FAIL("wxBookCtrlBase::DoGetBestSize");
	return wxSize();
}

void wxBookCtrlBase::DoInvalidateBestSize()
{
	FAIL("wxBookCtrlBase::DoInvalidateBestSize");
}

int wxBookCtrlBase::DoSetSelection(size_t nPage, int flags)
{
	FAIL("wxBookCtrlBase::DoSetSelection");
	return 0;
}

wxSize wxBookCtrlBase::GetControllerSize() const
{
	FAIL("wxBookCtrlBase::GetControllerSize");
	return wxSize();
}

void wxBookCtrlBase::Init()
{
	FAIL("wxBookCtrlBase::Init");
}

void wxBookCtrlBase::SetImageList(wxImageList *imageList)
{
	FAIL("wxBookCtrlBase::SetImageList");
}

void wxBookCtrlBase::SetPageSize(const wxSize& size)
{
	FAIL("wxBookCtrlBase::SetPageSize");
}


