
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/dc.h"

void wxDCBase::DrawLabel(const wxString& text, const wxBitmap& image, const wxRect& rect,
		int alignment, int indexAccel, wxRect *rectBounding)
{
	FAIL("wxDCBase::DrawLabel");
}

void wxDCBase::DoDrawCheckMark(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
{
	FAIL("wxDCBase::DoDrawCheckMark");
}

void wxDCBase::DoDrawPolyPolygon(int n, int count[], wxPoint points[], wxCoord xoffset,
		wxCoord yoffset, int fillStyle)
{
	FAIL("wxDCBase::DoDrawPolyPolygon");
}
void wxDCBase::DoGradientFillLinear(const wxRect& rect, const wxColour& initialColour,
		const wxColour& destColour, wxDirection nDirection)
{
	FAIL("wxDCBase::DoGradientFillLinear");
}

void wxDCBase::DoGradientFillConcentric(const wxRect& rect, const wxColour& initialColour,
		const wxColour& destColour, const wxPoint& circleCenter)
{
	FAIL("wxDCBase::DoGradientFillConcentric");
}

void wxDCBase::GetMultiLineTextExtent(const wxString& string, wxCoord *width, wxCoord *height,
		wxCoord *heightLine, wxFont *font) const
{
	FAIL("wxDCBase::GetMultiLineTextExtent");
}

bool wxDCBase::DoGetPartialTextExtents(const wxString& text, wxArrayInt& widths) const
{
	FAIL("wxDCBase::DoGetPartialTextExtents");
	return true;
}

void wxDCBase::DoDrawSpline(wxList *points)
{
	FAIL("wxDCBase::DoDrawSpline");
}

wxClassInfo *wxDCBase::GetClassInfo() const
{
	FAIL("wxDCBase::GetClassInfo");
	return NULL;
}

wxClassInfo *wxDC::GetClassInfo() const
{
	FAIL("wxDC::GetClassInfo");
	return NULL;
}

void wxDC::DoGetSize(int *width, int *height) const
{
	FAIL("wxDC::DoGetSize");
}

void wxDC::SetFont(const wxFont& font)
{
	FAIL("wxDC::SetFont");
}

void wxDC::SetPen(const wxPen& pen)
{
	FAIL("wxDC::SetPen");
}

void wxDC::SetBrush(const wxBrush& brush)
{
	FAIL("wxDC::SetBrush");
}

void wxDC::SetBackground(const wxBrush& brush)
{
	FAIL("wxDC::SetBackground");
}

void wxDC::SetBackgroundMode(int mode)
{
	FAIL("wxDC::SetBackgroundMode");
}

void wxDC::SetPalette(const wxPalette& palette)
{
	FAIL("wxDC::SetPalette");
}

void wxDC::DestroyClippingRegion()
{
	FAIL("wxDC::DestroyClippingRegion");
}

wxCoord wxDC::GetCharHeight() const
{
	FAIL("wxDC::GetCharHeight");
	return wxCoord();
}

wxCoord wxDC::GetCharWidth() const
{
	FAIL("wxDC::GetCharWidth");
	return wxCoord();
}

bool wxDC::CanDrawBitmap() const
{
	FAIL("wxDC::CanDrawBitmap");
	return true;
}

bool wxDC::CanGetTextExtent() const
{
	FAIL("wxDC::CanGetTextExtent");
	return true;
}

int wxDC::GetDepth() const
{
	FAIL("wxDC::GetDepth");
	return 0;
}

wxSize wxDC::GetPPI() const
{
	FAIL("wxDC::GetPPI");
	return wxSize();
}

void wxDC::SetMapMode(int mode)
{
	FAIL("wxDC::SetMapMode");
}

void wxDC::SetUserScale(double x, double y)
{
	FAIL("wxDC::SetUserScale");
}

void wxDC::SetLogicalScale(double x, double y)
{
	FAIL("wxDC::SetLogicalScale");
}

void wxDC::SetLogicalOrigin(wxCoord x, wxCoord y)
{
	FAIL("wxDC::SetLogicalOrigin");
}

void wxDC::SetDeviceOrigin(wxCoord x, wxCoord y)
{
	FAIL("wxDC::SetDeviceOrigin");
}

void wxDC::SetAxisOrientation(bool xLeftRight, bool yBottomUp)
{
	FAIL("wxDC::SetAxisOrientation");
}

void wxDC::SetLogicalFunction(int function)
{
	FAIL("wxDC::SetLogicalFunction");
}

void wxDC::SetTextForeground(const wxColour& colour)
{
	FAIL("wxDC::SetTextForeground");
}

void wxDC::SetTextBackground(const wxColour& colour)
{
	FAIL("wxDC::SetTextBackground");
}

void wxDC:: ComputeScaleAndOrigin()
{
	FAIL("wxDC::ComputeScaleAndOrigin");
}

void wxDC:: DoGetTextExtent(const wxString& string, wxCoord *x, wxCoord *y, wxCoord *descent,
		wxCoord *externalLeading, wxFont *theFont) const
{
	FAIL("wxDC::DoGetTextExtent");
}

bool wxDC::DoGetPartialTextExtents(const wxString& text, wxArrayInt& widths) const
{
	FAIL("wxDC::DoGetPartialTextExtents");
	return true;
}

bool wxDC::DoFloodFill(wxCoord x, wxCoord y, const wxColour& col, int style)
{
	FAIL("wxDC::DoFloodFill");
	return true;
}

bool wxDC::DoGetPixel(wxCoord x, wxCoord y, wxColour *col) const
{
	FAIL("wxDC::DoGetPixel");
	return true;
}

void wxDC:: DoDrawPoint(wxCoord x, wxCoord y)
{
	FAIL("wxDC::DoDrawPoint");
}

void wxDC:: DoDrawSpline(wxList *points)
{
	FAIL("wxDC::DoDrawSpline");
}

void wxDC:: DoDrawLine(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2)
{
	FAIL("wxDC::DoDrawLine");
}

void wxDC:: DoDrawArc(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2, wxCoord xc, wxCoord yc)
{
	FAIL("wxDC::DoDrawArc");
}

void wxDC:: DoDrawEllipticArc(wxCoord x, wxCoord y, wxCoord w, wxCoord h, double sa, double ea)
{
	FAIL("wxDC::DoDrawEllipticArc");
}

void wxDC:: DoDrawRectangle(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
{
	FAIL("wxDC::DoDrawRectangle");
}

void wxDC:: DoDrawRoundedRectangle(wxCoord x, wxCoord y, wxCoord width, wxCoord height, double radius)
{
	FAIL("wxDC::DoDrawRoundedRectangle");
}

void wxDC:: DoDrawEllipse(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
{
	FAIL("wxDC::DoDrawEllipse");
}

void wxDC:: DoCrossHair(wxCoord x, wxCoord y)
{
	FAIL("wxDC::DoCrossHair");
}

void wxDC:: DoDrawIcon(const wxIcon& icon, wxCoord x, wxCoord y)
{
	FAIL("wxDC::DoDrawIcon");
}

void wxDC:: DoDrawBitmap(const wxBitmap &bmp, wxCoord x, wxCoord y, bool useMask)
{
	FAIL("wxDC::DoDrawBitmap");
}

void wxDC:: DoDrawText(const wxString& text, wxCoord x, wxCoord y)
{
	FAIL("wxDC::DoDrawText");
}

void wxDC:: DoDrawRotatedText(const wxString& text, wxCoord x, wxCoord y, double angle)
{
	FAIL("wxDC::DoDrawRotatedText");
}

bool wxDC::DoBlit(wxCoord xdest, wxCoord ydest, wxCoord width, wxCoord height, wxDC *source,
		wxCoord xsrc, wxCoord ysrc, int rop, bool useMask, wxCoord xsrcMask, wxCoord ysrcMask)
{
	FAIL("wxDC::DoBlit");
	return true;
}

void wxDC:: DoSetClippingRegionAsRegion(const wxRegion& region)
{
	FAIL("wxDC::DoSetClippingRegionAsRegion");
}

void wxDC:: DoSetClippingRegion(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
{
	FAIL("wxDC::DoSetClippingRegion");
}

void wxDC:: DoGetSizeMM(int *width, int *height) const
{
	FAIL("wxDC::DoGetSizeMM");
}

void wxDC:: DoDrawLines(int n, wxPoint points[], wxCoord xoffset, wxCoord yoffset)
{
	FAIL("wxDC::DoDrawLines");
}

void wxDC::DoDrawPolygon(int n, wxPoint points[], wxCoord xoffset, wxCoord yoffset, int fillStyle)
{
	FAIL("wxDC::DoDrawPolygon");
}

void wxDC::DoDrawPolyPolygon(int n, int count[], wxPoint points[], wxCoord xoffset, wxCoord yoffset, int fillStyle)
{
	FAIL("wxDC::DoDrawPolyPolygon");
}

void wxDC::DoGradientFillLinear(const wxRect& rect, const wxColour& initialColour,
		const wxColour& destColour, wxDirection nDirection)
{
	FAIL("wxDC::DoGradientFillLinear");
}

void wxDC::DoGradientFillConcentric(const wxRect& rect, const wxColour& initialColour,
    const wxColour& destColour, const wxPoint& circleCenter)
{
	FAIL("wxDC::DoGradientFillConcentric");
}

void wxDC::DoDrawCheckMark(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
{
	FAIL("wxDC::DoDrawCheckMark");
}

void wxDC::SetGraphicsContext( wxGraphicsContext* ctx )
{
	FAIL("wxDC::SetGraphicsContext");
}

void wxDC::Clear()
{
	FAIL("wxDC::Clear");
}

bool wxDC::StartDoc( const wxString& message)
{
	FAIL("wxDC::StartDoc");
	return true;
}

void wxDC::EndDoc()
{
	FAIL("wxDC::EndDoc");
}

void wxDC::StartPage()
{
	FAIL("wxDC::StartPage");
}

void wxDC::EndPage()
{
	FAIL("wxDC::EndPage");
}

wxDC::~wxDC()
{
	FAIL("wxDC::~wxDC");
}

