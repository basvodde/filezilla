
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_text.h"

IMPLEMENT_DYNAMIC_CLASS(wxTextCtrlXmlHandler, wxXmlResourceHandler);

wxTextCtrlXmlHandler::wxTextCtrlXmlHandler()
{
	FAIL("wxTextCtrlXmlHandler::wxTextCtrlXmlHandler");
}

wxObject *wxTextCtrlXmlHandler::DoCreateResource()
{
	FAIL("wxTextCtrlXmlHandler::DoCreateResource");
	return NULL;
}

bool wxTextCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxTextCtrlXmlHandler::CanHandle");
	return true;
}
