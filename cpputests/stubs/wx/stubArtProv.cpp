
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/artprov.h"

wxArtProvider::~wxArtProvider()
{
	FAIL("wxArtProvider::~wxArtProvider");
}

wxSize wxArtProvider::GetSizeHint(const wxArtClient& client, bool platform_dependent)
{
	FAIL("wxArtProvider::GetSizeHint");
	return wxSize();
}

wxClassInfo *wxArtProvider::GetClassInfo() const
{
	FAIL("wxArtProvider::GetClassInfo");
	return NULL;
}
