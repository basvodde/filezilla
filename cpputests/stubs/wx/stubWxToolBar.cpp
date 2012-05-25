
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/toolbar.h"

void wxwxToolBarToolsListNode::DeleteData()
{
	FAIL("wxwxToolBarToolsListNode::DeleteData");
}

const wxEventTable wxToolBarBase::sm_eventTable = wxEventTable();
wxEventHashTable wxToolBarBase::sm_eventHashTable (wxToolBarBase::sm_eventTable);

const wxEventTable* wxToolBarBase::GetEventTable() const
{
	FAIL("wxToolBarBase::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxToolBarBase::GetEventHashTable() const
{
	FAIL("wxToolBarBase::GetEventHashTable");
	return sm_eventHashTable;
}

wxToolBarBase::wxToolBarBase()
{
	FAIL("wxToolBarBase::wxToolBarBase");
}

void wxToolBarBase::EnableTool(int toolid, bool enable)
{
	FAIL("wxToolBarBase::EnableTool");
}

void wxToolBarBase::ToggleTool(int toolid, bool toggle)
{
	FAIL("wxToolBarBase::ToggleTool");
}

int wxToolBarBase::GetToolPos(int id) const
{
	FAIL("wxToolBarBase::GetToolPos");
	return 1;
}

bool wxToolBarBase::GetToolState(int toolid) const
{
	FAIL("wxToolBarBase::GetToolState");
	return true;
}

bool wxToolBarBase::GetToolEnabled(int toolid) const
{
	FAIL("wxToolBarBase::GetToolEnabled");
	return true;
}

void wxToolBarBase::SetToolShortHelp(int toolid, const wxString& helpString)
{
	FAIL("wxToolBarBase::SetToolShortHelp");
}

wxString wxToolBarBase::GetToolShortHelp(int toolid) const
{
	FAIL("wxToolBarBase::GetToolShortHelp");
	return L"";
}

void wxToolBarBase::SetToolLongHelp(int toolid, const wxString& helpString)
{
	FAIL("wxToolBarBase::SetToolLongHelp");
}

wxString wxToolBarBase::GetToolLongHelp(int toolid) const
{
	FAIL("wxToolBarBase::GetToolLongHelp");
	return L"";
}

void wxToolBarBase::SetMargins(int x, int y)
{
	FAIL("wxToolBarBase::SetMargins");
}

wxToolBarToolBase *wxToolBarBase::DoAddTool(int toolid, const wxString& label, const wxBitmap& bitmap,
		const wxBitmap& bmpDisabled, wxItemKind kind, const wxString& shortHelp,
		const wxString& longHelp, wxObject *clientData, wxCoord xPos,
		wxCoord yPos)
{
	FAIL("wxToolBarBase::DoAddTool");
	return NULL;
}

wxToolBarToolBase *wxToolBarBase::InsertTool(size_t pos, int toolid, const wxString& label,
		const wxBitmap& bitmap, const wxBitmap& bmpDisabled, wxItemKind kind, const wxString& shortHelp,
		const wxString& longHelp, wxObject *clientData)
{
	FAIL("wxToolBarBase::InsertTool");
	return NULL;
}
wxToolBarToolBase *wxToolBarBase::AddTool (wxToolBarToolBase *tool)
{
	FAIL("wxToolBarBase::AddTool");
	return NULL;
}

wxToolBarToolBase *wxToolBarBase::InsertTool (size_t pos, wxToolBarToolBase *tool)
{
	FAIL("wxToolBarBase::InsertTool");
	return NULL;
}

wxToolBarToolBase *wxToolBarBase::AddControl(wxControl *control)
{
	FAIL("wxToolBarBase::AddControl");
	return NULL;
}

wxToolBarToolBase *wxToolBarBase::InsertControl(size_t pos, wxControl *control)
{
	FAIL("wxToolBarBase::InsertControl");
	return NULL;
}

wxControl *wxToolBarBase::FindControl( int toolid )
{
	FAIL("wxToolBarBase::FindControl");
	return NULL;
}

wxToolBarToolBase *wxToolBarBase::AddSeparator()
{
	FAIL("wxToolBarBase::AddSeparator");
	return NULL;
}

wxToolBarToolBase *wxToolBarBase::InsertSeparator(size_t pos)
{
	FAIL("wxToolBarBase::InsertSeparator");
	return NULL;
}

wxToolBarToolBase *wxToolBarBase::RemoveTool(int toolid)
{
	FAIL("wxToolBarBase::RemoveTool");
	return NULL;
}

bool wxToolBarBase::DeleteToolByPos(size_t pos)
{
	FAIL("wxToolBarBase::DeleteToolByPos");
	return true;
}

bool wxToolBarBase::DeleteTool(int toolid)
{
	FAIL("wxToolBarBase::DeleteTool");
	return true;
}

void wxToolBarBase::ClearTools()
{
	FAIL("wxToolBarBase::ClearTools");
}

bool wxToolBarBase::Realize()
{
	FAIL("wxToolBarBase::Realize");
	return true;
}

void wxToolBarBase::SetToggle(int toolid, bool toggle)
{
	FAIL("wxToolBarBase::SetToggle");
}

wxObject *wxToolBarBase::GetToolClientData(int toolid) const
{
	FAIL("wxToolBarBase::GetToolClientData");
	return NULL;
}

void wxToolBarBase::SetToolClientData(int toolid, wxObject *clientData)
{
	FAIL("wxToolBarBase::SetToolClientData");
}

wxToolBarBase::~wxToolBarBase()
{
	FAIL("wxToolBarBase::~wxToolBarBase");
}

bool wxToolBarBase::OnLeftClick(int toolid, bool toggleDown)
{
	FAIL("wxToolBarBase::OnLeftClick");
	return true;
}

void wxToolBarBase::OnRightClick(int toolid, long x, long y)
{
	FAIL("wxToolBarBase::OnRightClick");
}

void wxToolBarBase::OnMouseEnter(int toolid)
{
	FAIL("wxToolBarBase::OnMouseEnter");
}

void wxToolBarBase::SetRows(int nRows)
{
	FAIL("wxToolBarBase::SetRows");
}

void wxToolBarBase::UpdateWindowUI(long flags)
{
	FAIL("wxToolBarBase::UpdateWindowUI");
}

const wxEventTable wxToolBar::sm_eventTable = wxEventTable();
wxEventHashTable wxToolBar::sm_eventHashTable (wxToolBar::sm_eventTable);

const wxEventTable* wxToolBar::GetEventTable() const
{
	FAIL("wxToolBar::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxToolBar::GetEventHashTable() const
{
	FAIL("wxToolBar::GetEventHashTable");
	return sm_eventHashTable;
}

wxClassInfo *wxToolBar::GetClassInfo() const
{
	FAIL("wxToolBar::GetClassInfo");
	return NULL;
}

void wxToolBar::DoGetSize(int *width, int *height) const
{
	FAIL("wxToolBar::DoGetSize");
}

wxSize wxToolBar::DoGetBestSize() const
{
	FAIL("wxToolBar::DoGetBestSize");
	return wxSize();
}

bool wxToolBar::DoInsertTool(size_t pos, wxToolBarToolBase *tool)
{
	FAIL("wxToolBar::DoInsertTool");
	return true;
}

bool wxToolBar::DoDeleteTool(size_t pos, wxToolBarToolBase *tool)
{
	FAIL("wxToolBar::DoDeleteTool");
	return true;
}

void wxToolBar::DoEnableTool(wxToolBarToolBase *tool, bool enable)
{
	FAIL("wxToolBar::DoEnableTool");
}

void wxToolBar::DoToggleTool(wxToolBarToolBase *tool, bool toggle)
{
	FAIL("wxToolBar::DoToggleTool");
}

void wxToolBar::DoSetToggle(wxToolBarToolBase *tool, bool toggle)
{
	FAIL("wxToolBar::DoSetToggle");
}

wxToolBarToolBase *wxToolBar::CreateTool(int id, const wxString& label, const wxBitmap& bmpNormal,
		const wxBitmap& bmpDisabled, wxItemKind kind, wxObject *clientData,
		const wxString& shortHelp, const wxString& longHelp)
{
	FAIL("wxToolBar::CreateTool");
	return NULL;
}

wxToolBarToolBase *wxToolBar::CreateTool(wxControl *control)
{
	FAIL("wxToolBar::CreateTool");
	return NULL;
}

void wxToolBar::SetWindowStyleFlag(long style)
{
	FAIL("wxToolBar::SetWindowStyleFlag");
}

wxToolBarToolBase *wxToolBar::FindToolForPosition(wxCoord x, wxCoord y) const
{
	FAIL("wxToolBar::FindToolForPosition");
	return NULL;
}
bool wxToolBar::Show(bool show)
{
	FAIL("wxToolBar::Show");
	return true;
}

bool wxToolBar::IsShown() const
{
	FAIL("wxToolBar::IsShown");
	return true;
}

bool wxToolBar::Realize()
{
	FAIL("wxToolBar::Realize");
	return true;
}

void wxToolBar::SetToolBitmapSize(const wxSize& size)
{
	FAIL("wxToolBar::SetToolBitmapSize");
}

wxSize wxToolBar::GetToolSize() const
{
	FAIL("wxToolBar::GetToolSize");
	return wxSize();
}

void wxToolBar::SetRows(int nRows)
{
	FAIL("wxToolBar::SetRows");
}

wxToolBar::~wxToolBar()
{
	FAIL("wxToolBar::~wxToolBar");
}

wxString wxToolBar::MacGetToolTipString( wxPoint &where )
{
	FAIL("wxToolBar::MacGetToolTipString");
	return L"";
}

void wxToolBar::MacSuperChangedPosition()
{
	FAIL("wxToolBar::MacSuperChangedPosition");
}

bool wxToolBar::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
{
	FAIL("wxToolBar::Create");
	return true;
}

void wxToolBar::Init()
{
	FAIL("wxToolBar::Init");
}

wxClassInfo wxToolBar::ms_classInfo(NULL, NULL, NULL, 0, NULL);

