

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_gauge.h"

wxGaugeXmlHandler::wxGaugeXmlHandler()
{
	FAIL("wxGaugeXmlHandler::wxGaugeXmlHandler");
}

wxClassInfo *wxGaugeXmlHandler::GetClassInfo() const
{
	FAIL("wxGaugeXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxGaugeXmlHandler::DoCreateResource()
{
	FAIL("wxGaugeXmlHandler::DoCreateResource");
	return NULL;
}

bool wxGaugeXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxGaugeXmlHandler::CanHandle");
	return true;
}
