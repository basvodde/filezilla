
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/dataobj.h"

void wxwxSimpleDataObjectListNode::DeleteData()
{
	FAIL("wxwxToolBarToolsListNode::DeleteData");
}

wxDataFormat::wxDataFormat()
{
	FAIL("wxDataFormat::wxDataFormat");

}

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

bool wxDataFormat::operator==(const wxDataFormat& format) const
{
	FAIL("wxDataFormat::operator==");
	return true;
}

wxDataFormat::wxDataFormat(const wxChar* pId)
{
	FAIL("wxDataFormat::wxDataFormat");
}

wxDataFormat::wxDataFormat(wxDataFormatId vType)
{
	FAIL("wxDataFormat::wxDataFormat");
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

wxDataObject::wxDataObject()
{
	FAIL("wxDataObject::wxDataObject");
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

wxDataObjectComposite::wxDataObjectComposite()
{
	FAIL("wxFileDataObject::wxDataObjectComposite");

}

wxDataObjectComposite::~wxDataObjectComposite()
{
	FAIL("wxDataObjectComposite::~wxDataObjectComposite");
}

void wxDataObjectComposite::Add(wxDataObjectSimple *dataObject, bool preferred)
{
	FAIL("wxDataObjectComposite::Add");
}

wxDataFormat wxDataObjectComposite::GetReceivedFormat() const
{
	FAIL("wxDataObjectComposite::GetReceivedFormat");
	return wxDataFormat();
}

wxDataFormat wxDataObjectComposite::GetPreferredFormat(wxDataObjectBase::Direction dir) const
{
	FAIL("wxDataObjectComposite::GetPreferredFormat");
	return wxDataFormat();
}

size_t wxDataObjectComposite::GetFormatCount(wxDataObjectBase::Direction dir) const
{
	FAIL("wxDataObjectComposite::GetFormatCount");
	return 0;
}

void wxDataObjectComposite::GetAllFormats(wxDataFormat *formats, wxDataObjectBase::Direction dir) const
{
	FAIL("wxDataObjectComposite::GetAllFormats");
}

size_t wxDataObjectComposite::GetDataSize(const wxDataFormat& format) const
{
	FAIL("wxDataObjectComposite::GetDataSize");
	return 0;
}

bool wxDataObjectComposite::GetDataHere(const wxDataFormat& format, void *buf) const
{
	FAIL("wxDataObjectComposite::GetDataHere");
	return true;
}

bool wxDataObjectComposite::SetData(const wxDataFormat& format, size_t len, const void *buf)
{
	FAIL("wxDataObjectComposite::SetData");
	return true;
}



