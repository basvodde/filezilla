
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/dialog.h"

const wxChar wxDialogNameStr[] = L"";;

IMPLEMENT_DYNAMIC_CLASS(wxDialog, wxDialogBase);

const wxEventTable wxDialogBase::sm_eventTable = wxEventTable();
wxEventHashTable wxDialogBase::sm_eventHashTable (wxDialogBase::sm_eventTable);

const wxEventTable* wxDialogBase::GetEventTable() const
{
	FAIL("wxDialogBase::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxDialogBase::GetEventHashTable() const
{
	FAIL("wxDialogBase::GetEventHashTable");
	return sm_eventHashTable;
}

void wxDialogBase::OnChildFocus(wxChildFocusEvent& event)
{
	FAIL("wxDialogBase::OnChildFocus");
}

void wxDialogBase::SetFocusIgnoringChildren()
{
	FAIL("wxDialogBase::SetFocusIgnoringChildren");
}

void wxDialogBase::SetFocus()
{
	FAIL("wxDialogBase::SetFocus");
}

void wxDialogBase::RemoveChild(wxWindowBase *child)
{
	FAIL("wxDialogBase::RemoveChild");
}

bool wxDialogBase::AcceptsFocus() const
{
	FAIL("wxDialogBase::AcceptsFocus");
	return true;
}

bool wxDialogBase::IsEscapeKey(const wxKeyEvent& event)
{
	FAIL("wxDialogBase::IsEscapeKey");
	return true;
}

void wxDialogBase::Init()
{
}

void wxDialogBase::EndDialog(int rc)
{
	FAIL("wxDialogBase::EndDialog");
}

void wxDialogBase::SetEscapeId(int escapeId)
{
	FAIL("wxDialogBase::SetEscapeId");
}

bool wxDialog::Create(wxWindow *parent, wxWindowID id, const wxString& title, const wxPoint& pos,
		const wxSize& size, long style, const wxString& name)
{
	FAIL("wxDialog::Create");
	return true;
}


wxDialog::~wxDialog()
{
}

void wxDialog::EndModal(int retCode)
{
	FAIL("wxDialog::EndModal");
}

bool wxDialog::IsModal() const
{
	FAIL("wxDialog::IsModal");
	return true;
}

bool wxDialog::Show(bool show)
{
	FAIL("wxDialog::Show");
	return true;
}

int wxDialog::ShowModal()
{
	FAIL("wxDialog::ShowModal");
	return 1;
}

bool wxDialog::IsEscapeKey(const wxKeyEvent& event)
{
	FAIL("wxDialog::IsEscapeKey");
	return true;
}

void wxDialog::Init()
{
}

