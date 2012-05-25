

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_hyperlink.h"

wxHyperlinkCtrlXmlHandler::wxHyperlinkCtrlXmlHandler()
{
	FAIL("wxHyperlinkCtrlXmlHandler::wxHyperlinkCtrlXmlHandler");
}

wxClassInfo *wxHyperlinkCtrlXmlHandler::GetClassInfo() const
{
	FAIL("wxHyperlinkCtrlXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxHyperlinkCtrlXmlHandler::DoCreateResource()
{
	FAIL("wxHyperlinkCtrlXmlHandler::DoCreateResource");
	return NULL;
}

bool wxHyperlinkCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxHyperlinkCtrlXmlHandler::CanHandle");
	return true;
}
