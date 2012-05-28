
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/notebook.h"

IMPLEMENT_DYNAMIC_CLASS(wxNotebook, wxNotebookBase);

const wxChar wxNotebookNameStr[] = L"";

wxNotebook::wxNotebook()
{
	FAIL("wxNotebook::wxNotebook");
}

wxSize wxNotebookBase::CalcSizeFromPage(const wxSize& sizePage) const
{
	FAIL("wxNotebookBase::CalcSizeFromPage");
	return wxSize();
}

const wxEventTable wxNotebook::sm_eventTable = wxEventTable();
wxEventHashTable wxNotebook::sm_eventHashTable (wxNotebook::sm_eventTable);

const wxEventTable* wxNotebook::GetEventTable() const
{
	FAIL("wxNotebook::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxNotebook::GetEventHashTable() const
{
	FAIL("wxNotebook::GetEventHashTable");
	return sm_eventHashTable;
}

wxNotebook::~wxNotebook()
{
	FAIL("wxNotebook::~wxNotebook");
}

wxNotebook::wxNotebook(wxWindow *parent, wxWindowID id,  const wxPoint& pos, const wxSize& size,
		long style, const wxString& name)
{
	FAIL("wxNotebook::wxNotebook");
}

wxSize wxNotebook::CalcSizeFromPage(const wxSize& sizePage) const
{
	FAIL("wxNotebook::CalcSizeFromPage");
	return wxSize();
}

void wxNotebook::Command(wxCommandEvent& event)
{
	FAIL("wxNotebook::Command");
}

bool wxNotebook::DeleteAllPages()
{
	FAIL("wxNotebook::DeleteAllPages");
	return true;
}

bool wxNotebook::DoPhase(int nPhase)
{
	FAIL("wxNotebook::DoPhase");
	return true;
}

wxNotebookPage *wxNotebook::DoRemovePage(size_t page)
{
	FAIL("wxNotebook::DoRemovePage");
	return NULL;
}

int wxNotebook::DoSetSelection(size_t nPage, int flags)
{
	FAIL("wxNotebook::DoSetSelection");
	return 0;
}

int wxNotebook:: GetPageImage(size_t nPage) const
{
	FAIL("wxNotebook::GetPageImage");
	return 0;
}

wxString wxNotebook::GetPageText(size_t nPage) const
{
	FAIL("wxNotebook::GetPageText");
	return L"";
}

int wxNotebook::HitTest(const wxPoint& pt, long *flags) const
{
	FAIL("wxNotebook::HitTest");
	return 0;
}

bool wxNotebook::InsertPage(size_t nPage, wxNotebookPage *pPage, const wxString& strText,
		bool bSelect, int imageId)
{
	FAIL("wxNotebook::InsertPage");
	return true;
}

wxInt32 wxNotebook::MacControlHit(WXEVENTHANDLERREF handler, WXEVENTREF event)
{
	FAIL("wxNotebook::MacControlHit");
	return 0;
}

void wxNotebook::SetConstraintSizes(bool recurse)
{
	FAIL("wxNotebook::SetConstraintSizes");
}

void wxNotebook::SetPadding(const wxSize& padding)
{
	FAIL("wxNotebook::SetPadding");
}

bool wxNotebook::SetPageImage(size_t nPage, int nImage)
{
	FAIL("wxNotebook::SetPageImage");
	return true;
}

void wxNotebook::SetPageSize(const wxSize& size)
{
	FAIL("wxNotebook::SetPageSize");
}

bool wxNotebook::SetPageText(size_t nPage, const wxString& strText)
{
	FAIL("wxNotebook::SetPageText");
	return true;
}

void wxNotebook::SetTabSize(const wxSize& sz)
{
	FAIL("wxNotebook::SetPageText");
}

