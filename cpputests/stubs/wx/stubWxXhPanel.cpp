
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_panel.h"

wxPanelXmlHandler::wxPanelXmlHandler()
{
	FAIL("wxPanelXmlHandler::wxPanelXmlHandler");
}

wxClassInfo *wxPanelXmlHandler::GetClassInfo() const
{
	FAIL("wxPanelXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxPanelXmlHandler::DoCreateResource()
{
	FAIL("wxPanelXmlHandler::DoCreateResource");
	return NULL;
}

bool wxPanelXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxPanelXmlHandler::CanHandle");
	return true;
}

