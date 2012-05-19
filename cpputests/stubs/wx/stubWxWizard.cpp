
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/wizard.h"

void wxWizard::DoCreateControls()
{
	FAIL("wxWizard::DoCreateControls");
}
