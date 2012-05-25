
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/wizard.h"

DEFINE_EVENT_TYPE(wxEVT_WIZARD_CANCEL);
DEFINE_EVENT_TYPE(wxEVT_WIZARD_FINISHED);
DEFINE_EVENT_TYPE(wxEVT_WIZARD_PAGE_CHANGING);
DEFINE_EVENT_TYPE(wxEVT_WIZARD_PAGE_CHANGED);

void wxWizard::DoCreateControls()
{
	FAIL("wxWizard::DoCreateControls");
}

wxClassInfo *wxWizardPage::GetClassInfo() const
{
	FAIL("wxWizardPage::GetClassInfo");
	return NULL;
}

void wxWizardPage::Init()
{
	FAIL("wxWizardPage::Init");
}

wxWizardPage *wxWizardPageSimple::GetPrev() const
{
	FAIL("wxWizardPageSimple::GetPrev");
	return NULL;
}

wxWizardPage *wxWizardPageSimple::GetNext() const
{
	FAIL("wxWizardPageSimple::GetNext");
	return NULL;
}

wxClassInfo *wxWizardPageSimple::GetClassInfo() const
{
	FAIL("wxWizardPageSimple::GetClassInfo");
	return NULL;
}

bool wxWizard::RunWizard(wxWizardPage *firstPage)
{
	FAIL("wxWizard::RunWizard");
	return true;
}

wxWizardPage *wxWizard::GetCurrentPage() const
{
	FAIL("wxWizard::GetCurrentPage");
	return NULL;
}

void wxWizard::SetPageSize(const wxSize& size)
{
	FAIL("wxWizard::SetPageSize");
}

wxSize wxWizard::GetPageSize() const
{
	FAIL("wxWizard::GetPageSize");
	return wxSize();
}

void wxWizard::FitToPage(const wxWizardPage *firstPage)
{
	FAIL("wxWizard::FitToPage");
}

wxSizer *wxWizard::GetPageAreaSizer() const
{
	FAIL("wxWizard::GetPageAreaSizer");
	return NULL;
}

void wxWizard::SetBorder(int border)
{
	FAIL("wxWizard::SetBorder");
}

wxWizard::~wxWizard()
{
	FAIL("wxWizard::~wxWizard");
}

const wxEventTable wxWizard::sm_eventTable = wxEventTable();
wxEventHashTable wxWizard::sm_eventHashTable (wxWizard::sm_eventTable);

const wxEventTable* wxWizard::GetEventTable() const
{
	FAIL("wxWizard::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxWizard::GetEventHashTable() const
{
	FAIL("wxWizard::GetEventHashTable");
	return sm_eventHashTable;
}

wxClassInfo *wxWizard::GetClassInfo() const
{
	FAIL("wxWizard::GetClassInfo");
	return NULL;
}

void wxWizard::Init()
{
	FAIL("wxWizard::Init");
}

bool wxWizard::ShowPage(wxWizardPage *page, bool goingForward)
{
	FAIL("wxWizard::ShowPage");
	return true;
}

bool wxWizard::Create(wxWindow *parent, int id, const wxString& title, const wxBitmap& bitmap,
		const wxPoint& pos, long style)
{
	FAIL("wxWizard::Create");
	return true;
}


