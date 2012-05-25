#include <CppUTest/TestHarness.h>
#include "wx/msgdlg.h"


const wxChar wxMessageBoxCaptionStr[] = L"STUB message caption";

int wxMessageBox(const wxString& message, const wxString& caption, long style,
		wxWindow *parent, int x, int y)
{
	FAIL("MessageBox");
	return 0;
}

wxClassInfo *wxMessageDialog::GetClassInfo() const
{
	FAIL("wxMessageDialog::GetClassInfo");
	return NULL;
}
int wxMessageDialog::ShowModal()
{
	FAIL("wxMessageDialog::ShowModal");
	return 0;
}

wxMessageDialog::wxMessageDialog(wxWindow *parent, const wxString& message, const wxString& caption,
		long style, const wxPoint& pos)
{
	FAIL("wxMessageDialog::wxMessageDialog");
}

