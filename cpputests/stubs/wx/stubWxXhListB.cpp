
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_listb.h"

wxListBoxXmlHandler::wxListBoxXmlHandler()
{
	FAIL("wxListBoxXmlHandler::wxListBoxXmlHandler");
}

wxClassInfo *wxListBoxXmlHandler::GetClassInfo() const
{
	FAIL("wxListBoxXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxListBoxXmlHandler::DoCreateResource()
{
	FAIL("wxListBoxXmlHandler::DoCreateResource");
	return NULL;
}

bool wxListBoxXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxListBoxXmlHandler::CanHandle");
	return true;
}
