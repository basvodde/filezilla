#include <CppUTest/TestHarness.h>
#include "wx/msgdlg.h"


const wxChar wxMessageBoxCaptionStr[] = L"STUB message caption";

int wxMessageBox(const wxString& message, const wxString& caption, long style,
		wxWindow *parent, int x, int y)
{
	FAIL("MessageBox");
	return 0;
}

