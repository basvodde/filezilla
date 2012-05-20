
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/icon.h"

wxIcon::wxIcon()
{
	FAIL("wxIcon::wxIcon");
}

wxIcon::~wxIcon()
{
	FAIL("wxIcon::~wxIcon");
}

wxClassInfo *wxIcon::GetClassInfo() const
{
	FAIL("wxIcon::GetClassInfo");
	return NULL;
}

int wxIcon::GetHeight() const
{
	FAIL("wxIcon::GetHeight");
	return 0;
}

bool wxIcon::IsOk() const
{
	FAIL("wxIcon::IsOk");
	return true;
}

int wxIcon::GetWidth() const
{
	FAIL("wxIcon::GetWidth");
	return 0;
}

bool wxIcon::LoadFile(const wxString& name, wxBitmapType flags, int desiredWidth, int desiredHeight)
{
	FAIL("wxIcon::LoadFile");
	return true;
}
