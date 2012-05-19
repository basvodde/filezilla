
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/graphics.h"

wxGraphicsObject::~wxGraphicsObject()
{
	FAIL("wxGraphicsObject::~wxGraphicsObject");
}

wxClassInfo *wxGraphicsObject::GetClassInfo() const
{
	FAIL("wxGraphicsObject::GetClassInfo");
	return NULL;
}

wxObjectRefData* wxGraphicsObject::CreateRefData() const
{
	FAIL("wxGraphicsObject::CreateRefData");
	return NULL;
}

wxObjectRefData* wxGraphicsObject::CloneRefData(const wxObjectRefData* data) const
{
	FAIL("wxGraphicsObject::CloneRefData");
	return NULL;
}

void wxGraphicsMatrix::Concat( const wxGraphicsMatrix *t )
{
	FAIL("wxGraphicsMatrix::Concat");
}

void wxGraphicsMatrix::Set(wxDouble a, wxDouble b, wxDouble c, wxDouble d, wxDouble tx, wxDouble ty)
{
	FAIL("wxGraphicsMatrix::Set");
}

void wxGraphicsMatrix::Get(wxDouble* a, wxDouble* b,  wxDouble* c, wxDouble* d, wxDouble* tx, wxDouble* ty) const
{
	FAIL("wxGraphicsMatrix::Get");
}

void wxGraphicsMatrix::Invert()
{
	FAIL("wxGraphicsMatrix::Invert");
}

bool wxGraphicsMatrix::IsEqual( const wxGraphicsMatrix* t) const
{
	FAIL("wxGraphicsMatrix::IsEqual");
	return true;
}

bool wxGraphicsMatrix::IsIdentity() const
{
	FAIL("wxGraphicsMatrix::IsIdentity");
	return true;
}

void wxGraphicsMatrix::Translate( wxDouble dx , wxDouble dy )
{
	FAIL("wxGraphicsMatrix::Translate");
}

void wxGraphicsMatrix::Scale( wxDouble xScale , wxDouble yScale )
{
	FAIL("wxGraphicsMatrix::Scale");
}

void wxGraphicsMatrix::Rotate( wxDouble angle )
{
	FAIL("wxGraphicsMatrix::Rotate");
}

void wxGraphicsMatrix::TransformPoint( wxDouble *x, wxDouble *y ) const
{
	FAIL("wxGraphicsMatrix::TransformPoint");
}

void wxGraphicsMatrix::TransformDistance( wxDouble *dx, wxDouble *dy ) const
{
	FAIL("wxGraphicsMatrix::TransformDistance");
}

void * wxGraphicsMatrix::GetNativeMatrix() const
{
	FAIL("wxGraphicsMatrix::GetNativeMatrix");
	return NULL;
}

wxClassInfo *wxGraphicsMatrix::GetClassInfo() const
{
	FAIL("wxGraphicsMatrix::GetClassInfo");
	return NULL;
}



