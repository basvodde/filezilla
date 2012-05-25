

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xh_notbk.h"

wxNotebookXmlHandler::wxNotebookXmlHandler()
{
	FAIL("wxNotebookXmlHandler::wxNotebookXmlHandler");
}

wxClassInfo *wxNotebookXmlHandler::GetClassInfo() const
{
	FAIL("wxNotebookXmlHandler::GetClassInfo");
	return NULL;
}

wxObject *wxNotebookXmlHandler::DoCreateResource()
{
	FAIL("wxNotebookXmlHandler::DoCreateResource");
	return NULL;
}

bool wxNotebookXmlHandler::CanHandle(wxXmlNode *node)
{
	FAIL("wxNotebookXmlHandler::CanHandle");
	return true;
}
