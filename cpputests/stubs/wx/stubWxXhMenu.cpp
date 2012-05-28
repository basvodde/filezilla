

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_menu.h"

wxMenuXmlHandler::wxMenuXmlHandler()
{
	FAIL("wxMenuBarXmlHandler::wxMenuXmlHandler");
}

wxClassInfo *wxMenuXmlHandler::GetClassInfo() const
{
	FAIL("wxMenuXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxMenuXmlHandler::DoCreateResource()
{
	FAIL("wxMenuXmlHandler::DoCreateResource");
	return NULL;
}

bool wxMenuXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxMenuXmlHandler::CanHandle");
	return true;
}

IMPLEMENT_DYNAMIC_CLASS(wxMenuBarXmlHandler, wxXmlResourceHandler);

wxMenuBarXmlHandler::wxMenuBarXmlHandler()
{
	FAIL("wxMenuBarXmlHandler::wxMenuBarXmlHandler");
}

wxObject *wxMenuBarXmlHandler::DoCreateResource()
{
	FAIL("wxMenuBarXmlHandler::DoCreateResource");
	return NULL;
}

bool wxMenuBarXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxMenuBarXmlHandler::CanHandle");
	return true;
}
