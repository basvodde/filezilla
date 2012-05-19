
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/filedlg.h"

wxClassInfo *wxFileDialogBase::GetClassInfo() const
{
	FAIL("wxFileDialogBase::GetClassInfo");
	return NULL;
}

wxClassInfo *wxFileDialog::GetClassInfo() const
{
	FAIL("wxFileDialog::GetClassInfo");
	return NULL;
}

int wxFileDialog::ShowModal()
{
	FAIL("wxFileDialog::ShowModal");
	return 0;
}

