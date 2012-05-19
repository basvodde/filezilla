

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/menuitem.h"

void wxMenuItemBase::SetAccel(wxAcceleratorEntry *accel)
{
	FAIL("wxMenuItemBase::SetAccel");
}

wxMenuItemBase::~wxMenuItemBase()
{
	FAIL("wxMenuItemBase::~wxMenuItemBase");
}

wxMenuItemBase::wxMenuItemBase(wxMenu *parentMenu, int itemid, const wxString& text,
		const wxString& help, wxItemKind kind, wxMenu *subMenu)
{
	FAIL("wxMenuItemBase::wxMenuItemBase");
}

wxAcceleratorEntry *wxMenuItemBase::GetAccel() const
{
	FAIL("wxMenuItemBase::GetAccel");
	return NULL;
}

void wxMenuItemBase::SetText(const wxString& str)
{
	FAIL("wxMenuItem::SetText");
}

wxMenuItem::wxMenuItem(wxMenu *parentMenu, int id, const wxString& name, const wxString& help,
               wxItemKind kind, wxMenu *subMenu)
{
	FAIL("wxMenuItem::wxMenuItem");
}

wxMenuItem::~wxMenuItem()
{
	FAIL("wxMenuItem::~wxMenuItem");
}

void wxMenuItem::Check(bool bDoCheck)
{
	FAIL("wxMenuItem::Check");
}

void wxMenuItem::SetText(const wxString& strName)
{
	FAIL("wxMenuItem::SetText");
}

void wxMenuItem::Enable(bool bDoEnable)
{
	FAIL("wxMenuItem::Enable");
}

wxClassInfo *wxMenuItem::GetClassInfo() const
{
	FAIL("wxMenuItem::GetClassInfo");
	return NULL;
}

void wxMenuItem::SetBitmap(const wxBitmap& bitmap)
{
	FAIL("wxMenuItem::SetBitmap");
}

