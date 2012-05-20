
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/aui/auibook.h"

DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_UP);
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE);
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_UP);
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_BG_DCLICK);
DEFINE_EVENT_TYPE(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED);

const wxEventTable wxAuiNotebook::sm_eventTable = wxEventTable();
wxEventHashTable wxAuiNotebook::sm_eventHashTable (wxAuiNotebook::sm_eventTable);

const wxEventTable* wxAuiNotebook::GetEventTable() const
{
	FAIL("wxAuiNotebook::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxAuiNotebook::GetEventHashTable() const
{
	FAIL("wxAuiNotebook::GetEventHashTable");
	return sm_eventHashTable;
}

wxClassInfo *wxAuiNotebook::GetClassInfo() const
{
	FAIL("wxAuiNotebook::GetClassInfo");
	return NULL;
}

bool wxAuiNotebook::SetFont(const wxFont& font)
{
	FAIL("wxAuiNotebook::SetFont");
	return true;
}


size_t wxAuiNotebook::SetSelection(size_t new_page)
{
	FAIL("wxAuiNotebook::SetSelection");
	return 0;
}

bool wxAuiNotebook::RemovePage(size_t page)
{
	FAIL("wxAuiNotebook::RemovePage");
	return true;
}

bool wxAuiNotebook::DeletePage(size_t page)
{
	FAIL("wxAuiNotebook::DeletePage");
	return true;
}

int wxAuiNotebook::CalculateTabCtrlHeight()
{
	FAIL("wxAuiNotebook::CalculateTabCtrlHeight");
	return 0;
}

wxSize wxAuiNotebook::CalculateNewSplitSize()
{
	FAIL("wxAuiNotebook::CalculateNewSplitSize");
	return wxSize();
}

bool wxAuiNotebook::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
	FAIL("wxAuiNotebook::Create");
	return true;
}

void wxAuiNotebook::SetArtProvider(wxAuiTabArt* art)
{
	FAIL("wxAuiNotebook::SetArtProvider");
}

bool wxAuiNotebook::AddPage(wxWindow* page, const wxString& caption, bool select, const wxBitmap& bitmap)
{
	FAIL("wxAuiNotebook::AddPage");
	return true;
}

void wxAuiNotebook::SetSelectedFont(const wxFont& font)
{
	FAIL("wxAuiNotebook::SetSelectedFont");
}

size_t wxAuiNotebook::GetPageCount() const
{
	FAIL("wxAuiNotebook::GetPageCount");
	return 0;
}

int wxAuiNotebook:: GetSelection() const
{
	FAIL("wxAuiNotebook::GetSelection");
	return 0;
}

void wxAuiNotebook::SetUniformBitmapSize(const wxSize& size)
{
	FAIL("wxAuiNotebook::SetUniformBitmapSize");
}

void wxAuiNotebook::SetMeasuringFont(const wxFont& font)
{
	FAIL("wxAuiNotebook::SetMeasuringFont");
}

wxAuiTabCtrl* wxAuiNotebook::GetActiveTabCtrl()
{
	FAIL("wxAuiNotebook::GetActiveTabCtrl");
	return NULL;
}

void wxAuiNotebook::SetWindowStyleFlag(long style)
{
	FAIL("wxAuiNotebook::SetWindowStyleFlag");
}

void wxAuiNotebook::Split(size_t page, int direction)
{
	FAIL("wxAuiNotebook::Split");
}

wxAuiNotebook::wxAuiNotebook()
{
	FAIL("wxAuiNotebook::wxAuiNotebook");
}

wxAuiNotebook::~wxAuiNotebook()
{
	FAIL("wxAuiNotebook::~wxAuiNotebook");
}

static wxAuiNotebookPage gPage;

wxAuiNotebookPage& GetPage(size_t idx)
{
	FAIL("wxAuiNotebook::GetPage");
	return gPage;
}

void wxAuiNotebook::UpdateTabCtrlHeight()
{
	FAIL("wxAuiNotebook::UpdateTabCtrlHeight");
}

void wxAuiNotebook::SetTabCtrlHeight(int height)
{
	FAIL("wxAuiNotebook::SetTabCtrlHeight");
}

int wxAuiNotebook::GetPageIndex(wxWindow* page_wnd) const
{
	FAIL("wxAuiNotebook::GetPageIndex");
	return 0;
}

bool wxAuiNotebook::FindTab(wxWindow* page, wxAuiTabCtrl** ctrl, int* idx)
{
	FAIL("wxAuiNotebook::FindTab");
	return true;
}

wxAuiTabContainer::wxAuiTabContainer()
{
	FAIL("wxAuiTabContainer::wxAuiTabContainer");
}

wxAuiTabContainer::~wxAuiTabContainer()
{
	FAIL("wxAuiTabContainer::~wxAuiTabContainer");
}

void wxAuiTabContainer::Render(wxDC* dc, wxWindow* wnd)
{
	FAIL("wxAuiTabContainer::Render");
}

void wxAuiDefaultTabArt::DrawButton(wxDC& dc, wxWindow* wnd, const wxRect& in_rect, int bitmap_id,
		int button_state, int orientation, wxRect* out_rect)
{
	FAIL("wxAuiDefaultTabArt::DrawButton");
}

wxAuiNotebookPageArray::~wxAuiNotebookPageArray()
{
	FAIL("wxAuiNotebookPageArray::~wxAuiNotebookPageArray");
}

wxAuiTabContainerButtonArray::~wxAuiTabContainerButtonArray()
{
	FAIL("wxAuiTabContainerButtonArray::~wxAuiTabContainerButtonArray");
}

