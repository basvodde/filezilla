

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/listctrl.h"

void wxwxColumnListNode::DeleteData()
{
	FAIL("wxwxColumnListNode::DeleteData");
}

const wxEventTable wxListCtrl::sm_eventTable = wxEventTable();
wxEventHashTable wxListCtrl::sm_eventHashTable (wxListCtrl::sm_eventTable);

const wxEventTable* wxListCtrl::GetEventTable() const
{
	FAIL("wxListCtrl::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxListCtrl::GetEventHashTable() const
{
	FAIL("wxListCtrl::GetEventHashTable");
	return sm_eventHashTable;
}

wxClassInfo *wxListCtrl::GetClassInfo() const
{
	FAIL("wxListCtrl::GetClassInfo");
	return NULL;
}

void wxListCtrl::SetDropTarget( wxDropTarget *dropTarget )
{
	FAIL("wxListCtrl::SetDropTarget");
}

wxDropTarget* wxListCtrl::GetDropTarget() const
{
	FAIL("wxListCtrl::GetDropTarget");
	return NULL;
}

wxString wxListCtrl::OnGetItemText(long item, long column) const
{
	FAIL("wxListCtrl::OnGetItemText");
	return L"";
}

int wxListCtrl::OnGetItemImage(long item) const
{
	FAIL("wxListCtrl::OnGetItemImage");
	return 0;
}

int wxListCtrl::OnGetItemColumnImage(long item, long column) const
{
	FAIL("wxListCtrl::OnGetItemColumnImage");
	return 0;
}

wxListItemAttr *wxListCtrl::OnGetItemAttr(long item) const
{
	FAIL("wxListCtrl::OnGetItemAttr");
	return NULL;
}

void wxListCtrl::SetFocus()
{
	FAIL("wxListCtrl::SetFocus");
}

bool wxListCtrl::SetFont(const wxFont& font)
{
	FAIL("wxListCtrl::SetFont");
	return true;
}

bool wxListCtrl::SetForegroundColour(const wxColour& colour)
{
	FAIL("wxListCtrl::SetForegroundColour");
	return true;
}

bool wxListCtrl::SetBackgroundColour(const wxColour& colour)
{
	FAIL("wxListCtrl::SetBackgroundColour");
	return true;
}

wxColour wxListCtrl::GetBackgroundColour() const
{
	FAIL("wxListCtrl::GetBackgroundColour");
	return wxColour();

}

void wxListCtrl::Freeze ()
{
	FAIL("wxListCtrl::Freeze");
}

void wxListCtrl::Thaw ()
{
	FAIL("wxListCtrl::Thaw");
}

void wxListCtrl::Update ()
{
	FAIL("wxListCtrl::Update");
}

int wxListCtrl::GetScrollPos(int orient) const
{
	FAIL("wxListCtrl::GetScrollPos");
	return 0;
}

wxVisualAttributes wxListCtrl::GetClassDefaultAttributes(wxWindowVariant variant)
{
	FAIL("wxListCtrl::GetClassDefaultAttributes");
	return wxVisualAttributes();
}

void wxListCtrl::DoSetSize(int x, int y, int width, 	int height, int sizeFlags)
{
	FAIL("wxListCtrl::DoSetSize");
}

wxSize wxListCtrl::DoGetBestSize() const
{
	FAIL("wxListCtrl::DoGetBestSize");
	return wxSize();
}

void wxListCtrl::SetWindowStyleFlag(long style)
{
	FAIL("wxListCtrl::SetWindowStyleFlag");
}

wxListCtrl::~wxListCtrl()
{
	FAIL("wxListCtrl::~wxListCtrl");
}

