
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/textdlg.h"

const wxEventTable wxTextEntryDialog::sm_eventTable = wxEventTable();
wxEventHashTable wxTextEntryDialog::sm_eventHashTable (wxTextEntryDialog::sm_eventTable);

wxTextEntryDialog::wxTextEntryDialog(wxWindow *parent, const wxString& message, const wxString& caption,
		const wxString& value, long style, const wxPoint& pos)
{
	FAIL("wxTextEntryDialog::wxTextEntryDialog");
}

const wxEventTable* wxTextEntryDialog::GetEventTable() const
{
	FAIL("wxTextEntryDialog::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxTextEntryDialog::GetEventHashTable() const
{
	FAIL("wxTextEntryDialog::GetEventHashTable");
	return sm_eventHashTable;
}

wxClassInfo *wxTextEntryDialog::GetClassInfo() const
{
	FAIL("wxTextEntryDialog::GetClassInfo");
	return NULL;
}
