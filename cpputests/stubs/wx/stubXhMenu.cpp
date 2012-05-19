

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_menu.h"

wxClassInfo *wxMenuBarXmlHandler::GetClassInfo() const
{
	FAIL("wxMenuBarXmlHandler::GetClassInfo");
	return NULL;
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
