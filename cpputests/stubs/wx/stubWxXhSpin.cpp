
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_spin.h"

wxSpinCtrlXmlHandler::wxSpinCtrlXmlHandler()
{
	FAIL("wxSpinCtrlXmlHandler::wxSpinCtrlXmlHandler");
}

wxClassInfo *wxSpinCtrlXmlHandler::GetClassInfo() const
{
	FAIL("wxSpinCtrlXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxSpinCtrlXmlHandler::DoCreateResource()
{
	FAIL("wxSpinCtrlXmlHandler::DoCreateResource");
	return NULL;
}

bool wxSpinCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxSpinCtrlXmlHandler::CanHandle");
	return true;
}
