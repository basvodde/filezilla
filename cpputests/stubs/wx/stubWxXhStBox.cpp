
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_stbox.h"

wxStaticBoxXmlHandler::wxStaticBoxXmlHandler()
{
	FAIL("wxStaticBoxXmlHandler::wxStaticBoxXmlHandler");
}

wxClassInfo *wxStaticBoxXmlHandler::GetClassInfo() const
{
	FAIL("wxStaticBoxXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxStaticBoxXmlHandler::DoCreateResource()
{
	FAIL("wxStaticBoxXmlHandler::DoCreateResource");
	return NULL;
}

bool wxStaticBoxXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxStaticBoxXmlHandler::CanHandle");
	return true;
}
