

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#define __WXMAC_CARBON__
#include "wx/imaglist.h"

wxImageList::~wxImageList()
{
	FAIL("wxImageList::~wxImageList");
}

int wxImageList::GetImageCount() const
{
	FAIL("wxImageList::GetImageCount");
	return 0;
}

bool wxImageList::GetSize( int index, int &width, int &height ) const
{
	FAIL("wxImageList::GetSize");
	return true;
}

bool wxImageList::Draw(int index, wxDC& dc, int x, int y, int flags, bool solidBackground)
{
	FAIL("wxImageList::Draw");
	return true;
}

wxClassInfo *wxImageList::GetClassInfo() const
{
	FAIL("wxImageList::GetClassInfo");
	return NULL;
}
