

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_stbmp.h"

wxStaticBitmapXmlHandler::wxStaticBitmapXmlHandler()
{
	FAIL("wxStaticBitmapXmlHandler::wxStaticBitmapXmlHandler");
}

wxClassInfo *wxStaticBitmapXmlHandler::GetClassInfo() const
{
	FAIL("wxStaticBitmapXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxStaticBitmapXmlHandler::DoCreateResource()
{
	FAIL("wxStaticBitmapXmlHandler::DoCreateResource");
	return NULL;
}

bool wxStaticBitmapXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxStaticBitmapXmlHandler::CanHandle");
	return true;
}
