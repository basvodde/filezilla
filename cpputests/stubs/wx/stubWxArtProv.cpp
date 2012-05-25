
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

bool wxArtProvider::Remove(wxArtProvider *provider)
{
	FAIL("wxArtProvider::Remove");
	return true;
}

wxBitmap wxArtProvider::GetBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& size)
{
	FAIL("wxArtProvider::GetBitmap");
	return wxBitmap();
}

void wxArtProvider::Push(wxArtProvider *provider)
{
	FAIL("wxArtProvider::Push");
}
