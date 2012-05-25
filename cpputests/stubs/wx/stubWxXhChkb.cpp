
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_chckb.h"

wxCheckBoxXmlHandler::wxCheckBoxXmlHandler()
{
	FAIL("wxCheckBoxXmlHandler::wxCheckBoxXmlHandler");
}

wxClassInfo *wxCheckBoxXmlHandler::GetClassInfo() const
{
	FAIL("wxCheckBoxXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxCheckBoxXmlHandler::DoCreateResource()
{
	FAIL("wxCheckBoxXmlHandler::DoCreateResource");
	return NULL;
}

bool wxCheckBoxXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxCheckBoxXmlHandler::CanHandle");
	return true;
}
