
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/region.h"

wxRegion::wxRegion()
{
}

wxRegion::~wxRegion()
{
}

wxClassInfo *wxRegion::GetClassInfo() const
{
	FAIL("wxRegion::GetClassInfo");
	return NULL;
}

void wxRegion::Clear()
{
	FAIL("wxRegion::Clear");
}

bool wxRegion::IsEmpty() const
{
	FAIL("wxRegion::IsEmpty");
	return false;
}

bool wxRegion::DoIsEqual(const wxRegion& region) const
{
	FAIL("wxRegion::DoIsEqual");
	return false;
}

bool wxRegion::DoGetBox(wxCoord& x, wxCoord& y, wxCoord& w, wxCoord& h) const
{
	FAIL("wxRegion::DoGetBox");
	return true;
}

wxRegionContain wxRegion::DoContainsPoint(wxCoord x, wxCoord y) const
{
	FAIL("wxRegion::DoContainsPoint");
	return wxRegionContain();
}

wxRegionContain wxRegion::DoContainsRect(const wxRect& rect) const
{
	FAIL("wxRegion::DoContainsRect");
	return wxRegionContain();
}

bool wxRegion::DoCombine(const wxRegion& region, wxRegionOp op)
{
	FAIL("wxRegion::DoCombine");
	return true;
}

bool wxRegion::DoOffset(wxCoord x, wxCoord y)
{
	FAIL("wxRegion::DoOffset");
	return true;
}

bool wxRegionWithCombine::DoUnionWithRect(const wxRect& rect)
{
	FAIL("wxRegionWithCombine::DoUnionWithRect");
	return true;
}

bool wxRegionWithCombine::DoUnionWithRegion(const wxRegion& region)
{
	FAIL("wxRegionWithCombine::DoUnionWithRegion");
	return true;
}

bool wxRegionWithCombine::DoIntersect(const wxRegion& region)
{
	FAIL("wxRegionWithCombine::DoIntersect");
	return true;
}

bool wxRegionWithCombine::DoSubtract(const wxRegion& region)
{
	FAIL("wxRegionWithCombine::DoSubtract");
	return true;
}

bool wxRegionWithCombine::DoXor(const wxRegion& region)
{
	FAIL("wxRegionWithCombine::DoXor");
	return true;
}





