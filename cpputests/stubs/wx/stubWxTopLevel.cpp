

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/toplevel.h"

wxClassInfo wxTopLevelWindow::ms_classInfo(NULL, NULL, NULL, 0, NULL);

const wxChar wxFrameNameStr[100] = L"";

const wxEventTable wxTopLevelWindowBase::sm_eventTable = wxEventTable();
wxEventHashTable wxTopLevelWindowBase::sm_eventHashTable (wxTopLevelWindowBase::sm_eventTable);

const wxEventTable* wxTopLevelWindowBase::GetEventTable() const
{
	FAIL("wxTopLevelWindowBase::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxTopLevelWindowBase::GetEventHashTable() const
{
	FAIL("wxTopLevelWindowBase::GetEventHashTable");
	return sm_eventHashTable;
}

wxTopLevelWindowBase::wxTopLevelWindowBase()
{
	FAIL("wxTopLevelWindowBase::wxTopLevelWindowBase");
}

wxTopLevelWindowBase::~wxTopLevelWindowBase()
{
	FAIL("wxTopLevelWindowBase::~wxTopLevelWindowBase");
}

void wxTopLevelWindowBase::SetMinSize(const wxSize& minSize)
{
	FAIL("wxTopLevelWindowBase::SetMinSize");
}

void wxTopLevelWindowBase::DoUpdateWindowUI(wxUpdateUIEvent& event)
{
	FAIL("wxTopLevelWindowBase::DoUpdateWindowUI");
}

void wxTopLevelWindowBase::DoSetSizeHints( int minW, int minH, int maxW, int maxH, int incW, int incH)
{
	FAIL("wxTopLevelWindowBase::DoSetSizeHints");
}

bool wxTopLevelWindowBase::IsAlwaysMaximized() const
{
	FAIL("wxTopLevelWindowBase::IsAlwaysMaximized");
	return true;
}

void wxTopLevelWindowBase::SetMaxSize(const wxSize& maxSize)
{
	FAIL("wxTopLevelWindowBase::SetMaxSize");
}

void wxTopLevelWindowBase::DoScreenToClient(int *x, int *y) const
{
	FAIL("wxTopLevelWindowBase::DoScreenToClient");
}

void wxTopLevelWindowBase::DoClientToScreen(int *x, int *y) const
{
	FAIL("wxTopLevelWindowBase::DoClientToScreen");
}

void wxTopLevelWindowBase::GetRectForTopLevelChildren(int *x, int *y, int *w, int *h)
{
	FAIL("wxTopLevelWindowBase::GetRectForTopLevelChildren");
}

void wxTopLevelWindowBase::DoLayout()
{
	FAIL("wxTopLevelWindowBase::DoLayout");
}

bool wxTopLevelWindowBase::Destroy()
{
	FAIL("wxTopLevelWindowBase::Destroy");
	return true;
}

void wxTopLevelWindowBase::DoCentre(int dir)
{
	FAIL("wxTopLevelWindowBase::DoCentre");
}

void wxTopLevelWindowBase::RemoveChild(wxWindowBase *child)
{
	FAIL("wxTopLevelWindowBase::RemoveChild");
}

void wxTopLevelWindowBase::RequestUserAttention(int flags)
{
	FAIL("wxTopLevelWindowBase::RequestUserAttention");
}

const wxEventTable wxTopLevelWindowMac::sm_eventTable = wxEventTable();
wxEventHashTable wxTopLevelWindowMac::sm_eventHashTable (wxTopLevelWindowMac::sm_eventTable);

const wxEventTable* wxTopLevelWindowMac::GetEventTable() const
{
	FAIL("wxTopLevelWindowBase::GetEventTable");
	return &sm_eventTable;
}

wxEventHashTable& wxTopLevelWindowMac::GetEventHashTable() const
{
	FAIL("wxTopLevelWindowBase::GetEventHashTable");
	return sm_eventHashTable;
}


bool wxTopLevelWindowMac::IsFullScreen() const
{
	FAIL("wxTopLevelWindowMac::IsFullScreen");
	return true;
}

bool wxTopLevelWindowMac::IsIconized() const
{
	FAIL("wxTopLevelWindowMac::IsIconized");
	return true;
}

bool wxTopLevelWindowMac::IsMaximized() const
{
	FAIL("wxTopLevelWindowMac::IsMaximized");
	return true;
}

void wxTopLevelWindowMac::MacGetContentAreaInset( int &left , int &top , int &right , int &bottom )
{
	FAIL("wxTopLevelWindowMac::MacGetContentAreaInset");
}

void wxTopLevelWindowMac::DoMacCreateRealWindow( wxWindow *parent, const wxString& title,
		const wxPoint& pos, const wxSize& size, long style, const wxString& name )
{
	FAIL("wxTopLevelWindowMac::DoMacCreateRealWindow");
}

bool wxTopLevelWindowMac::SetTransparent(wxByte alpha)
{
	FAIL("wxTopLevelWindowMac::SetTransparent");
	return true;
}

void wxTopLevelWindowMac::ClearBackground()
{
	FAIL("wxTopLevelWindowMac::ClearBackground");
}

void wxTopLevelWindowMac::DoGetClientSize(int *width, int *height) const
{
	FAIL("wxTopLevelWindowMac::DoGetClientSize");
}

void wxTopLevelWindowMac::DoMoveWindow(int x, int y, int width, int height)
{
	FAIL("wxTopLevelWindowMac::DoMoveWindow");
}
void wxTopLevelWindowMac::MacPerformUpdates()
{
	FAIL("wxTopLevelWindowMac::MacPerformUpdates");
}

void wxTopLevelWindowMac::SetLabel( const wxString& title)
{
	FAIL("wxTopLevelWindowMac::SetLabel");
}

void wxTopLevelWindowMac::SetExtraStyle(long exStyle)
{
	FAIL("wxTopLevelWindowMac::SetExtraStyle");
}

void wxTopLevelWindowMac::DoGetPosition( int *x, int *y ) const
{
	FAIL("wxTopLevelWindowMac::DoGetPosition");
}

void wxTopLevelWindowMac::RequestUserAttention(int flags)
{
	FAIL("wxTopLevelWindowMac::RequestUserAttention");
}

wxString wxTopLevelWindowMac::GetTitle() const
{
	FAIL("wxTopLevelWindowMac::GetTitle");
	return L"";
}

bool wxTopLevelWindowMac::Destroy()
{
	FAIL("wxTopLevelWindowMac::Destroy");
	return true;
}

void wxTopLevelWindowMac::Maximize(bool maximize)
{
	FAIL("wxTopLevelWindowMac::Maximize");
}

void wxTopLevelWindowMac::MacCreateRealWindow( const wxString& title, const wxPoint& pos, const wxSize& size,
		long style, const wxString& name )
{
	FAIL("wxTopLevelWindowMac::MacCreateRealWindow");
}

void wxTopLevelWindowMac::MacActivate( long timestamp , bool inIsActivating )
{
	FAIL("wxTopLevelWindowMac::MacActivate");
}

void wxTopLevelWindowMac::Restore()
{
	FAIL("wxTopLevelWindowMac::Restore");
}

void wxTopLevelWindowMac::DoCentre(int dir)
{
	FAIL("wxTopLevelWindowMac::DoCentre");
}

wxPoint wxTopLevelWindowMac::GetClientAreaOrigin() const
{
	FAIL("wxTopLevelWindowMac::GetClientAreaOrigin");
	return wxPoint();
}

void wxTopLevelWindowMac::SetTitle( const wxString& title)
{
	FAIL("wxTopLevelWindowMac::SetTitle");
}

bool wxTopLevelWindowMac::ShowFullScreen(bool show, long style)
{
	FAIL("wxTopLevelWindowMac::ShowFullScreen");
	return true;
}

void wxTopLevelWindowMac::DoGetSize( int *width, int *height ) const
{
	FAIL("wxTopLevelWindowMac::DoGetSize");
}

void wxTopLevelWindowMac::MacSetBackgroundBrush( const wxBrush &brush )
{
	FAIL("wxTopLevelWindowMac::MacSetBackgroundBrush");
}

void wxTopLevelWindowMac::SetIcon(const wxIcon& icon)
{
	FAIL("wxTopLevelWindowMac::SetIcon");
}

bool wxTopLevelWindowMac::CanSetTransparent()
{
	FAIL("wxTopLevelWindowMac::CanSetTransparent");
	return true;
}

void wxTopLevelWindowMac::Iconize(bool iconize)
{
	FAIL("wxTopLevelWindowMac::Iconize");
}

void wxTopLevelWindowMac::MacInstallTopLevelWindowEventHandler()
{
	FAIL("wxTopLevelWindowMac::MacInstallTopLevelWindowEventHandler");
}

bool wxTopLevelWindowMac::SetShape(const wxRegion& region)
{
	FAIL("wxTopLevelWindowMac::SetShape");
	return true;
}

void wxTopLevelWindowMac::Raise()
{
	FAIL("wxTopLevelWindowMac::Raise");
}

void wxTopLevelWindowMac::Lower()
{
	FAIL("wxTopLevelWindowMac::Lower");
}

wxTopLevelWindowMac::~wxTopLevelWindowMac()
{
	FAIL("wxTopLevelWindowMac::~wxTopLevelWindowMac");
}

bool wxTopLevelWindowMac::Show( bool show)
{
	FAIL("wxTopLevelWindowMac::Show");
	return true;
}

void wxTopLevelWindowMac::Init()
{
	FAIL("wxTopLevelWindowMac::Init");
}

wxClassInfo *wxTopLevelWindow::GetClassInfo() const
{
	FAIL("wxTopLevelWindow::GetClassInfo");
	return NULL;
}


