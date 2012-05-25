
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_chckl.h"

wxCheckListBoxXmlHandler::wxCheckListBoxXmlHandler()
{
	FAIL("wxCheckListBoxXmlHandler::wxCheckListBoxXmlHandler");
}

wxClassInfo *wxCheckListBoxXmlHandler::GetClassInfo() const
{
	FAIL("wxCheckListBoxXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxCheckListBoxXmlHandler::DoCreateResource()
{
	FAIL("wxCheckListBoxXmlHandler::DoCreateResource");
	return NULL;
}

bool wxCheckListBoxXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxCheckListBoxXmlHandler::CanHandle");
	return true;
}
