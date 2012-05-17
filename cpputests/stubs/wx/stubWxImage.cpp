
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



