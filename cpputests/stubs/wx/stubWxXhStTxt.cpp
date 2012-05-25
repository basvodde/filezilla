
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_sttxt.h"

wxStaticTextXmlHandler::wxStaticTextXmlHandler()
{
	FAIL("wxStaticTextXmlHandler::wxStaticTextXmlHandler");
}

wxClassInfo *wxStaticTextXmlHandler::GetClassInfo() const
{
	FAIL("wxStaticTextXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxStaticTextXmlHandler::DoCreateResource()
{
	FAIL("wxStaticTextXmlHandler::DoCreateResource");
	return NULL;
}

bool wxStaticTextXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxStaticTextXmlHandler::CanHandle");
	return true;
}
