
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/window.h"
#include "wx/font.h"

wxFontBase::~wxFontBase()
{
}

wxSize wxFontBase::GetPixelSize() const
{
	FAIL("wxFontBase::GetPixelSize");
	return wxSize();
}

bool wxFontBase::IsFixedWidth() const
{
	FAIL("wxFontBase::IsFixedWidth");
	return false;
}

void wxFontBase::DoSetNativeFontInfo(const wxNativeFontInfo& info)
{
	FAIL("wxFontBase::DoSetNativeFontInfo");
}

wxString wxFontBase::GetNativeFontInfoDesc() const
{
	FAIL("wxFontBase::GetNativeFontInfoDesc");
	return L"";
}


bool wxFontBase::SetFaceName( const wxString& faceName )
{
	FAIL("wxFontBase::SetFaceName");
	return true;
}

void wxFontBase::SetPixelSize( const wxSize& pixelSize )
{
	FAIL("wxFontBase::SetPixelSize");
}

bool wxFontBase::IsUsingSizeInPixels() const
{
	FAIL("wxFontBase::IsUsingSizeInPixels");
	return true;
}

wxFont::~wxFont()
{
}

int wxFont::GetPointSize() const
{
	FAIL("wxFont::GetPointSize");
	return 0;
}

wxSize wxFont::GetPixelSize() const
{
	FAIL("wxFont::GetPixelSize");
	return wxSize();
}

int wxFont::GetFamily() const
{
	FAIL("wxFont::GetFamily");
	return 0;
}

int wxFont::GetStyle() const
{
	FAIL("wxFont::GetStyle");
	return 0;
}

int wxFont::GetWeight() const
{
	FAIL("wxFont::GetWeight");
	return 0;
}

bool wxFont::GetUnderlined() const
{
	FAIL("wxFont::GetUnderlined");
	return false;
}

wxString wxFont::GetFaceName() const
{
	FAIL("wxFont::GetFaceName");
	return L"";
}

wxFontEncoding wxFont::GetEncoding() const
{
	FAIL("wxFont::GetEncoding");
	return wxFONTENCODING_DEFAULT;
}

const wxNativeFontInfo *wxFont::GetNativeFontInfo() const
{
	FAIL("wxFont::GetNativeFontInfo");
	return NULL;
}

void wxFont::SetPointSize(int pointSize)
{
	FAIL("wxFont::SetPointSize");
}

void wxFont::SetFamily(int family)
{
	FAIL("wxFont::SetFamily");
}

void wxFont::SetStyle(int style)
{
	FAIL("wxFont::SetStyle");
}

void wxFont::SetWeight(int weight)
{
	FAIL("wxFont::SetWeight");
}

bool wxFont::SetFaceName(const wxString& faceName)
{
	FAIL("wxFont::SetFaceName");
	return true;
}

void wxFont::SetUnderlined(bool underlined)
{
	FAIL("wxFont::SetUnderlined");
}

void wxFont::SetEncoding(wxFontEncoding encoding)
{
	FAIL("wxFont::SetEncoding");
}

bool wxFont::GetNoAntiAliasing() const
{
	FAIL("wxFont::GetNoAntiAliasing");
	return true;
}

bool wxFont::RealizeResource()
{
	FAIL("wxFont::RealizeResource");
	return true;
}

wxClassInfo *wxFont::GetClassInfo() const
{
	FAIL("wxFont::GetClassInfo");
	return NULL;
}

void wxFont::SetNoAntiAliasing( bool noAA)
{
	FAIL("wxFont::SetNoAntiAliasing");
}

