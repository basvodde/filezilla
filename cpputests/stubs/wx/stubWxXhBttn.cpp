
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_bttn.h"

wxButtonXmlHandler::wxButtonXmlHandler()
{
	FAIL("wxButtonXmlHandler::wxButtonXmlHandler");
}

wxClassInfo *wxButtonXmlHandler::GetClassInfo() const
{
	FAIL("wxButtonXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxButtonXmlHandler::DoCreateResource()
{
	FAIL("wxButtonXmlHandler::DoCreateResource");
	return NULL;
}

bool wxButtonXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxButtonXmlHandler::CanHandle");
	return true;
}
