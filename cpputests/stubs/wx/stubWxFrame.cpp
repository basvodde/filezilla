
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/frame.h"

const wxEventTable wxFrameBase::sm_eventTable = wxEventTable();
wxEventHashTable wxFrameBase::sm_eventHashTable (wxFrameBase::sm_eventTable);

const wxEventTable* wxFrameBase::GetEventTable() const
{
	FAIL("wxFrameBase::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxFrameBase::GetEventHashTable() const
{
	FAIL("wxFrameBase::GetEventHashTable");
	return sm_eventHashTable;
}


wxFrameBase::~wxFrameBase()
{
	FAIL("wxFrameBase::wxFrameBase");
}

wxPoint wxFrameBase::GetClientAreaOrigin() const
{
	FAIL("wxFrameBase::GetClientAreaOrigin");
	return wxPoint();
}

void wxFrameBase::SendSizeEvent()
{
	FAIL("wxFrameBase::SendSizeEvent");
}

wxToolBar* wxFrameBase::CreateToolBar(long style, wxWindowID winid, const wxString& name)
{
	FAIL("wxFrameBase::CreateToolBar");
	return NULL;
}

wxToolBar *wxFrameBase::OnCreateToolBar(long style, wxWindowID winid, const wxString& name )
{
	FAIL("wxFrameBase::OnCreateToolBar");
	return NULL;
}

void wxFrameBase::SetToolBar(wxToolBar *toolbar)
{
	FAIL("wxFrameBase::SetToolBar");
}

wxStatusBar* wxFrameBase::CreateStatusBar(int number, long style, wxWindowID winid, const wxString& name)
{
	FAIL("wxFrameBase::CreateStatusBar");
	return NULL;
}

wxStatusBar *wxFrameBase::OnCreateStatusBar(int number, long style, wxWindowID winid, const wxString& name)
{
	FAIL("wxFrameBase::OnCreateStatusBar");
	return NULL;
}

void wxFrameBase::SetStatusBar(wxStatusBar *statBar)
{
	FAIL("wxFrameBase::SetStatusBar");
}

void wxFrameBase::SetStatusText(const wxString &text, int number)
{
	FAIL("wxFrameBase::SetStatusText");
}
void wxFrameBase::SetStatusWidths(int n, const int widths_field[])
{
	FAIL("wxFrameBase::SetStatusWidths");
}

void wxFrameBase::DetachMenuBar()
{
	FAIL("wxFrameBase::DetachMenuBar");
}

void wxFrameBase::AttachMenuBar(wxMenuBar *menubar)
{
	FAIL("wxFrameBase::AttachMenuBar");
}

void wxFrameBase::DoGiveHelp(const wxString& text, bool show)
{
	FAIL("wxFrameBase::DoGiveHelp");
}

void wxFrameBase::DoMenuUpdates(wxMenu* menu)
{
	FAIL("wxFrameBase::DoMenuUpdates");
}

void wxFrameBase::SetMenuBar(wxMenuBar *menubar)
{
	FAIL("wxFrameBase::SetMenuBar");
}

bool wxFrameBase::IsOneOfBars(const wxWindow *win) const
{
	FAIL("wxFrameBase::IsOneOfBars");
	return true;
}

void wxFrameBase::OnInternalIdle()
{
	FAIL("wxFrameBase::OnInternalIdle");
}

void wxFrameBase::UpdateWindowUI(long flags)
{
	FAIL("wxFrameBase::UpdateWindowUI");
}

wxFrameBase::wxFrameBase()
{
	FAIL("wxFrameBase::wxFrameBase");
}


const wxEventTable wxFrame::sm_eventTable = wxEventTable();
wxEventHashTable wxFrame::sm_eventHashTable (wxFrame::sm_eventTable);

const wxEventTable* wxFrame::GetEventTable() const
{
	FAIL("wxFrame::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxFrame::GetEventHashTable() const
{
	FAIL("wxFrame::GetEventHashTable");
	return sm_eventHashTable;
}

void wxFrame::DoGetClientSize(int *width, int *height) const
{
	FAIL("wxFrame::DoGetClientSize");
}

wxFrame::~wxFrame()
{
	FAIL("wxFrame::~wxFrame");
}

wxPoint wxFrame::GetClientAreaOrigin() const
{
	FAIL("wxFrame::GetClientAreaOrigin");
	return wxPoint();
}

bool wxFrame::Enable(bool enable)
{
	FAIL("wxFrame::Enable");
	return true;
}

wxToolBar* wxFrame::CreateToolBar(long style,  wxWindowID id,  const wxString& name)
{
	FAIL("wxFrame::CreateToolBar");
	return NULL;
}

void wxFrame::SetToolBar(wxToolBar *toolbar)
{
	FAIL("wxFrame::SetToolBar");
}

wxStatusBar* wxFrame::OnCreateStatusBar(int number, long style, wxWindowID id, const wxString& name)
{
	FAIL("wxFrame::OnCreateStatusBar");
	return NULL;
}

void wxFrame::DoSetClientSize(int width, int height)
{
	FAIL("wxFrame::DoSetClientSize");
}

void wxFrame::DetachMenuBar()
{
	FAIL("wxFrame::DetachMenuBar");
}

void wxFrame::AttachMenuBar(wxMenuBar *menubar)
{
	FAIL("wxFrame::AttachMenuBar");
}

bool wxFrame::MacIsChildOfClientArea( const wxWindow* child ) const
{
	FAIL("wxFrame::MacIsChildOfClientArea");
	return true;
}

void wxFrame::PositionStatusBar()
{
	FAIL("wxFrame::PositionStatusBar");
}

void wxFrame::PositionToolBar()
{
	FAIL("wxFrame::PositionToolBar");
}

void wxFrame::Init()
{
	FAIL("wxFrame::Init");
}

bool wxFrame::Create(wxWindow *parent, wxWindowID id, const wxString& title, const wxPoint& pos,
		const wxSize& size, long style, const wxString& name)
{
	FAIL("wxFrame::Create");
	return true;
}

IMPLEMENT_DYNAMIC_CLASS(wxFrame, wxFrameBase);

