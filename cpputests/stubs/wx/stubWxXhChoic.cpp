
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_choic.h"

wxChoiceXmlHandler::wxChoiceXmlHandler()
{
	FAIL("wxChoiceXmlHandler::wxChoiceXmlHandler");
}

wxClassInfo *wxChoiceXmlHandler::GetClassInfo() const
{
	FAIL("wxChoiceXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxChoiceXmlHandler::DoCreateResource()
{
	FAIL("wxChoiceXmlHandler::DoCreateResource");
	return NULL;
}

bool wxChoiceXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxChoiceXmlHandler::CanHandle");
	return true;
}
