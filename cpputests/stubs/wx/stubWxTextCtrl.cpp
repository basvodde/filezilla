

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/textctrl.h"

void wxTextAttr::Init()
{
	FAIL("wxTextAttr::Init");
}

wxString wxTextCtrlBase::GetRange(long from, long to) const
{
	FAIL("wxTextCtrlBase::GetRange");
	return L"";
}

wxTextCtrlHitTestResult wxTextCtrlBase::HitTest(const wxPoint& pt, long *pos) const
{
	FAIL("wxTextCtrlBase::HitTest");
	return wxTextCtrlHitTestResult();
}

wxTextCtrlHitTestResult wxTextCtrlBase::HitTest(const wxPoint& pt,  wxTextCoord *col, wxTextCoord *row) const
{
	FAIL("wxTextCtrlBase::HitTest");
	return wxTextCtrlHitTestResult();
}

bool wxTextCtrlBase::CanCopy() const
{
	FAIL("wxTextCtrlBase::CanCopy");
	return true;
}

bool wxTextCtrlBase::CanCut() const
{
	FAIL("wxTextCtrlBase::CanCut");
	return true;
}

bool wxTextCtrlBase::CanPaste() const
{
	FAIL("wxTextCtrlBase::CanPaste");
	return true;
}

bool wxTextCtrlBase::EmulateKeyPress(const wxKeyEvent& event)
{
	FAIL("wxTextCtrlBase::EmulateKeyPress");
	return true;
}

bool wxTextCtrlBase::SetStyle(long start, long end, const wxTextAttr& style)
{
	FAIL("wxTextCtrlBase::SetStyle");
	return true;
}

bool wxTextCtrlBase::GetStyle(long position, wxTextAttr& style)
{
	FAIL("wxTextCtrlBase::GetStyle");
	return true;
}

bool wxTextCtrlBase::SetDefaultStyle(const wxTextAttr& style)
{
	FAIL("wxTextCtrlBase::SetDefaultStyle");
	return true;
}

static wxTextAttr gAttr;
const wxTextAttr& wxTextCtrlBase::GetDefaultStyle() const
{
	FAIL("wxTextCtrlBase::GetDefaultStyle");
	return gAttr;
}

wxString wxTextCtrlBase::GetStringSelection() const
{
	FAIL("wxTextCtrlBase::GetStringSelection");
	return L"";
}

void wxTextCtrlBase::SelectAll()
{
	FAIL("wxTextCtrlBase::SelectAll");
}

bool wxTextCtrlBase::DoLoadFile(const wxString& file, int fileType)
{
	FAIL("wxTextCtrlBase::DoLoadFile");
	return true;
}

bool wxTextCtrlBase::DoSaveFile(const wxString& file, int fileType)
{
	FAIL("wxTextCtrlBase::DoSaveFile");
	return true;
}

void wxTextCtrlBase::DoUpdateWindowUI(wxUpdateUIEvent& event)
{
	FAIL("wxTextCtrlBase::DoUpdateWindowUI");
}

wxClassInfo *wxTextCtrlBase::GetClassInfo() const
{
	FAIL("wxTextCtrlBase::GetClassInfo");
	return NULL;
}

const wxEventTable wxTextCtrl::sm_eventTable = wxEventTable();
wxEventHashTable wxTextCtrl::sm_eventHashTable (wxTextCtrl::sm_eventTable);

const wxEventTable* wxTextCtrl::GetEventTable() const
{
	FAIL("wxTextCtrl::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxTextCtrl::GetEventHashTable() const
{
	FAIL("wxTextCtrl::GetEventHashTable");
	return sm_eventHashTable;
}

void wxTextCtrl::DoSetValue(const wxString& value, int flags)
{
	FAIL("wxTextCtrl::DoSetValue");
}

wxTextPos wxTextCtrl::GetLastPosition() const
{
	FAIL("wxTextCtrl::GetLastPosition");
	return 1;
}

bool wxTextCtrl::MacSetupCursor( const wxPoint& pt )
{
	FAIL("wxTextCtrl::MacSetupCursor");
	return true;
}

bool wxTextCtrl::PositionToXY(long pos, long *x, long *y) const
{
	FAIL("wxTextCtrl::PositionToXY");
	return true;
}

int wxTextCtrl::GetLineLength(long lineNo) const
{
	FAIL("wxTextCtrl::GetLineLength");
	return 1;
}

bool wxTextCtrl::CanRedo() const
{
	FAIL("wxTextCtrl::CanRedo");
	return true;
}

void wxTextCtrl::SetMaxLength(unsigned long len)
{
	FAIL("wxTextCtrl::SetMaxLength");
}

bool wxTextCtrl::AcceptsFocus() const
{
	FAIL("wxTextCtrl::AcceptsFocus");
	return true;
}

void wxTextCtrl::MacVisibilityChanged()
{
	FAIL("wxTextCtrl::MacVisibilityChanged");
}

wxString wxTextCtrl::GetLineText(long lineNo) const
{
	FAIL("wxTextCtrl::GetLineText");
	return L"";
}

void wxTextCtrl::SetInsertionPoint(long pos)
{
	FAIL("wxTextCtrl::SetInsertionPoint");
}

void wxTextCtrl::Remove(long from, long to)
{
	FAIL("wxTextCtrl::Remove");
}

wxClassInfo *wxTextCtrl::GetClassInfo() const
{
	FAIL("wxTextCtrl::GetClassInfo");
	return NULL;
}

wxString wxTextCtrl::GetValue() const
{
	FAIL("wxTextCtrl::GetValue");
	return L"";
}

int wxTextCtrl::GetNumberOfLines() const
{
	FAIL("wxTextCtrl::GetNumberOfLines");
	return 0;
}

bool wxTextCtrl::IsModified() const
{
	FAIL("wxTextCtrl::IsModified");
	return true;
}

bool wxTextCtrl::IsEditable() const
{
	FAIL("wxTextCtrl::IsEditable");
	return true;
}

void wxTextCtrl::GetSelection(long* from, long* to) const
{
	FAIL("wxTextCtrl::GetSelection");
}

void wxTextCtrl::Clear()
{
	FAIL("wxTextCtrl::Clear");
}

void wxTextCtrl::Replace(long from, long to, const wxString& value)
{
	FAIL("wxTextCtrl::Clear");
}

void wxTextCtrl::MarkDirty()
{
	FAIL("wxTextCtrl::MarkDirty");
}

void wxTextCtrl::DiscardEdits()
{
	FAIL("wxTextCtrl::DiscardEdits");
}

bool wxTextCtrl::SetFont( const wxFont &font )
{
	FAIL("wxTextCtrl::SetFont");
	return true;
}

bool wxTextCtrl::SetStyle(long start, long end, const wxTextAttr& style)
{
	FAIL("wxTextCtrl::SetStyle");
	return true;
}

bool wxTextCtrl::SetDefaultStyle(const wxTextAttr& style)
{
	FAIL("wxTextCtrl::SetDefaultStyle");
	return true;
}

void wxTextCtrl::WriteText(const wxString& text)
{
	FAIL("wxTextCtrl::WriteText");
}

void wxTextCtrl::AppendText(const wxString& text)
{
	FAIL("wxTextCtrl::AppendText");
}

long wxTextCtrl::XYToPosition(long x, long y) const
{
	FAIL("wxTextCtrl::XYToPosition");
	return 0;
}

void wxTextCtrl::ShowPosition(long pos)
{
	FAIL("wxTextCtrl::ShowPosition");
}

void wxTextCtrl::Copy()
{
	FAIL("wxTextCtrl::Copy");
}

void wxTextCtrl::Cut()
{
	FAIL("wxTextCtrl::Cut");
}

void wxTextCtrl::Paste()
{
	FAIL("wxTextCtrl::Paste");
}

bool wxTextCtrl::CanCopy() const
{
	FAIL("wxTextCtrl::CanCopy");
	return true;
}

bool wxTextCtrl::CanCut() const
{
	FAIL("wxTextCtrl::CanCut");
	return true;
}

bool wxTextCtrl::CanPaste() const
{
	FAIL("wxTextCtrl::CanPaste");
	return true;
}

void wxTextCtrl::Undo()
{
	FAIL("wxTextCtrl::Undo");
}

void wxTextCtrl::Redo()
{
	FAIL("wxTextCtrl::Redo");
}

bool wxTextCtrl::CanUndo() const
{
	FAIL("wxTextCtrl::CanUndo");
	return true;
}

void wxTextCtrl::SetInsertionPointEnd()
{
	FAIL("wxTextCtrl::SetInsertionPointEnd");
}

long wxTextCtrl::GetInsertionPoint() const
{
	FAIL("wxTextCtrl::GetInsertionPoint");
	return 0;
}

void wxTextCtrl::SetSelection(long from, long to)
{
	FAIL("wxTextCtrl::SetSelection");
}

void wxTextCtrl::SetEditable(bool editable)
{
	FAIL("wxTextCtrl::SetEditable");
}

void wxTextCtrl::Command(wxCommandEvent& event)
{
	FAIL("wxTextCtrl::Command");
}

void wxTextCtrl::MacEnabledStateChanged()
{
	FAIL("wxTextCtrl::MacEnabledStateChanged");
}

void wxTextCtrl::MacSuperChangedPosition()
{
	FAIL("wxTextCtrl::MacSuperChangedPosition");
}

void wxTextCtrl::MacCheckSpelling(bool check)
{
	FAIL("wxTextCtrl::MacCheckSpelling");
}

wxTextCtrl::~wxTextCtrl()
{
	FAIL("wxTextCtrl::~wxTextCtrl");
}

wxSize wxTextCtrl::DoGetBestSize() const
{
	FAIL("wxTextCtrl::DoGetBestSize");
	return wxSize();
}

void wxTextCtrl::CreatePeer(const wxString& str, const wxPoint& pos, const wxSize& size, long style )
{
	FAIL("wxTextCtrl::CreatePeer");
}


