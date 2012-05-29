
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "filezilla.h"
#include "filezillaapp.h"
#include "aboutdialog.h"

TEST_GROUP(AboutDialog)
{
};

TEST(AboutDialog, fail)
{
	mock().expectOneCall("wxIconBundle::DeleteIcons");
	CFileZillaApp app;
	CFileZillaApp::SetInstance(&app);
	CAboutDialog dialog;
}
