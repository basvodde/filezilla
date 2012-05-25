

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/defs.h"
#include "wx/dirdlg.h"

const wxChar wxDirDialogNameStr[100] = L"";

int wxDirDialog::ShowModal()
{
	FAIL("wxDirDialog::ShowModal");
	return 0;
}

wxDirDialog::wxDirDialog(wxWindow *parent, const wxString& message, const wxString& defaultPath,
		long style, const wxPoint& pos, const wxSize& size, const wxString& name)
{
	FAIL("wxDirDialog::wxDirDialog");
}

wxClassInfo *wxDirDialog::GetClassInfo() const
{
	FAIL("wxDirDialog::GetClassInfo");
	return NULL;
}
