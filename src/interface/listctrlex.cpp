#include "FileZilla.h"
#include "listctrlex.h"
#include <wx/renderer.h>
#include <wx/tokenzr.h>
#include "Options.h"

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
	m_pVisibleColumnMapping = 0;
	m_prefixSearch_enabled = false;
}

wxListCtrlEx::~wxListCtrlEx()
{
	delete [] m_pVisibleColumnMapping;
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

	int code = event.GetKeyCode();
	if (code == WXK_LEFT ||
		code == WXK_RIGHT ||
		code == WXK_UP ||
		code == WXK_DOWN ||
		code == WXK_HOME ||
		code == WXK_END)
	{
		ResetSearchPrefix();
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
	return pThis->GetItemText(item, (unsigned int)m_pVisibleColumnMapping[column]);
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

void wxListCtrlEx::SaveSetItemCount(long count)
{
#ifndef __WXMSW__
	int focused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
	if (focused >= count)
		SetItemState(focused, 0, wxLIST_STATE_FOCUSED);
#endif //__WXMSW__
	SetItemCount(count);
}

void wxListCtrlEx::ResetSearchPrefix()
{
	m_prefixSearch_prefix = _T("");
}

void wxListCtrlEx::ShowColumn(unsigned int col, bool show)
{
	if (col >= m_columnInfo.size())
		return;

	if (m_columnInfo[col].shown == show)
		return;

	m_columnInfo[col].shown = show;

	if (show)
	{
		int i;
		for (i = 0; i < GetColumnCount(); i++)
		{
			if (m_pVisibleColumnMapping[i] >= col)
				break;
		}

		for (int j = i; j < GetColumnCount(); j++)
			m_pVisibleColumnMapping[j + 1] = m_pVisibleColumnMapping[j];
		m_pVisibleColumnMapping[i] = col;

		InsertColumn(i, m_columnInfo[col].name, m_columnInfo[col].align, m_columnInfo[col].width);
	}
	else
	{
		int i;
		for (i = 0; i < GetColumnCount(); i++)
		{
			if (m_pVisibleColumnMapping[i] == col)
				break;
		}
		for (int j = i + 1; j < GetColumnCount(); j++)
			m_pVisibleColumnMapping[j - 1] = m_pVisibleColumnMapping[j];

		wxASSERT(i < GetColumnCount());

		m_columnInfo[col].width = GetColumnWidth(i);
		DeleteColumn(i);
	}
}

void wxListCtrlEx::LoadColumnSettings(int widthsOptionId, int visibilityOptionId)
{
	wxASSERT(!GetColumnCount());

	if (widthsOptionId != -1)
		ReadColumnWidths(widthsOptionId);

	delete [] m_pVisibleColumnMapping;
	m_pVisibleColumnMapping = new int[m_columnInfo.size()];

	if (visibilityOptionId != -1)
	{
		wxString visibleColumns = COptions::Get()->GetOption(visibilityOptionId);
		if (visibleColumns.Len() >= m_columnInfo.size())
		{
			for (unsigned int i = 0; i < m_columnInfo.size(); i++)
				m_columnInfo[i].shown = visibleColumns[i] == '1';
		}
	}

	int j = 0;
	for (unsigned int i = 0; i < m_columnInfo.size(); i++)
	{
		const t_columnInfo &column = m_columnInfo[i];

		if (!column.shown)
			continue;

		m_pVisibleColumnMapping[j++] = i;
		InsertColumn(j, column.name, column.align, column.width);
	}
}

void wxListCtrlEx::SaveColumnSettings(int widthsOptionId, int visibilityOptionId)
{
	if (widthsOptionId != -1)
		SaveColumnWidths(widthsOptionId);

	if (visibilityOptionId != -1)
	{
		wxString visibleColumns;
		for (unsigned int i = 0; i < m_columnInfo.size(); i++)
		{
			if (m_columnInfo[i].shown)
				visibleColumns += _T("1");
			else
				visibleColumns += _T("0");
		}
		COptions::Get()->SetOption(visibilityOptionId, visibleColumns);
	}
}

bool wxListCtrlEx::ReadColumnWidths(unsigned int optionId)
{
	wxASSERT(!GetColumnCount());

	if (wxGetKeyState(WXK_SHIFT) &&
		wxGetKeyState(WXK_ALT) &&
		wxGetKeyState(WXK_CONTROL))
	{
		return true;
	}

	const unsigned int count = m_columnInfo.size();

	wxString savedWidths = COptions::Get()->GetOption(optionId);
	wxStringTokenizer tokens(savedWidths, _T(" "));
	if (tokens.CountTokens() < count)
		return false;

	unsigned long* newWidths = new unsigned long[count];
	for (unsigned int i = 0; i < count; i++)
	{
		wxString token = tokens.GetNextToken();
		if (!token.ToULong(newWidths + i) || newWidths[i] > 5000)
		{
			delete [] newWidths;
			return false;
		}
	}

	for (unsigned int i = 0; i < count; i++)
		m_columnInfo[i].width = newWidths[i];

	delete [] newWidths;
	return true;
}

void wxListCtrlEx::SaveColumnWidths(unsigned int optionId)
{
	const unsigned int count = m_columnInfo.size();

	wxString widths;
	for (unsigned int i = 0; i < count; i++)
	{
		int width;

		bool found = false;
		for (int j = 0; j < GetColumnCount(); j++)
		{
			if (m_pVisibleColumnMapping[j] != i)
				continue;

			found = true;
			width = GetColumnWidth(j);
		}
		if (!found)
			width = m_columnInfo[i].width;
		widths += wxString::Format(_T("%d "), width);
	}
	widths.RemoveLast();

	COptions::Get()->SetOption(optionId, widths);
}


void wxListCtrlEx::AddColumn(const wxString& name, int align, int initialWidth)
{
	wxASSERT(!GetColumnCount());

	t_columnInfo info;
	info.name = name;
	info.align = align;
	info.width = initialWidth;
	info.shown = true;
	info.order = m_columnInfo.size();

	m_columnInfo.push_back(info);
}
