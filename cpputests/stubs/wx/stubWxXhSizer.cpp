

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_sizer.h"

wxSizerXmlHandler::wxSizerXmlHandler()
{
	FAIL("wxSizerXmlHandler::wxSizerXmlHandler");
}

wxObject *wxSizerXmlHandler::DoCreateResource()
{
	FAIL("wxSizerXmlHandler::DoCreateResource");
	return NULL;
}

bool wxSizerXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxSizerXmlHandler::CanHandle");
	return true;
}

wxClassInfo *wxSizerXmlHandler::GetClassInfo() const
{
	FAIL("wxSizerXmlHandler::GetClassInfo");
	return NULL;
}
