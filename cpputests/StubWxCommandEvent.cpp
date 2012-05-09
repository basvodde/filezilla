
#include "CppUTest/TestHarness.h"
#include "wx/event.h"

wxCommandEvent::wxCommandEvent(wxEventType commandType, int winid)
{
}

wxClassInfo *wxCommandEvent::GetClassInfo() const
{
	FAIL("wxCommandEvent::GetCalssInfo");
	return NULL;
}
