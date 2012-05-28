
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/window.h"

IMPLEMENT_ABSTRACT_CLASS(wxWindowBase, wxEvtHandler);
IMPLEMENT_DYNAMIC_CLASS(wxWindow, wxWindowBase);

wxWindowList wxTopLevelWindows;

wxWindow* wxGetTopLevelParent(wxWindow *win)
{
	FAIL("wxGetTopLevelParent");
	return NULL;
}

void wxWindowListNode::DeleteData()
{
	FAIL("wxWindowListNode::DeleteData");
}

int wxWindowBase::ms_lastControlId = 0;

const wxEventTable wxWindowBase::sm_eventTable = wxEventTable();
wxEventHashTable wxWindowBase::sm_eventHashTable (wxWindowBase::sm_eventTable);

bool wxWindowBase::DoIsExposed( int x, int y, int w, int h ) const
{
	FAIL("wxWindowBase::DoIsExposed");
	return true;
}

bool wxWindowBase::DoIsExposed( int x, int y ) const
{
	FAIL("wxWindowBase::DoIsExposed");
	return true;
}

void wxWindowBase::SetVirtualSizeHints( int minW, int minH, int maxW, int maxH)
{
	FAIL("wxWindowBase::SetVirtualSizeHints");
}

void wxWindowBase::DoGetScreenPosition(int *x, int *y) const
{
	FAIL("wxWindowBase::DoGetScreenPosition");
}

void wxWindowBase::SetConstraintSizes(bool recurse)
{
	FAIL("wxWindowBase::SetConstraintSizes");
}

bool wxWindowBase::TransferDataFromWindow()
{
	FAIL("wxWindowBase::SetConstraintSizes");
	return true;
}

void wxWindowBase::GetClientSizeConstraint(int *w, int *h) const
{
	FAIL("wxWindowBase::GetClientSizeConstraint");
}

wxString wxWindowBase::GetHelpTextAtPoint(const wxPoint& pt, wxHelpEvent::Origin origin) const
{
	FAIL("wxWindowBase::GetHelpTextAtPoint");
	return L"";
}

wxCoord wxWindowBase::AdjustForLayoutDirection(wxCoord x, wxCoord width, wxCoord widthTotal) const
{
	FAIL("wxWindowBase::AdjustForLayoutDirection");
	return wxCoord();
}

void wxWindowBase::FitInside()
{
	FAIL("wxWindowBase::FitInside");
}

bool wxWindowBase::Destroy()
{
	FAIL("wxWindowBase::Destroy");
	return true;
}

void wxWindowBase::SetSizeConstraint(int x, int y, int w, int h)
{
	FAIL("wxWindowBase::SetSizeConstraint");
}

wxWindowBase::wxWindowBase()
{
	FAIL("wxWindowBase::wxWindowBase");
}

void wxWindowBase::AddChild( wxWindowBase *child )
{
	FAIL("wxWindowBase::AddChild");
}

void wxWindowBase::UpdateWindowUI(long flags)
{
	FAIL("wxWindowBase::UpdateWindowUI");
}

void wxWindowBase::DoCentre(int dir)
{
	FAIL("wxWindowBase::DoCentre");
}

wxWindowBase::~wxWindowBase()
{
	FAIL("wxWindowBase::~wxWindowBase");
}

bool wxWindowBase::Reparent( wxWindowBase *newParent )
{
	FAIL("wxWindowBase::Reparent");
	return true;
}

wxEventHashTable& wxWindowBase::GetEventHashTable() const
{
	FAIL("wxWindowBase::GetEventHashTable");
	return sm_eventHashTable;
}

wxSize wxWindowBase::DoGetVirtualSize() const
{
	FAIL("wxWindowBase::DoGetVirtualSize");
	return wxSize();
}

void wxWindowBase::DoUpdateWindowUI(wxUpdateUIEvent& event)
{
	FAIL("wxWindowBase::DoUpdateWindowUI");
}

void wxWindowBase::Fit()
{
	FAIL("wxWindowBase::Fit");
}

wxPoint wxWindowBase::GetClientAreaOrigin() const
{
	FAIL("wxWindowBase::GetClientAreaOrigin");
	return wxPoint();
}

void wxWindowBase::SetValidator( const wxValidator &validator )
{
	FAIL("wxWindowBase::SetValidator");
}

wxVisualAttributes wxWindowBase::GetClassDefaultAttributes(wxWindowVariant variant)
{
	FAIL("wxWindowBase::GetClassDefaultAttributes");
	return wxVisualAttributes();
}

void wxWindowBase::MakeModal(bool modal)
{
	FAIL("wxWindowBase::MakeModal");
}

bool wxWindowBase::Show( bool show)
{
	FAIL("wxWindowBase::Show");
	return true;
}

void wxWindowBase::ClearBackground()
{
	FAIL("wxWindowBase::ClearBackground");
}

void wxWindowBase::DoSetToolTip( wxToolTip *tip )
{
	FAIL("wxWindowBase::DoSetToolTip");
}

bool wxWindowBase::SetBackgroundColour(const wxColour& colour)
{
	FAIL("wxWindowBase::SetBackgroundColour");
	return true;
}

bool wxWindowBase::LayoutPhase1(int *noChanges)
{
	FAIL("wxWindowBase::LayoutPhase1");
	return true;
}

bool wxWindowBase::LayoutPhase2(int *noChanges)
{
	FAIL("wxWindowBase::LayoutPhase2");
	return true;
}

bool wxWindowBase::IsTopLevel() const
{
	FAIL("wxWindowBase::IsTopLevel");
	return true;
}

void wxWindowBase::SetInitialSize(const wxSize& size)
{
	FAIL("wxWindowBase::SetInitialSize");
}

wxBorder wxWindowBase::GetBorder(long flags) const
{
	FAIL("wxWindowBase::GetBorder");
	return wxBorder();
}

void wxWindowBase::InheritAttributes()
{
	FAIL("wxWindowBase::InheritAttributes");
}

bool wxWindowBase::TryValidator(wxEvent& event)
{
	FAIL("wxWindowBase::TryValidator");
	return true;
}

void wxWindowBase::AdjustForParentClientOrigin(int& x, int& y, int sizeFlags) const
{
	FAIL("wxWindowBase::AdjustForParentClientOrigin");
}

bool wxWindowBase::Enable( bool enable)
{
	FAIL("wxWindowBase::Enable");
	return true;
}

bool wxWindowBase::SetCursor( const wxCursor &cursor )
{
	FAIL("wxWindowBase::SetCursor");
	return true;
}

void wxWindowBase::DoSetSizeHints( int minW, int minH, int maxW, int maxH, int incW, int incH )
{
	FAIL("wxWindowBase::DoSetSizeHints");
}

bool wxWindowBase::DoPhase(int phase)
{
	FAIL("wxWindowBase::DoPhase");
	return true;
}

void wxWindowBase::GetPositionConstraint(int *x, int *y) const
{
	FAIL("wxWindowBase::GetPositionConstraint");
}

void wxWindowBase::MoveConstraint(int x, int y)
{
	FAIL("wxWindowBase::MoveConstraint");
}

wxWindow *wxWindowBase::GetCapture()
{
	FAIL("wxWindowBase::GetCapture");
	return NULL;
}

wxBorder wxWindowBase::GetDefaultBorder() const
{
	FAIL("wxWindowBase::GetDefaultBorder");
	return wxBorder();
}

const wxEventTable* wxWindowBase::GetEventTable() const
{
	FAIL("wxWindowBase::GetEventTable");
	return &sm_eventTable;
}

bool wxWindowBase::Layout()
{
	FAIL("wxWindowBase::Layout");
	return true;
}

wxSize wxWindowBase::GetWindowBorderSize() const
{
	FAIL("wxWindowBase::GetWindowBorderSize");
	return wxSize();
}

void wxWindowBase::DoMoveInTabOrder(wxWindow *win, MoveKind move)
{
	FAIL("wxWindowBase::DoMoveInTabOrder");
}

bool wxWindowBase::TransferDataToWindow()
{
	FAIL("wxWindowBase::TransferDataToWindow");
	return true;
}

wxSize wxWindowBase::DoGetBestSize() const
{
	FAIL("wxWindowBase::DoGetBestSize");
	return wxSize();
}

void wxWindowBase::GetSizeConstraint(int *w, int *h) const
{
	FAIL("wxWindowBase::GetSizeConstraint");
}

bool wxWindowBase::Validate()
{
	FAIL("wxWindowBase::Validate");
	return true;
}

bool wxWindowBase::SetForegroundColour(const wxColour& colour)
{
	FAIL("wxWindowBase::SetForegroundColour");
	return true;
}

void wxWindowBase::DoSetWindowVariant( wxWindowVariant variant )
{
	FAIL("wxWindowBase::DoSetWindowVariant");
}

wxHitTest wxWindowBase::DoHitTest(wxCoord x, wxCoord y) const
{
	FAIL("wxWindowBase::DoHitTest");
	return wxHitTest();
}

bool wxWindowBase::IsShownOnScreen() const
{
	FAIL("wxWindowBase::IsShownOnScreen");
	return true;
}

bool wxWindowBase::Navigate(int flags)
{
	FAIL("wxWindowBase::Navigate");
	return true;
}

bool wxWindowBase::TryParent(wxEvent& event)
{
	FAIL("wxWindowBase::TryParent");
	return true;
}

void wxWindowBase::InitDialog()
{
	FAIL("wxWindowBase::InitDialog");
}

void wxWindowBase::RemoveChild( wxWindowBase *child )
{
	FAIL("wxWindowBase::RemoveChild");
}

void wxWindowBase::DoSetVirtualSize( int x, int y )
{
	FAIL("wxWindowBase::DoSetVirtualSize");
}

wxWindow *wxWindowBase::FindFocus()
{
	FAIL("wxWindowBase::FindFocus");
	return NULL;
}

wxWindow *wxWindowBase::FindWindowById( long winid, const wxWindow *parent)
{
	FAIL("wxWindowBase::FindWindowById");
	return NULL;
}

wxColour wxWindowBase::GetBackgroundColour() const
{
	FAIL("wxWindowBase::GetBackgroundColour");
	return wxColour();
}

wxColour wxWindowBase::GetForegroundColour() const
{
	FAIL("wxWindowBase::GetForegroundColour");
	return wxColour();
}

void wxWindowBase::SetToolTip( const wxString &tip )
{
	FAIL("wxWindowBase::SetToolTip");
}

void wxWindowBase::SetContainingSizer(wxSizer* sizer)
{
	FAIL("wxWindowBase::SetContainingSizer");
}

void wxWindowBase::SetSizer(wxSizer *sizer, bool deleteOld)
{
	FAIL("wxWindowBase::SetSizer");
}

wxFont wxWindowBase::GetFont() const
{
	FAIL("wxWindowBase::GetFont");
	return wxFont();
}

void wxWindowBase::InvalidateBestSize()
{
	FAIL("wxWindowBase::InvalidateBestSize");
}

wxWindow *wxWindowBase::FindWindow(long winid) const
{
	FAIL("wxWindowBase::FindWindow");
	return NULL;
}

bool wxWindowBase::Close( bool force )
{
	FAIL("wxWindowBase::Close");
	return true;
}

const wxEventTable wxWindow::sm_eventTable = wxEventTable();
wxEventHashTable wxWindow::sm_eventHashTable (wxWindow::sm_eventTable);

void wxWindow::DoGetSize(int *width, int *height) const
{
	FAIL("wxWindow::DoGetSize");
}

wxWindow::wxWindow()
{
	FAIL("wxWindow::wxWindow");
}

bool wxWindow::Enable( bool enable)
{
	FAIL("wxWindow::Enable");
	return true;
}

bool wxWindow::SetTransparent(wxByte alpha)
{
	FAIL("wxWindow::SetTransparent");
	return true;
}

bool wxWindow::SetBackgroundColour(const wxColour& colour)
{
	FAIL("wxWindow::SetBackgroundColour");
	return true;
}

int wxWindow::GetScrollRange( int orient ) const
{
	FAIL("wxWindow::GetScrollRange");
	return 0;
}

void wxWindow::Update()
{
	FAIL("wxWindow::Update");
}

long wxWindow::MacGetLeftBorderSize() const
{
	FAIL("wxWindow::MacGetLeftBorderSize");
	return 0;
}

bool wxWindow::MacIsChildOfClientArea( const wxWindow* child ) const
{
	FAIL("wxWindow::MacIsChildOfClientArea");
	return true;
}

void wxWindow::MacChildAdded()
{
	FAIL("wxWindow::MacChildAdded");
}

void wxWindow::MacPaintBorders( int left , int top )
{
	FAIL("wxWindow::MacPaintBorders");
}

void wxWindow::MacGetContentAreaInset( int &left , int &top , int &right , int &bottom )
{
	FAIL("wxWindow::MacGetContentAreaInset");
}

wxInt32 wxWindow::MacControlHit( WXEVENTHANDLERREF handler , WXEVENTREF event )
{
	FAIL("wxWindow::MacControlHit");
	return 0;
}

void wxWindow::MacSuperChangedPosition()
{
	FAIL("wxWindow::MacSuperChangedPosition");
}

void wxWindow::MacHiliteChanged()
{
	FAIL("wxWindow::MacHiliteChanged");
}

void wxWindow::MacSetBackgroundBrush( const wxBrush &brush )
{
	FAIL("wxWindow::MacSetBackgroundBrush");
}

bool wxWindow::MacCanFocus() const
{
	FAIL("wxWindow::MacCanFocus");
	return true;
}

long wxWindow::MacGetTopBorderSize() const
{
	FAIL("wxWindow::MacGetTopBorderSize");
	return 0;
}

void wxWindow::MacEnabledStateChanged()
{
	FAIL("wxWindow::MacEnabledStateChanged");
}

void wxWindow::MacInstallEventHandler(WXWidget native)
{
	FAIL("wxWindow::MacInstallEventHandler");
}

bool wxWindow::Create( wxWindow *parent, wxWindowID id, const wxPoint& pos,
		const wxSize& size, long style, const wxString& name)
{
	FAIL("wxWindow::Create");
	return true;
}

void wxWindow::SetScrollPos( int orient, int pos, bool refresh)
{
	FAIL("wxWindow::SetScrollPos");
}

void wxWindow::SetLabel( const wxString& label )
{
	FAIL("wxWindow::SetLabel");
}

bool wxWindow::IsFrozen() const
{
	FAIL("wxWindow::IsFrozen");
	return false;
}

void wxWindow::DoGetPosition( int *x, int *y ) const
{
	FAIL("wxWindow::DoGetPosition");
}

int wxWindow::GetCharWidth() const
{
	FAIL("wxWindow::GetCharWidth");
	return 0;
}

void wxWindow::SetDropTarget( wxDropTarget *dropTarget)
{
	FAIL("wxWindow::SetDropTarget");
}

void wxWindow::Raise()
{
	FAIL("wxWindow::Raise");
}

void wxWindow::SetScrollbar( int orient, int pos, int thumbVisible, int range, bool refresh)
{
	FAIL("wxWindow::SetScrollbar");
}

wxByte wxWindow::GetTransparent() const
{
	FAIL("wxWindow::GetTransparent");
	return 0;
}

bool wxWindow::SetForegroundColour( const wxColour &colour )
{
	FAIL("wxWindow::SetForegroundColour");
	return true;
}

bool wxWindow::Reparent( wxWindowBase *newParent )
{
	FAIL("wxWindow::Reparent");
	return true;
}

void wxWindow::SetFocus()
{
	FAIL("wxWindow::SetFocus");
}

void wxWindow::WarpPointer( int x, int y )
{
	FAIL("wxWindow::WarpPointer");
}

wxPoint wxWindow::GetClientAreaOrigin() const
{
	FAIL("wxWindow::GetClientAreaOrigin");
	return wxPoint();
}

void wxWindow::DoClientToScreen( int *x, int *y ) const
{
	FAIL("wxWindow::DoClientToScreen");
}

bool wxWindow::SetFont( const wxFont &font )
{
	FAIL("wxWindow::SetFont");
	return true;
}

wxWindow::wxWindow( wxWindowMac *parent, wxWindowID id, const wxPoint& pos,
		const wxSize& size, long style, const wxString& name)
{
	FAIL("wxWindow::wxWindow");
}

wxVisualAttributes wxWindow::GetDefaultAttributes() const
{
	FAIL("wxWindow::GetDefaultAttributes");
	return wxVisualAttributes();
}

void wxWindow::DoSetClientSize(int width, int height)
{
	FAIL("wxWindow::DoSetClientSize");
}

wxSize wxWindow::DoGetBestSize() const
{
	FAIL("wxWindow::DoGetBestSize");
	return wxSize();
}

void wxWindow::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
	FAIL("wxWindow::DoSetSize");
}

void wxWindow::Refresh( bool eraseBackground, const wxRect *rect)
{
	FAIL("wxWindow::Refresh");
}

bool wxWindow::SetCursor( const wxCursor &cursor )
{
	FAIL("wxWindow::SetCursor");
	return true;
}

WXWidget wxWindow::GetHandle() const
{
	FAIL("wxWindow::GetHandle");
	return 0;
}

int wxWindow::GetCharHeight() const
{
	FAIL("wxWindow::GetCharHeight");
	return 0;
}

wxString wxWindow::GetLabel() const
{
	FAIL("wxWindow::GetLabel");
	return L"";
}

void wxWindow::DragAcceptFiles( bool accept )
{
	FAIL("wxWindow::DragAcceptFiles");
}

void wxWindow::RemoveChild( wxWindowBase *child )
{
	FAIL("wxWindow::RemoveChild");
}

void wxWindow::DoScreenToClient( int *x, int *y ) const
{
	FAIL("wxWindow::DoScreenToClient");
}

void wxWindow::DoCaptureMouse()
{
	FAIL("wxWindow::DoCaptureMouse");
}

void wxWindow::ScrollWindow(int dx, int dy, const wxRect* rect)
{
	FAIL("wxWindow::ScrollWindow");
}

int wxWindow::GetScrollThumb( int orient ) const
{
	FAIL("wxWindow::GetScrollThumb");
	return 0;
}

void wxWindow::Freeze()
{
	FAIL("wxWindow::Freeze");
}

void wxWindow::MacVisibilityChanged()
{
	FAIL("wxWindow::MacVisibilityChanged");
}

void wxWindow::DoMoveWindow( int x, int y, int width, int height )
{
	FAIL("wxWindow::DoMoveWindow");
}

void wxWindow::DoGetClientSize( int *width, int *height ) const
{
	FAIL("wxWindow::DoGetClientSize");
}

int wxWindow::GetScrollPos( int orient ) const
{
	FAIL("wxWindow::GetScrollPos");
	return 0;
}

void wxWindow::GetTextExtent( const wxString& string, int *x, int *y,int *descent,
		int *externalLeading, const wxFont *theFont) const
{
	FAIL("wxWindow::GetTextExtent");
}

long wxWindow::MacGetBottomBorderSize() const
{
	FAIL("wxWindow::MacGetBottomBorderSize");
	return 0;
}

void wxWindow::Thaw()
{
	FAIL("wxWindow::Thaw");
}

bool wxWindow::AcceptsFocus() const
{
	FAIL("wxWindow::AcceptsFocus");
	return true;
}

void wxWindow::MacTopLevelWindowChangedPosition()
{
	FAIL("wxWindow::MacTopLevelWindowChangedPosition");
}

void wxWindow::Lower()
{
	FAIL("wxWindow::Lower");
}

void wxWindow::DoReleaseMouse()
{
	FAIL("wxWindow::DoReleaseMouse");
}

bool wxWindow::CanSetTransparent()
{
	FAIL("wxWindow::CanSetTransparent");
	return true;
}

void wxWindow::ClearBackground()
{
	FAIL("wxWindow::ClearBackground");
}

long wxWindow::MacGetRightBorderSize() const
{
	FAIL("wxWindow::MacGetRightBorderSize");
	return 0;
}

void wxWindow::OnInternalIdle()
{
	FAIL("wxWindow::OnInternalIdle");
}

wxWindow::~wxWindow()
{
	FAIL("wxWindow::~wxWindow");
}

void wxWindow::MacHandleControlClick( WXWidget control , wxInt16 controlpart , bool mouseStillDown)
{
	FAIL("wxWindow::MacHandleControlClick");
}

void wxWindow::DoSetToolTip( wxToolTip *tip )
{
	FAIL("wxWindow::DoSetToolTip");
}

wxString wxWindow::MacGetToolTipString( wxPoint &where )
{
	FAIL("wxWindow::MacGetToolTipString");
	return L"";
}

bool wxWindow::DoPopupMenu( wxMenu *menu, int x, int y )
{
	FAIL("wxWindow::DoPopupMenu");
	return true;
}

bool wxWindow::MacSetupCursor( const wxPoint& pt )
{
	FAIL("wxWindow::MacSetupCursor");
	return true;
}

bool wxWindow::Show( bool show)
{
	FAIL("wxWindow::Show");
	return true;
}

void wxWindow::DoSetWindowVariant( wxWindowVariant variant )
{
	FAIL("wxWindow::DoSetWindowVariant");
}

bool wxWindow::MacDoRedraw( WXHRGN updatergn , long time )
{
	FAIL("wxWindow::MacDoRedraw");
	return true;
}

const wxEventTable* wxWindow::GetEventTable() const
{
	FAIL("wxWindow::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxWindow::GetEventHashTable() const
{
	FAIL("wxWindow::GetEventHashTable");
	return sm_eventHashTable;
}

wxSize wxWindow::DoGetSizeFromClientSize( const wxSize & size ) const
{
	FAIL("wxWindow::DoGetSizeFromClientSize");
	return wxSize();
}

