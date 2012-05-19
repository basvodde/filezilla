
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/bitmap.h"
#include "wx/image.h"

wxBitmap wxNullBitmap;

wxBitmap::wxBitmap()
{
	FAIL("wxBitmap::wxBitmap");
}

wxBitmap::~wxBitmap()
{
	FAIL("wxBitmap::~wxBitmap");
}

bool wxBitmap::LoadFile(const wxString& name, wxBitmapType type)
{
	FAIL("wxBitmap::LoadFile")
	return true;
}

bool wxBitmap::Create(const void* data, wxBitmapType type, int width, int height, int depth)
{
	FAIL("wxBitmap::Create")
	return true;
}

void wxBitmap::SetMask(wxMask *mask)
{
	FAIL("wxBitmap::SetMask")
}

wxImage wxBitmap::ConvertToImage() const
{
	FAIL("wxBitmap::ConvertToImage")
	return wxImage();
}

wxBitmap wxBitmap::GetSubBitmap( const wxRect& rect ) const
{
	FAIL("wxBitmap::GetSubBitmap")
	return wxBitmap();
}

void wxBitmap::SetPalette(const wxPalette& palette)
{
	FAIL("wxBitmap::SetPalette")
}

void wxBitmap::SetDepth(int d)
{
	FAIL("wxBitmap::SetDepth")
}

void wxBitmap::SetWidth(int w)
{
	FAIL("wxBitmap::SetWidth")
}

wxObjectRefData *wxBitmap::CloneRefData(const wxObjectRefData *data) const
{
	FAIL("wxBitmap::CloneRefData")
	return NULL;
}

int wxBitmap::GetHeight() const
{
	FAIL("wxBitmap::GetHeight")
	return 1;
}

bool wxBitmap::Create(int width, int height, int depth)
{
	FAIL("wxBitmap::Create")
	return true;
}

bool wxBitmap::SaveFile(const wxString& name, wxBitmapType type, const wxPalette *cmap) const
{
	FAIL("wxBitmap::SaveFile")
	return false;
}

int wxBitmap::GetDepth() const
{
	FAIL("wxBitmap::GetDepth")
	return 1;
}

wxPalette* wxBitmap::GetPalette() const
{
	FAIL("wxBitmap::GetPalette")
	return NULL;
}

wxObjectRefData *wxBitmap::CreateRefData() const
{
	FAIL("wxBitmap::CreateRefData")
	return NULL;
}

void wxBitmap::SetHeight(int h)
{
	FAIL("wxBitmap::SetHeight")
}

int wxBitmap::GetWidth() const
{
	FAIL("wxBitmap::GetWidth")
	return 1;
}

wxMask *wxBitmap::GetMask() const
{
	FAIL("wxBitmap::GetMask")
	return NULL;
}

bool wxBitmap::CopyFromIcon(const wxIcon& icon)
{
	FAIL("wxBitmap::CopyFromIcon")
	return false;
}

wxClassInfo *wxBitmap::GetClassInfo() const
{
	FAIL("wxBitmap::GetClassInfo");
	return NULL;
}

bool wxBitmap::IsOk() const
{
	FAIL("wxBitmap::IsOk");
	return true;
}

wxClassInfo *wxBitmapBase::GetClassInfo() const
{
	FAIL("wxBitmapBase::GetClassInfo");
	return NULL;
}

