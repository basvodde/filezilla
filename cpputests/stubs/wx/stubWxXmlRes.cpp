
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xmlres.h"

wxXmlResourceHandler::wxXmlResourceHandler()
{
	FAIL("wxXmlResourceHandler::wxXmlResourceHandler");
}

wxClassInfo *wxXmlResourceHandler::GetClassInfo() const
{
	FAIL("wxXmlResourceHandler::GetClassInfo");
	return NULL;
}
