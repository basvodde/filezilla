

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_dlg.h"

wxDialogXmlHandler::wxDialogXmlHandler()
{
	FAIL("wxDialogXmlHandler::wxDialogXmlHandler");
}

wxClassInfo *wxDialogXmlHandler::GetClassInfo() const
{
	FAIL("wxDialogXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxDialogXmlHandler::DoCreateResource()
{
	FAIL("wxDialogXmlHandler::DoCreateResource");
	return NULL;
}

bool wxDialogXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxDialogXmlHandler::CanHandle");
	return true;
}
