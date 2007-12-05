#include "FileZilla.h"
#include "listctrlex.h"

DECLARE_EVENT_TYPE(fzEVT_POSTSCROLL, -1)
DEFINE_EVENT_TYPE(fzEVT_POSTSCROLL)

BEGIN_EVENT_TABLE(wxListCtrlEx, wxListCtrl)
EVT_COMMAND(wxID_ANY, fzEVT_POSTSCROLL, wxListCtrlEx::OnPostScrollEvent)
EVT_SCROLLWIN(wxListCtrlEx::OnScrollEvent)
EVT_MOUSEWHEEL(wxListCtrlEx::OnMouseWheel)
EVT_LIST_ITEM_FOCUSED(wxID_ANY, wxListCtrlEx::OnSelectionChanged)
EVT_LIST_ITEM_SELECTED(wxID_ANY, wxListCtrlEx::OnSelectionChanged)
END_EVENT_TABLE()

wxListCtrlEx::wxListCtrlEx(wxWindow *parent,
						   wxWindowID id,
						   const wxPoint& pos,
						   const wxSize& size,
						   long style,
						   const wxValidator& validator,
						   const wxString& name)
						   : wxListCtrl(parent, id, pos, size, style, validator, name)
{
#ifndef __WXMSW__
	// The generic list control a scrolled child window. In order to receive
	// scroll events, we have to connect the event handler to it.
	GetMainWindow()->Connect(-1, wxEVT_SCROLLWIN_TOP, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
	GetMainWindow()->Connect(-1, wxEVT_SCROLLWIN_BOTTOM, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
	GetMainWindow()->Connect(-1, wxEVT_SCROLLWIN_LINEUP, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
	GetMainWindow()->Connect(-1, wxEVT_SCROLLWIN_LINEDOWN, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
	GetMainWindow()->Connect(-1, wxEVT_SCROLLWIN_PAGEUP, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
	GetMainWindow()->Connect(-1, wxEVT_SCROLLWIN_PAGEDOWN, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
#endif
}

wxListCtrlEx::~wxListCtrlEx()
{
#ifndef __WXMSW__
	GetMainWindow()->Disconnect(-1, wxEVT_SCROLLWIN_TOP, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
	GetMainWindow()->Disconnect(-1, wxEVT_SCROLLWIN_BOTTOM, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
	GetMainWindow()->Disconnect(-1, wxEVT_SCROLLWIN_LINEUP, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
	GetMainWindow()->Disconnect(-1, wxEVT_SCROLLWIN_LINEDOWN, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
	GetMainWindow()->Disconnect(-1, wxEVT_SCROLLWIN_PAGEUP, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
	GetMainWindow()->Disconnect(-1, wxEVT_SCROLLWIN_PAGEDOWN, wxScrollWinEventHandler(wxListCtrlEx::OnScrollEvent), 0, this);
#endif
}

#ifndef __WXMSW__
wxScrolledWindow* wxListCtrlEx::GetMainWindow()
{
#ifdef __WXMAC__
	return (wxScrolledWindow*)m_genericImpl->m_mainWin;
#else
	return (wxScrolledWindow*)m_mainWin;
#endif
}
#endif

void wxListCtrlEx::OnPostScroll()
{
}

void wxListCtrlEx::OnPostScrollEvent(wxCommandEvent& event)
{
	OnPostScroll();
}

void wxListCtrlEx::OnPreEmitPostScrollEvent()
{
	EmitPostScrollEvent();
}

void wxListCtrlEx::EmitPostScrollEvent()
{
	wxCommandEvent evt(fzEVT_POSTSCROLL, wxID_ANY);
	AddPendingEvent(evt);
}

void wxListCtrlEx::OnScrollEvent(wxScrollWinEvent& event)
{
	event.Skip();
	OnPreEmitPostScrollEvent();
}

void wxListCtrlEx::OnMouseWheel(wxMouseEvent& event)
{
	event.Skip();
	OnPreEmitPostScrollEvent();
}

void wxListCtrlEx::OnSelectionChanged(wxListEvent& event)
{
	event.Skip();
	OnPreEmitPostScrollEvent();
}

void wxListCtrlEx::ScrollTopItem(int item)
{
#ifdef __WXMSW__
	const int current = GetTopItem();

	int delta = item - current;
	if (!delta)
		return;

	wxRect rect;
	GetItemRect(current, rect, wxLIST_RECT_BOUNDS);

	delta *= rect.GetHeight();
	ScrollList(0, delta);
#else
	GetMainWindow()->Scroll(0, item);
	EnsureVisible(item);
#endif
}
