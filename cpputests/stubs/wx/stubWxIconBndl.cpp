

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/iconbndl.h"
#include "wx/icon.h"

wxIconArray::~wxIconArray()
{
}

static wxIcon gIcon;
const wxIcon& wxIconBundle::GetIcon( const wxSize& size ) const
{
	FAIL("wxIconBundle::GetIcon");
	return gIcon;
}

const wxIconBundle& wxIconBundle::operator =( const wxIconBundle& ic )
{
	FAIL("wxIconBundle::operator=()");
	return *this;
}

void wxIconBundle::AddIcon( const wxString& file, long type )
{
	FAIL("wxIconBundle::AddIcon()");
}

void wxIconBundle::DeleteIcons()
{
	mock().actualCall("wxIconBundle::DeleteIcons");
}

void wxIconBundle::AddIcon( const wxIcon& icon )
{
	FAIL("wxIconBundle::AddIcon()");
}
