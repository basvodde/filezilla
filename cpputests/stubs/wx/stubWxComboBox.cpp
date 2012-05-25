
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/combobox.h"

const wxChar wxComboBoxNameStr[100] = L"";

const wxEventTable wxComboBox::sm_eventTable = wxEventTable();
wxEventHashTable wxComboBox::sm_eventHashTable (wxComboBox::sm_eventTable);

void wxComboBox::SelectAll()
{
	FAIL("wxComboBox::SelectAll");
}

void wxComboBox::DoSetItemClientObject(unsigned int n, wxClientData* clientData)
{
	FAIL("wxComboBox::DoSetItemClientObject");
}

void wxComboBox::Remove(long from, long to)
{
	FAIL("wxComboBox::Remove");
}

void wxComboBox::Redo()
{
	FAIL("wxComboBox::Redo");
}

unsigned int wxComboBox::GetCount() const
{
	FAIL("wxComboBox::GetCount");
	return 0;
}

void wxComboBox::Replace(long from, long to, const wxString& value)
{
	FAIL("wxComboBox::Replace");
}

void wxComboBox::SetInsertionPoint(long pos)
{
	FAIL("wxComboBox::SetInsertionPoint");
}

wxClientData * wxComboBox::DoGetItemClientObject(unsigned int n) const
{
	FAIL("wxComboBox::DoGetItemClientObject");
	return NULL;
}

void wxComboBox::DelegateChoice( const wxString& value )
{
	FAIL("wxComboBox::DelegateChoice");
}

wxSize wxComboBox::DoGetBestSize() const
{
	FAIL("wxComboBox::DoGetBestSize");
	return wxSize();
}

void wxComboBox::SetValue(const wxString& value)
{
	FAIL("wxComboBox::SetValue");
}

void wxComboBox::Cut()
{
	FAIL("wxComboBox::Cut");
}

wxTextPos wxComboBox::GetLastPosition() const
{
	FAIL("wxComboBox::GetLastPosition");
	return 0;
}

bool wxComboBox::CanUndo() const
{
	FAIL("wxComboBox::CanUndo");
	return true;
}

wxString wxComboBox::GetStringSelection() const
{
	FAIL("wxComboBox::GetStringSelection");
	return L"";
}

void wxComboBox::Clear()
{
	FAIL("wxComboBox::Clear");
}

int wxComboBox::DoAppend(const wxString& item)
{
	FAIL("wxComboBox::DoAppend");
	return 0;
}

int wxComboBox::GetSelection() const
{
	FAIL("wxComboBox::GetSelection");
	return 0;
}

bool wxComboBox::Create(wxWindow *parent, wxWindowID id, const wxString& value, const wxPoint& pos,
		const wxSize& size, int n, const wxString choices[], long style,
		const wxValidator& validator, const wxString& name)
{
	FAIL("wxComboBox::Create");
	return true;
}

bool wxComboBox::CanCut() const
{
	FAIL("wxComboBox::CanCut");
	return true;
}

wxString wxComboBox::GetValue() const
{
	FAIL("wxComboBox::GetValue");
	return L"";
}

void wxComboBox::Undo()
{
	FAIL("wxComboBox::Undo");
}

void wxComboBox::Copy()
{
	FAIL("wxComboBox::Copy");
}

void wxComboBox::SetInsertionPointEnd()
{
	FAIL("wxComboBox::SetInsertionPointEnd");
}

void wxComboBox::SetSelection(long from, long to)
{
	FAIL("wxComboBox::SetSelection");
}

void wxComboBox::Paste()
{
	FAIL("wxComboBox::Paste");
}

int wxComboBox::FindString(const wxString& s, bool bCase) const
{
	FAIL("wxComboBox::FindString");
	return 0;
}

wxString wxComboBox::GetString(unsigned int n) const
{
	FAIL("wxComboBox::GetString");
	return L"";
}

bool wxComboBox::CanPaste() const
{
	FAIL("wxComboBox::CanPaste");
	return true;
}

void wxComboBox::SetSelection(int n)
{
	FAIL("wxComboBox::SetSelection");
}

void wxComboBox::DoSetItemClientData(unsigned int n, void* clientData)
{
	FAIL("wxComboBox::DoSetItemClientData");
}

bool wxComboBox::CanCopy() const
{
	FAIL("wxComboBox::CanCopy");
	return true;
}

wxComboBox::~wxComboBox()
{
	FAIL("wxComboBox::~wxComboBox");
}

wxClassInfo *wxComboBox::GetClassInfo() const
{
	FAIL("wxComboBox::GetClassInfo");
	return NULL;
}

void * wxComboBox::DoGetItemClientData(unsigned int n) const
{
	FAIL("wxComboBox::DoGetItemClientData");
	return NULL;
}

void wxComboBox::DelegateTextChanged( const wxString& value )
{
	FAIL("wxComboBox::DelegateTextChanged");
}

void wxComboBox::SetEditable(bool editable)
{
	FAIL("wxComboBox::SetEditable");
}

void wxComboBox::SetString(unsigned int n, const wxString& s)
{
	FAIL("wxComboBox::SetString");
}

void wxComboBox::OnChildFocus(wxChildFocusEvent& event)
{
	FAIL("wxComboBox::OnChildFocus");
}

void wxComboBox::SetFocusIgnoringChildren()
{
	FAIL("wxComboBox::SetFocusIgnoringChildren");
}

void wxComboBox::SetFocus()
{
	FAIL("wxComboBox::SetFocus");
}

void wxComboBox::RemoveChild(wxWindowBase *child)
{
	FAIL("wxComboBox::RemoveChild");
}

bool wxComboBox::AcceptsFocus() const
{
	FAIL("wxComboBox::AcceptsFocus");
	return true;
}

const wxEventTable* wxComboBox::GetEventTable() const
{
	FAIL("wxComboBox::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxComboBox::GetEventHashTable() const
{
	FAIL("wxComboBox::GetEventHashTable");
	return sm_eventHashTable;
}

long wxComboBox::GetInsertionPoint() const
{
	FAIL("wxComboBox::GetInsertionPoint");
	return 0;
}

bool wxComboBox::IsEditable() const
{
	FAIL("wxComboBox::IsEditable");
	return true;
}

int wxComboBox::DoInsert(const wxString& item, unsigned int pos)
{
	FAIL("wxComboBox::DoInsert");
	return 0;
}

void wxComboBox::DoMoveWindow(int x, int y, int width, int height)
{
	FAIL("wxComboBox::DoMoveWindow");
}

void wxComboBox::Delete(unsigned int n)
{
	FAIL("wxComboBox::Delete");
}

wxInt32 wxComboBox::MacControlHit( WXEVENTHANDLERREF handler, WXEVENTREF event )
{
	FAIL("wxComboBox::MacControlHit");
	return 0;
}

bool wxComboBox::Enable(bool enable)
{
	FAIL("wxComboBox::Enable");
	return true;
}

bool wxComboBox::CanRedo() const
{
	FAIL("wxComboBox::CanRedo");
	return true;
}

bool wxComboBox::Show(bool show)
{
	FAIL("wxComboBox::Show");
	return true;
}

bool wxComboBox::Create(wxWindow *parent, wxWindowID id, const wxString& value, const wxPoint& pos, const wxSize& size,
		const wxArrayString& choices, long style, const wxValidator& validator, const wxString& name)
{
	FAIL("wxComboBox::Create");
	return true;
}

void wxComboBox::Init()
{
	FAIL("wxComboBox::Init");
}

