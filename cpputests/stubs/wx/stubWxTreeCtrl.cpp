
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/treectrl.h"

wxTreeCtrlBase::~wxTreeCtrlBase()
{
	FAIL("wxTreeCtrlBase::~wxTreeCtrlBase");
}

wxSize wxTreeCtrlBase::DoGetBestSize() const
{
	FAIL("wxTreeCtrlBase::DoGetBestSize");
	return wxSize();
}

const wxEventTable wxGenericTreeCtrl::sm_eventTable = wxEventTable();
wxEventHashTable wxGenericTreeCtrl::sm_eventHashTable (wxGenericTreeCtrl::sm_eventTable);

const wxEventTable* wxGenericTreeCtrl::GetEventTable() const
{
	FAIL("wxGenericTreeCtrl::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxGenericTreeCtrl::GetEventHashTable() const
{
	FAIL("wxGenericTreeCtrl::GetEventHashTable");
	return sm_eventHashTable;
}

bool wxGenericTreeCtrl::SetBackgroundColour(const wxColour& colour)
{
	FAIL("wxGenericTreeCtrl::SetBackgroundColour");
	return true;
}

bool wxGenericTreeCtrl::SetForegroundColour(const wxColour& colour)
{
	FAIL("wxGenericTreeCtrl::SetForegroundColour");
	return true;
}

void wxGenericTreeCtrl::Freeze()
{
	FAIL("wxGenericTreeCtrl::Freeze");
}

void wxGenericTreeCtrl::Thaw()
{
	FAIL("wxGenericTreeCtrl::Thaw");
}

void wxGenericTreeCtrl::Refresh(bool eraseBackground, const wxRect *rect)
{
	FAIL("wxGenericTreeCtrl::Refresh");
}

bool wxGenericTreeCtrl::SetFont( const wxFont &font )
{
	FAIL("wxGenericTreeCtrl::SetFont");
	return true;
}

void wxGenericTreeCtrl::SetWindowStyle(const long styles)
{
	FAIL("wxGenericTreeCtrl::SetWindowStyle");
}

unsigned int wxGenericTreeCtrl::GetCount() const
{
	FAIL("wxGenericTreeCtrl::GetCount");
	return 0;
}

void wxGenericTreeCtrl::SetIndent(unsigned int indent)
{
	FAIL("wxGenericTreeCtrl::SetIndent");
}

void wxGenericTreeCtrl::SetImageList(wxImageList *imageList)
{
	FAIL("wxGenericTreeCtrl::SetImageList");
}

void wxGenericTreeCtrl::SetStateImageList(wxImageList *imageList)
{
	FAIL("wxGenericTreeCtrl::SetStateImageList");
}

wxString wxGenericTreeCtrl::GetItemText(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetItemText");
	return L"";
}

int wxGenericTreeCtrl::GetItemImage(const wxTreeItemId& item, wxTreeItemIcon which) const
{
	FAIL("wxGenericTreeCtrl::GetItemImage");
	return 0;
}

wxTreeItemData *wxGenericTreeCtrl::GetItemData(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetItemData");
	return NULL;
}

wxColour wxGenericTreeCtrl::GetItemTextColour(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetItemTextColour");
	return wxColour();
}

wxColour wxGenericTreeCtrl::GetItemBackgroundColour(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetItemBackgroundColour");
	return wxColour();
}

wxFont wxGenericTreeCtrl::GetItemFont(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetItemFont");
	return wxFont();
}

void wxGenericTreeCtrl::SetItemText(const wxTreeItemId& item, const wxString& text)
{
	FAIL("wxGenericTreeCtrl::SetItemText");
}

void wxGenericTreeCtrl::SetItemImage(const wxTreeItemId& item, int image, wxTreeItemIcon which)
{
	FAIL("wxGenericTreeCtrl::SetItemImage");
}

void wxGenericTreeCtrl::SetItemData(const wxTreeItemId& item, wxTreeItemData *data)
{
	FAIL("wxGenericTreeCtrl::SetItemData");
}

void wxGenericTreeCtrl::SetItemHasChildren(const wxTreeItemId& item, bool has)
{
	FAIL("wxGenericTreeCtrl::SetItemHasChildren");
}

void wxGenericTreeCtrl::SetItemBold(const wxTreeItemId& item, bool bold)
{
	FAIL("wxGenericTreeCtrl::SetItemBold");
}

void wxGenericTreeCtrl::SetItemDropHighlight(const wxTreeItemId& item, bool highlight)
{
	FAIL("wxGenericTreeCtrl::SetItemDropHighlight");
}

void wxGenericTreeCtrl::SetItemTextColour(const wxTreeItemId& item, const wxColour& col)
{
	FAIL("wxGenericTreeCtrl::SetItemTextColour");
}

void wxGenericTreeCtrl::SetItemBackgroundColour(const wxTreeItemId& item, const wxColour& col)
{
	FAIL("wxGenericTreeCtrl::SetItemBackgroundColour");
}

void wxGenericTreeCtrl::SetItemFont(const wxTreeItemId& item, const wxFont& font)
{
	FAIL("wxGenericTreeCtrl::SetItemFont");
}

bool wxGenericTreeCtrl::IsVisible(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::IsVisible");
	return true;
}

bool wxGenericTreeCtrl::ItemHasChildren(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::ItemHasChildren");
	return true;
}

bool wxGenericTreeCtrl::IsExpanded(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::IsExpanded");
	return true;
}

bool wxGenericTreeCtrl::IsSelected(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::IsSelected");
	return true;
}

bool wxGenericTreeCtrl::IsBold(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::IsBold");
	return true;
}

size_t wxGenericTreeCtrl::GetChildrenCount(const wxTreeItemId& item, bool recursively) const
{
	FAIL("wxGenericTreeCtrl::GetChildrenCount");
	return 0;
}

size_t wxGenericTreeCtrl::GetSelections(wxArrayTreeItemIds&) const
{
	FAIL("wxGenericTreeCtrl::GetSelections");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::GetItemParent(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetItemParent");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::GetFirstChild(const wxTreeItemId& item, wxTreeItemIdValue& cookie) const
{
	FAIL("wxGenericTreeCtrl::GetFirstChild");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::GetNextChild(const wxTreeItemId& item, wxTreeItemIdValue& cookie) const
{
	FAIL("wxGenericTreeCtrl::GetNextChild");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::GetLastChild(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetLastChild");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::GetNextSibling(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetNextSibling");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::GetPrevSibling(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetPrevSibling");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::GetFirstVisibleItem() const
{
	FAIL("wxGenericTreeCtrl::GetFirstVisibleItem");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::GetNextVisible(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetNextVisible");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::GetPrevVisible(const wxTreeItemId& item) const
{
	FAIL("wxGenericTreeCtrl::GetPrevVisible");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::AddRoot(const wxString& text, int image, int selectedImage, wxTreeItemData *data)
{
	FAIL("wxGenericTreeCtrl::AddRoot");
	return 0;
}

void wxGenericTreeCtrl::Delete(const wxTreeItemId& item)
{
	FAIL("wxGenericTreeCtrl::Delete");
}

void wxGenericTreeCtrl::DeleteChildren(const wxTreeItemId& item)
{
	FAIL("wxGenericTreeCtrl::DeleteChildren");
}

void wxGenericTreeCtrl::DeleteAllItems()
{
	FAIL("wxGenericTreeCtrl::DeleteAllItems");
}

void wxGenericTreeCtrl::Expand(const wxTreeItemId& item)
{
	FAIL("wxGenericTreeCtrl::Expand");
}

void wxGenericTreeCtrl::Collapse(const wxTreeItemId& item)
{
	FAIL("wxGenericTreeCtrl::Collapse");
}

void wxGenericTreeCtrl::CollapseAndReset(const wxTreeItemId& item)
{
	FAIL("wxGenericTreeCtrl::CollapseAndReset");
}

void wxGenericTreeCtrl::Toggle(const wxTreeItemId& item)
{
	FAIL("wxGenericTreeCtrl::Toggle");
}

void wxGenericTreeCtrl::Unselect()
{
	FAIL("wxGenericTreeCtrl::Unselect");
}

void wxGenericTreeCtrl::UnselectAll()
{
	FAIL("wxGenericTreeCtrl::UnselectAll");
}

void wxGenericTreeCtrl::SelectItem(const wxTreeItemId& item, bool select)
{
	FAIL("wxGenericTreeCtrl::SelectItem");
}

void wxGenericTreeCtrl::EnsureVisible(const wxTreeItemId& item)
{
	FAIL("wxGenericTreeCtrl::EnsureVisible");
}

void wxGenericTreeCtrl::ScrollTo(const wxTreeItemId& item)
{
	FAIL("wxGenericTreeCtrl::ScrollTo");
}

wxTextCtrl *wxGenericTreeCtrl::EditLabel(const wxTreeItemId& item, wxClassInfo* textCtrlClass)
{
	FAIL("wxGenericTreeCtrl::EditLabel");
	return NULL;
}

wxTextCtrl *wxGenericTreeCtrl::GetEditControl() const
{
	FAIL("wxGenericTreeCtrl::GetEditControl");
	return NULL;
}

void wxGenericTreeCtrl::EndEditLabel(const wxTreeItemId& item, bool discardChanges)
{
	FAIL("wxGenericTreeCtrl::EndEditLabel");
}

void wxGenericTreeCtrl::SortChildren(const wxTreeItemId& item)
{
	FAIL("wxGenericTreeCtrl::SortChildren");
}

wxTreeItemId wxGenericTreeCtrl::DoInsertItem(const wxTreeItemId& parent, size_t previous, const wxString& text,
		int image, int selectedImage, wxTreeItemData *data)
{
	FAIL("wxGenericTreeCtrl::DoInsertItem");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::DoInsertAfter(const wxTreeItemId& parent, const wxTreeItemId& idPrevious, const wxString& text,
		int image, int selImage, wxTreeItemData *data)
{
	FAIL("wxGenericTreeCtrl::DoInsertAfter");
	return 0;
}

wxTreeItemId wxGenericTreeCtrl::DoTreeHitTest(const wxPoint& point, int& flags) const
{
	FAIL("wxGenericTreeCtrl::DoTreeHitTest");
	return 0;
}

wxGenericTreeCtrl::~wxGenericTreeCtrl()
{
	FAIL("wxGenericTreeCtrl::~wxGenericTreeCtrl");
}

wxVisualAttributes wxGenericTreeCtrl::GetClassDefaultAttributes(wxWindowVariant variant)
{
	FAIL("wxGenericTreeCtrl::GetClassDefaultAttributes");
	return wxVisualAttributes();
}

wxSize wxGenericTreeCtrl::DoGetBestSize() const
{
	FAIL("wxGenericTreeCtrl::DoGetBestSize");
	return wxSize();
}

void wxGenericTreeCtrl::OnInternalIdle( )
{
	FAIL("wxGenericTreeCtrl::OnInternalIdle");
}

bool wxGenericTreeCtrl::GetBoundingRect(const wxTreeItemId& item, wxRect& rect, bool textOnly) const
{
	FAIL("wxGenericTreeCtrl::GetBoundingRect");
	return true;
}

wxClassInfo *wxGenericTreeCtrl::GetClassInfo() const
{
	FAIL("wxGenericTreeCtrl::GetClassInfo");
	return NULL;
}


wxClassInfo *wxTreeCtrl::GetClassInfo() const
{
	FAIL("wxTreeCtrl::GetClassInfo");
	return NULL;
}
