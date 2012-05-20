
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/menu.h"

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

wxClassInfo *wxMenu::GetClassInfo() const
{
	FAIL("wxMenu::GetClassInfo");
	return NULL;
}


