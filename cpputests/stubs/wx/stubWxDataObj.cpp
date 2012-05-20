

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/dataobj.h"

wxDataFormat::~wxDataFormat()
{
	FAIL("wxDataFormat::~wxDataFormat");
}

wxDataFormat::wxDataFormat(const wxDataFormat& rFormat)
{
	FAIL("wxDataFormat::wxDataFormat");
}

wxDataFormat& wxDataFormat::operator=(const wxDataFormat& format)
{
	FAIL("wxDataFormat::operator=");
	return *this;
}

wxDataObjectBase::~wxDataObjectBase()
{
	FAIL("wxDataObjectBase::~wxDataObjectBase");
}

void wxTextDataObject::GetAllFormats(wxDataFormat *formats, wxDataObjectBase::Direction dir) const
{
	FAIL("wxTextDataObject::GetAllFormats");
}

size_t wxTextDataObject::GetDataSize(const wxDataFormat& format) const
{
	FAIL("wxTextDataObject::GetDataSize");
	return 0;
}

bool wxTextDataObject::GetDataHere(const wxDataFormat& format, void *pBuf) const
{
	FAIL("wxTextDataObject::GetDataHere");
	return true;
}

bool wxTextDataObject::SetData(const wxDataFormat& format, size_t nLen, const void* pBuf)
{
	FAIL("wxTextDataObject::SetData");
	return true;
}

bool wxDataObject::IsSupportedFormat( const wxDataFormat& format, Direction dir) const
{
	FAIL("wxDataObject::IsSupportedFormat");
	return true;
}

void wxFileDataObject::AddFile( const wxString &filename )
{
	FAIL("wxFileDataObject::AddFile");
}

size_t wxFileDataObject::GetDataSize() const
{
	FAIL("wxFileDataObject::GetDataSize");
	return 0;
}

bool wxFileDataObject::GetDataHere(void *buf) const
{
	FAIL("wxFileDataObject::GetDataHere");
	return true;
}

bool wxFileDataObject::SetData(size_t len, const void *buf)
{
	FAIL("wxFileDataObject::SetData");
	return true;
}
