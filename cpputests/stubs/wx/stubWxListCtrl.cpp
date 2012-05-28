

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/listctrl.h"

const wxChar wxListCtrlNameStr[] = L"";

IMPLEMENT_DYNAMIC_CLASS(wxListCtrl, wxControl);

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

void wxListCtrl::ClearAll()
{
	FAIL("wxListCtrl::ClearAll");
}

bool wxListCtrl::SetColumnWidth(int col, int width)
{
	FAIL("wxListCtrl::SetColumnWidth");
	return true;
}

void wxListCtrl::Init()
{
	FAIL("wxListCtrl::Init");
}

bool wxListCtrl::SetItemState(long item, long state, long stateMask)
{
	FAIL("wxListCtrl::SetItemState");
	return true;
}

long wxListCtrl::GetItemData(long item) const
{
	FAIL("wxListCtrl::GetItemData");
	return 0;
}

void wxListCtrl::SetItemCount(long count)
{
	FAIL("wxListCtrl::SetItemCount");
}

void wxListCtrl::RefreshItem(long item)
{
	FAIL("wxListCtrl::RefreshItem");
}

long wxListCtrl::HitTest(const wxPoint& point, int& flags, long* ptrSubItem) const
{
	FAIL("wxListCtrl::HitTest");
	return 0;
}

long wxListCtrl::GetTopItem() const
{
	FAIL("wxListCtrl::GetTopItem");
	return 0;
}

void wxListCtrl::RefreshItems(long itemFrom, long itemTo)
{
	FAIL("wxListCtrl::RefreshItems");
}

void wxListCtrl::AssignImageList(wxImageList *imageList, int which)
{
	FAIL("wxListCtrl::AssignImageList");
}

bool wxListCtrl::EnsureVisible(long item)
{
	FAIL("wxListCtrl::EnsureVisible");
	return true;
}

int wxListCtrl::GetCountPerPage() const
{
	FAIL("wxListCtrl::GetCountPerPage");
	return 0;
}

bool wxListCtrl::GetColumn(int col, wxListItem& item) const
{
	FAIL("wxListCtrl::GetColumn");
	return true;
}

int wxListCtrl::GetItemCount() const
{
	FAIL("wxListCtrl::GetItemCount");
	return 0;
}

long wxListCtrl::GetNextItem(long item, int geometry, int state) const
{
	FAIL("wxListCtrl::GetNextItem");
	return 0;
}

bool wxListCtrl::DeleteColumn(int col)
{
	FAIL("wxListCtrl::DeleteColumn");
	return true;
}

bool wxListCtrl::DeleteItem(long item)
{
	FAIL("wxListCtrl::DeleteItem");
	return true;
}

wxString wxListCtrl::GetItemText(long item) const
{
	FAIL("wxListCtrl::GetItemText");
	return L"";
}

long wxListCtrl::InsertItem(long index, const wxString& label)
{
	FAIL("wxListCtrl::InsertItem");
	return 0;
}

wxTextCtrl* wxListCtrl::EditLabel(long item, wxClassInfo* textControlClass)
{
	FAIL("wxListCtrl::EditLabel");
	return NULL;
}

int wxListCtrl:: GetItemState(long item, long stateMask) const
{
	FAIL("wxListCtrl::GetItemState");
	return 0;
}

long wxListCtrl::InsertColumn(long col, wxListItem& info)
{
	FAIL("wxListCtrl::InsertColumn");
	return 0;
}

wxTextCtrl* wxListCtrl::GetEditControl() const
{
	FAIL("wxListCtrl::GetEditControl");
	return NULL;
}

bool wxListCtrl::GetItem(wxListItem& info) const
{
	FAIL("wxListCtrl::GetItem");
	return true;
}

bool wxListCtrl::SetColumn(int col, wxListItem& item)
{
	FAIL("wxListCtrl::SetColumn");
	return true;
}

int wxListCtrl::GetColumnCount() const
{
	FAIL("wxListCtrl::GetColumnCount");
	return 0;
}

int wxListCtrl::GetSelectedItemCount() const
{
	FAIL("wxListCtrl::GetSelectedItemCount");
	return 0;
}

int wxListCtrl::GetColumnWidth(int col) const
{
	FAIL("wxListCtrl::GetColumnWidth");
	return 0;
}

bool wxListCtrl::GetItemRect(long item, wxRect& rect, int code) const
{
	FAIL("wxListCtrl::GetItemRect");
	return true;
}

bool wxListCtrl::SetItem(wxListItem& info)
{
	FAIL("wxListCtrl::SetItem");
	return true;
}

bool wxListCtrl::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
		long style, const wxValidator& validator, const wxString& name)
{
	FAIL("wxListCtrl::Create");
	return true;
}

void wxListCtrl::SetImageList(wxImageList *imageList, int which)
{
	FAIL("wxListCtrl::SetImageList");
}

bool wxListCtrl::SetItemPtrData(long item, wxUIntPtr data)
{
	FAIL("wxListCtrl::SetItemPtrData");
	return true;
}

long wxListCtrl::SetItem(long index, int col, const wxString& label, int imageId)
{
	FAIL("wxListCtrl::SetItem");
	return 0;
}

long wxListCtrl::InsertColumn(long col, const wxString& heading, int format, int width)
{
	FAIL("wxListCtrl::InsertColumn");
	return 0;
}

