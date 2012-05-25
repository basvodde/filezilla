


#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_tree.h"

wxTreeCtrlXmlHandler::wxTreeCtrlXmlHandler()
{
	FAIL("wxTreeCtrlXmlHandler::wxTreeCtrlXmlHandler");
}

wxClassInfo *wxTreeCtrlXmlHandler::GetClassInfo() const
{
	FAIL("wxTreeCtrlXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxTreeCtrlXmlHandler::DoCreateResource()
{
	FAIL("wxTreeCtrlXmlHandler::DoCreateResource");
	return NULL;
}

bool wxTreeCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxTreeCtrlXmlHandler::CanHandle");
	return true;
}
