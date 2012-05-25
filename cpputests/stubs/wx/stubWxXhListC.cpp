
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_listc.h"

wxListCtrlXmlHandler::wxListCtrlXmlHandler()
{
	FAIL("wxListCtrlXmlHandler::wxListCtrlXmlHandler");
}

wxClassInfo *wxListCtrlXmlHandler::GetClassInfo() const
{
	FAIL("wxListCtrlXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxListCtrlXmlHandler::DoCreateResource()
{
	FAIL("wxListCtrlXmlHandler::DoCreateResource");
	return NULL;
}

bool wxListCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxListCtrlXmlHandler::CanHandle");
	return true;
}
