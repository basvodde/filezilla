#include "FileZilla.h"
#include "listctrlex.h"
#include <wx/renderer.h>

DECLARE_EVENT_TYPE(fzEVT_POSTSCROLL, -1)
DEFINE_EVENT_TYPE(fzEVT_POSTSCROLL)

BEGIN_EVENT_TABLE(wxListCtrlEx, wxListCtrl)
EVT_COMMAND(wxID_ANY, fzEVT_POSTSCROLL, wxListCtrlEx::OnPostScrollEvent)
EVT_SCROLLWIN(wxListCtrlEx::OnScrollEvent)
EVT_MOUSEWHEEL(wxListCtrlEx::OnMouseWheel)
EVT_LIST_ITEM_FOCUSED(wxID_ANY, wxListCtrlEx::OnSelectionChanged)
EVT_LIST_ITEM_SELECTED(wxID_ANY, wxListCtrlEx::OnSelectionChanged)
EVT_KEY_DOWN(wxListCtrlEx::OnKeyDown)
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
	m_prefixSearch_enabled = false;
}

wxListCtrlEx::~wxListCtrlEx()
{
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


void wxListCtrlEx::HandlePrefixSearch(wxChar character)
{
	wxASSERT(character);

	// Keyboard navigation within items
	wxDateTime now = wxDateTime::UNow();
	if (m_prefixSearch_lastKeyPress.IsValid())
	{
		wxTimeSpan span = now - m_prefixSearch_lastKeyPress;
		if (span.GetSeconds() >= 1)
			m_prefixSearch_prefix = _T("");
	}
	m_prefixSearch_lastKeyPress = now;

	wxString newPrefix = m_prefixSearch_prefix + character;

	bool beep = false;
	int item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != -1)
	{
		wxString text = GetItemText(item, 0);
		if (text.Length() >= m_prefixSearch_prefix.Length() && !m_prefixSearch_prefix.CmpNoCase(text.Left(m_prefixSearch_prefix.Length())))
			beep = true;
	}
	else if (m_prefixSearch_prefix == _T(""))
		beep = true;

	int start = item;
	if (start < 0)
		start = 0;

	int newPos = FindItemWithPrefix(newPrefix, start);

	if (newPos == -1 && m_prefixSearch_prefix[0] == character && !m_prefixSearch_prefix[1] && item != -1 && beep)
	{
		// Search the next item that starts with the same letter
		newPrefix = m_prefixSearch_prefix;
		newPos = FindItemWithPrefix(newPrefix, item + 1);
	}

	m_prefixSearch_prefix = newPrefix;
	if (newPos == -1)
	{
		if (beep)
			wxBell();
		return;
	}

	item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while (item != -1)
	{
		SetItemState(item, 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
	SetItemState(newPos, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	EnsureVisible(newPos);
}

void wxListCtrlEx::OnKeyDown(wxKeyEvent& event)
{
	if (!m_prefixSearch_enabled)
	{
		event.Skip();
		return;
	}

#if defined(__WXMSW__) && wxUSE_UNICODE
	wxChar key = MapVirtualKey(event.GetUnicodeKey(), 2);
	if (key < 32)
	{
		event.Skip();
		return;
	}
	else if (key == 32 || event.HasModifiers())
	{
		event.Skip();
		return;
	}
	HandlePrefixSearch(key);
	return;
#else
	int code = event.GetKeyCode();
	if (code > 32 && code < 300 && !event.HasModifiers())
	{
#if wxUSE_UNICODE
		int unicodeKey = event.GetUnicodeKey();
		if (unicodeKey)
			code = unicodeKey;
#endif
		HandlePrefixSearch(code);
	}
	else
		event.Skip();
#endif //defined(__WXMSW__) && wxUSE_UNICODE
}

// Declared const due to design error in wxWidgets.
// Won't be fixed since a fix would break backwards compatibility
// Both functions use a const_cast<CLocalListView *>(this) and modify
// the instance.
wxString wxListCtrlEx::OnGetItemText(long item, long column) const
{
	wxListCtrlEx *pThis = const_cast<wxListCtrlEx *>(this);
	return pThis->GetItemText(item, (unsigned int)column);
}

int wxListCtrlEx::FindItemWithPrefix(const wxString& searchPrefix, int start)
{
	const int count = GetItemCount();
	for (int i = start; i < (count + start); i++)
	{
		int item = i % count;
		wxString namePrefix = GetItemText(item, 0).Left(searchPrefix.Length());
		if (!namePrefix.CmpNoCase(searchPrefix))
			return i % count;
	}
	return -1;
}

#ifdef __WXGTK__
wxBitmap wxListCtrlEx::GetSortArrow(bool sortDown)
{
	wxBitmap bmp(16, 16);
	wxMemoryDC dc(bmp);

	dc.Clear();

	wxRendererNative& renderer = wxRendererNative::Get();
	renderer.DrawDropArrow(this, dc, wxRect(0, 0, 20, 20), 0);

	return bmp;
}
#endif //__WXGTK__

void wxListCtrlEx::SaveSetItemCount(long count)
{
#ifndef __WXMSW__
	int focused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
	if (focused >= count)
		SetItemState(focused, 0, wxLIST_STATE_FOCUSED);
#endif __WXMSW__
	SetItemCount(count);
}
