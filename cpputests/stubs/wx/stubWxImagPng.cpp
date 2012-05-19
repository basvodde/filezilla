
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/imagpng.h"

wxClassInfo *wxPNGHandler::GetClassInfo() const
{
	FAIL("wxPNGHandler::GetClassInfo");
	return NULL;
}

bool wxPNGHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose, int index)
{
	FAIL("wxPNGHandler::LoadFile");
	return true;
}

bool wxPNGHandler::SaveFile( wxImage *image, wxOutputStream& stream, bool verbose)
{
	FAIL("wxPNGHandler::SaveFile");
	return true;
}

bool wxPNGHandler::DoCanRead( wxInputStream& stream )
{
	FAIL("wxPNGHandler::DoCanRead");
	return true;
}
