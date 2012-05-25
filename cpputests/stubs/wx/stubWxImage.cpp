
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/image.h"

wxClassInfo *wxImage::GetClassInfo() const
{
	FAIL("wxImage::GetClassInfo");
	return NULL;
}

bool wxImage::LoadFile( const wxString& name, long type, int index)
{
	FAIL("wxImage::LoadFile");
	return true;
}

bool wxImage::LoadFile( wxInputStream& stream, long type, int index)
{
	FAIL("wxImage::LoadFile");
	return true;
}

wxObjectRefData* wxImage::CloneRefData(const wxObjectRefData* data) const
{
	FAIL("wxImage::CloneRefData");
	return NULL;
}

bool wxImage::LoadFile( wxInputStream& stream, const wxString& mimetype, int index)
{
	FAIL("wxImage::LoadFile");
	return true;
}

wxObjectRefData* wxImage::CreateRefData() const
{
	FAIL("wxImage::CreateRefData");
	return NULL;
}

bool wxImage::LoadFile( const wxString& name, const wxString& mimetype, int index)
{
	FAIL("wxImage::LoadFile");
	return true;
}

bool wxImage::SaveFile( const wxString& name ) const
{
	FAIL("wxImage::SaveFile");
	return true;
}
bool wxImage::SaveFile( const wxString& name, int type ) const
{
	FAIL("wxImage::SaveFile");
	return true;
}

bool wxImage::SaveFile( const wxString& name, const wxString& mimetype ) const
{
	FAIL("wxImage::SaveFile");
	return true;
}

bool wxImage::SaveFile( wxOutputStream& stream, int type ) const
{
	FAIL("wxImage::SaveFile");
	return true;
}

bool wxImage::SaveFile( wxOutputStream& stream, const wxString& mimetype ) const
{
	FAIL("wxImage::SaveFile");
	return true;
}

void wxImage::AddHandler( wxImageHandler *handler )
{
	FAIL("wxImage::AddHandler");
}

bool wxImageHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose, int index)
{
	FAIL("wxImageHandler::LoadFile");
	return true;
}

bool wxImageHandler::SaveFile( wxImage *image, wxOutputStream& stream, bool verbose)
{
	FAIL("wxImageHandler::SaveFile");
	return true;
}

bool wxImageHandler::DoCanRead( wxInputStream& stream )
{
	FAIL("wxImageHandler::DoCanRead");
	return true;
}

int wxImageHandler::GetImageCount( wxInputStream& stream )
{
	FAIL("wxImageHandler::GetImageCount");
	return 0;
}

wxClassInfo *wxImageHandler::GetClassInfo() const
{
	FAIL("wxImageHandler::GetClassInfo");
	return NULL;
}

wxImage wxImage::ConvertToGreyscale( double lr, double lg, double lb) const
{
	FAIL("wxImageHandler::ConvertToGreyscale");
	return wxImage();
}

bool wxImage::HasMask() const
{
	FAIL("wxImageHandler::HasMask");
	return true;
}

wxImage wxImage::Scale( int width, int height, int quality) const
{
	FAIL("wxImageHandler::Scale");
	return wxImage();
}

bool wxImage::IsOk() const
{
	FAIL("wxImageHandler::IsOk");
	return true;
}

wxImage::wxImage( const wxString& name, long type, int index)
{
	FAIL("wxImageHandler::GetClassInfo");
}

void wxImage::InitAlpha()
{
	FAIL("wxImageHandler::InitAlpha");
}

unsigned char wxImage::GetAlpha(int x, int y) const
{
	FAIL("wxImageHandler::GetAlpha");
	return 'a';
}

wxImage wxImage::GetSubImage( const wxRect& rect) const
{
	FAIL("wxImageHandler::GetSubImage");
	return wxImage();
}

unsigned char* wxImage::GetAlpha() const
{
	FAIL("wxImageHandler::GetAlpha");
	return NULL;
}




