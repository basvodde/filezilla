
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/menu.h"

IMPLEMENT_DYNAMIC_CLASS(wxMenu, wxMenuBase);
IMPLEMENT_DYNAMIC_CLASS(wxMenuBar, wxMenuBarBase);

void wxwxMenuListNode::DeleteData()
{
	FAIL("wxwxToolBarToolsListNode::DeleteData");
}

void wxwxMenuItemListNode::DeleteData()
{
	FAIL("wxwxToolBarToolsListNode::DeleteData");
}

void wxMenuBase::SetHelpString(int itemid, const wxString& helpString)
{
	FAIL("wxMenuBase::SetHelpString");
}

wxString wxMenuBase::GetHelpString(int itemid) const
{
	FAIL("wxMenuBase::GetHelpString");
	return L"";
}

wxMenuBase::~wxMenuBase()
{
	FAIL("wxMenuBase::~wxMenuBase");
}

wxMenuItem* wxMenuBase::DoAppend(wxMenuItem *item)
{
	FAIL("wxMenuBase::DoAppend");
	return NULL;
}

wxMenuItem* wxMenuBase::DoInsert(size_t pos, wxMenuItem *item)
{
	FAIL("wxMenuBase::DoInsert");
	return NULL;
}

wxMenuItem *wxMenuBase::DoRemove(wxMenuItem *item)
{
	FAIL("wxMenuBase::DoRemove");
	return NULL;
}

bool wxMenuBase::DoDelete(wxMenuItem *item)
{
	FAIL("wxMenuBase::DoDelete");
	return true;
}

bool wxMenuBase::DoDestroy(wxMenuItem *item)
{
	FAIL("wxMenuBase::DoDestroy");
	return true;
}

int wxMenuBase::FindItem(const wxString& item) const
{
	FAIL("wxMenuBase::FindItem");
	return 0;
}

void wxMenuBase::Attach(wxMenuBarBase *menubar)
{
	FAIL("wxMenuBase::Attach");
}

void wxMenuBase::Detach()
{
	FAIL("wxMenuBase::Detach");
}

wxMenuItem* wxMenuBase::FindItem(int itemid, wxMenu **menu) const
{
	FAIL("wxMenuBase::FindItem");
	return NULL;
}

void wxMenuBase::Check(int itemid, bool check)
{
	FAIL("wxMenuBase::Check");
}

wxMenuItem* wxMenuBase::FindItemByPosition(size_t position) const
{
	FAIL("wxMenuBase::FindItemByPosition");
	return NULL;
}

void wxMenuBase::Init(long style)
{
	FAIL("wxMenuBase::Init");
}

wxMenuItem *wxMenuBase::FindChildItem(int itemid, size_t *pos) const
{
	FAIL("wxMenuBase::FindChildItem");
	return NULL;
}

wxMenuItem* wxMenuBase::Insert(size_t pos, wxMenuItem *item)
{
	FAIL("wxMenuBase::Insert");
	return NULL;
}

void wxMenuBase::Enable(int itemid, bool enable)
{
	FAIL("wxMenuBase::Enable");
}

wxMenuItem *wxMenuBase::Remove(wxMenuItem *item)
{
	FAIL("wxMenuBase::Remove");
	return NULL;
}

bool wxMenuBase::Delete(wxMenuItem *item)
{
	FAIL("wxMenuBase::Delete");
	return true;
}

void wxMenu::Init()
{
	FAIL("wxMenu::Init");
}

wxMenu::~wxMenu()
{
	FAIL("wxMenu::~wxMenu");
}

void wxMenu::Attach(wxMenuBarBase *menubar)
{
	FAIL("wxMenu::Attach");
}

void wxMenu::Break()
{
	FAIL("wxMenu::Break");
}

void wxMenu::SetTitle(const wxString& title)
{
	FAIL("wxMenu::SetTitle");
}

wxMenuItem* wxMenu::DoAppend(wxMenuItem *item)
{
	FAIL("wxMenu::DoAppend");
	return NULL;
}

wxMenuItem* wxMenu::DoInsert(size_t pos, wxMenuItem *item)
{
	FAIL("wxMenu::DoInsert");
	return NULL;
}

wxMenuItem* wxMenu::DoRemove(wxMenuItem *item)
{
	FAIL("wxMenu::DoRemove");
	return NULL;
}

wxMenuBarBase::wxMenuBarBase()
{
	FAIL("wxMenuBarBase::wxMenuBarBase");
}

wxMenuBarBase::~wxMenuBarBase()
{
	FAIL("wxMenuBarBase::~wxMenuBarBase");
}

wxMenu *wxMenuBarBase::Replace(size_t pos, wxMenu *menu, const wxString& title)
{
	FAIL("wxMenuBarBase::Replace");
	return NULL;
}

wxMenuItem* wxMenuBarBase::FindItem(int itemid, wxMenu **menu) const
{
	FAIL("wxMenuBarBase::FindItem");
	return NULL;
}

int wxMenuBarBase::FindMenuItem(const wxString& menu, const wxString& item) const
{
	FAIL("wxMenuBarBase::FindMenuItem");
	return 0;
}

bool wxMenuBarBase::Append(wxMenu *menu, const wxString& title)
{
	FAIL("wxMenuBarBase::Append");
	return true;
}

void wxMenuBarBase::UpdateMenus()
{
	FAIL("wxMenuBarBase::UpdateMenus");
}

bool wxMenuBarBase::Insert(size_t pos, wxMenu *menu, const wxString& title)
{
	FAIL("wxMenuBarBase::Insert");
	return true;
}

void wxMenuBarBase::Attach(wxFrame *frame)
{
	FAIL("wxMenuBarBase::Attach");
}

void wxMenuBarBase::Detach()
{
	FAIL("wxMenuBarBase::Detach");
}

wxMenu *wxMenuBarBase::Remove(size_t pos)
{
	FAIL("wxMenuBarBase::Remove");
	return NULL;
}

void wxMenuBarBase::Enable(int itemid, bool enable)
{
	FAIL("wxMenuBarBase::Enable");
}

void wxMenuBarBase::Check(int itemid, bool check)
{
	FAIL("wxMenuBarBase::Check");
}

bool wxMenuBar::Insert(size_t pos, wxMenu *menu, const wxString& title)
{
	FAIL("wxMenuBar::Insert");
	return true;
}

void wxMenuBar::SetLabelTop( size_t pos, const wxString& label )
{
	FAIL("wxMenuBar::SetLabelTop");
}

wxMenu *wxMenuBar::Replace(size_t pos, wxMenu *menu, const wxString& title)
{
	FAIL("wxMenuBar::Replace");
	return NULL;
}

int wxMenuBar::FindMenuItem(const wxString& menuString, const wxString& itemString) const
{
	FAIL("wxMenuBar::FindMenuItem");
	return 0;
}

void wxMenuBar::Attach(wxFrame *frame)
{
	FAIL("wxMenuBar::Attach");
}

bool wxMenuBar::Append( wxMenu *menu, const wxString &title )
{
	FAIL("wxMenuBar::Append");
	return true;
}

wxMenuBar::wxMenuBar(long style)
{
	FAIL("wxMenuBar::wxMenuBar");
}

wxMenuBar::wxMenuBar()
{
	FAIL("wxMenuBar::wxMenuBar");
}

wxMenuBar::~wxMenuBar()
{
	FAIL("wxMenuBar::~wxMenuBar");
}

void wxMenuBar::Refresh(bool eraseBackground, const wxRect *rect)
{
	FAIL("wxMenuBar::Refresh");
}

wxMenuItem* wxMenuBar::FindItem( int id, wxMenu **menu) const
{
	FAIL("wxMenuBar::FindItem");
	return NULL;
}

void wxMenuBar::Detach()
{
	FAIL("wxMenuBar::Detach");
}

bool wxMenuBar::Enable( bool enable)
{
	FAIL("wxMenuBar::Enable");
	return true;
}

wxMenu *wxMenuBar::Remove(size_t pos)
{
	FAIL("wxMenuBar::Remove");
	return NULL;
}

wxString wxMenuBar::GetLabelTop( size_t pos ) const
{
	FAIL("wxMenuBar::GetLabelTop");
	return L"";
}


void wxMenuBar::EnableTop( size_t pos, bool flag )
{
	FAIL("wxMenuBar::EnableTop");
}




