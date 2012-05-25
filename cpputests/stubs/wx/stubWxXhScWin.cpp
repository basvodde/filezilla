
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_scwin.h"

wxScrolledWindowXmlHandler::wxScrolledWindowXmlHandler()
{
	FAIL("wxScrolledWindowXmlHandler::wxScrolledWindowXmlHandler");
}

wxClassInfo *wxScrolledWindowXmlHandler::GetClassInfo() const
{
	FAIL("wxScrolledWindowXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxScrolledWindowXmlHandler::DoCreateResource()
{
	FAIL("wxScrolledWindowXmlHandler::DoCreateResource");
	return NULL;
}

bool wxScrolledWindowXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxScrolledWindowXmlHandler::CanHandle");
	return true;
}
