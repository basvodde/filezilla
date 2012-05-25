
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/filedlg.h"

const wxChar wxFileDialogNameStr[100] = L"";

const wxChar wxFileSelectorDefaultWildcardStr[100] = L"";

wxClassInfo *wxFileDialogBase::GetClassInfo() const
{
	FAIL("wxFileDialogBase::GetClassInfo");
	return NULL;
}

void wxFileDialogBase::Init()
{
	FAIL("wxFileDialogBase::Init");
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

wxFileDialog::wxFileDialog(wxWindow *parent, const wxString& message, const wxString& defaultDir,
		const wxString& defaultFile, const wxString& wildCard, long style, const wxPoint& pos,
		const wxSize& sz, const wxString& name)
{
	FAIL("wxFileDialog::wxFileDialog");
}
