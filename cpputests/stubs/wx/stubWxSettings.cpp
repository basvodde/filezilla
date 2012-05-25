
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/colour.h"
#include "wx/font.h"
#include "wx/settings.h"

wxColour wxSystemSettingsNative::GetColour(wxSystemColour index)
{
	FAIL("wxSystemSettings::GetColour");
	return wxColour();
}

wxFont wxSystemSettingsNative::GetFont(wxSystemFont index)
{
	FAIL("wxSystemSettings::GetFont");
	return wxFont();
}

int wxSystemSettingsNative::GetMetric(wxSystemMetric index, wxWindow * win)
{
	FAIL("wxSystemSettings::GetMetric");
	return 0;
}
