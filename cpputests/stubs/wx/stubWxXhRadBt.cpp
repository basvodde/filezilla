

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_radbt.h"

wxRadioButtonXmlHandler::wxRadioButtonXmlHandler()
{
	FAIL("wxRadioButtonXmlHandler::wxRadioButtonXmlHandler");
}

wxObject *wxRadioButtonXmlHandler::DoCreateResource()
{
	FAIL("wxRadioButtonXmlHandler::DoCreateResource");
	return NULL;
}

bool wxRadioButtonXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxRadioButtonXmlHandler::CanHandle");
	return true;
}

wxClassInfo *wxRadioButtonXmlHandler::GetClassInfo() const
{
	FAIL("wxRadioButtonXmlHandler::GetClassInfo");
	return NULL;
}
