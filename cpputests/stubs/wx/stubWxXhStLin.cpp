
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_stlin.h"

wxStaticLineXmlHandler::wxStaticLineXmlHandler()
{
	FAIL("wxStaticLineXmlHandler::wxStaticLineXmlHandler");
}

wxClassInfo *wxStaticLineXmlHandler::GetClassInfo() const
{
	FAIL("wxStaticLineXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxStaticLineXmlHandler::DoCreateResource()
{
	FAIL("wxStaticLineXmlHandler::DoCreateResource");
	return NULL;
}

bool wxStaticLineXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxStaticLineXmlHandler::CanHandle");
	return true;
}
