
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_bmpbt.h"

wxBitmapButtonXmlHandler::wxBitmapButtonXmlHandler()
{
	FAIL("wxBitmapButtonXmlHandler::wxBitmapButtonXmlHandler");
}

wxClassInfo *wxBitmapButtonXmlHandler::GetClassInfo() const
{
	FAIL("wxBitmapButtonXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxBitmapButtonXmlHandler::DoCreateResource()
{
	FAIL("wxBitmapButtonXmlHandler::DoCreateResource");
	return NULL;
}

bool wxBitmapButtonXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxBitmapButtonXmlHandler::CanHandle");
	return true;
}
