#include <filezilla.h>
#include "listctrlex.h"
#include "filezillaapp.h"
#include <wx/renderer.h>
#include <wx/tokenzr.h>
#include "Options.h"
#include "dialogex.h"
#ifdef __WXMSW__
#include "commctrl.h"

#ifndef HDF_SORTUP
#define HDF_SORTUP              0x0400
#endif
#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN            0x0200
#endif
#else
#include "themeprovider.h"
#endif

DECLARE_EVENT_TYPE(fzEVT_POSTSCROLL, -1)
DEFINE_EVENT_TYPE(fzEVT_POSTSCROLL)

BEGIN_EVENT_TABLE(wxListCtrlEx, wxListCtrl)
EVT_COMMAND(wxID_ANY, fzEVT_POSTSCROLL, wxListCtrlEx::OnPostScrollEvent)
EVT_SCROLLWIN(wxListCtrlEx::OnScrollEvent)
EVT_MOUSEWHEEL(wxListCtrlEx::OnMouseWheel)
EVT_LIST_ITEM_FOCUSED(wxID_ANY, wxListCtrlEx::OnSelectionChanged)
EVT_LIST_ITEM_SELECTED(wxID_ANY, wxListCtrlEx::OnSelectionChanged)
EVT_KEY_DOWN(wxListCtrlEx::OnKeyDown)
EVT_LIST_BEGIN_LABEL_EDIT(wxID_ANY, wxListCtrlEx::OnBeginLabelEdit)
EVT_LIST_END_LABEL_EDIT(wxID_ANY, wxListCtrlEx::OnEndLabelEdit)
#ifndef __WXMSW__
EVT_LIST_COL_DRAGGING(wxID_ANY, wxListCtrlEx::OnColumnDragging)
#endif
END_EVENT_TABLE()

#define MIN_COLUMN_WIDTH 12

wxListCtrlEx::wxListCtrlEx(wxWindow *parent,
						   wxWindowID id,
						   const wxPoint& pos,
						   const wxSize& size,
						   long style,
						   const wxValidator& validator,
						   const wxString& name)
						   : wxListCtrl(parent, id, pos, size, style, validator, name)
{
#ifdef __WXMSW__
	m_pHeaderImageList = 0;
#endif
	m_header_icon_index.down = m_header_icon_index.up = -1;

	m_pVisibleColumnMapping = 0;
	m_prefixSearch_enabled = false;

#ifndef __WXMSW__
	m_editing = false;
#else
	m_columnDragging = false;
#endif
	m_blockedLabelEditing = false;
}

wxListCtrlEx::~wxListCtrlEx()
{
#ifdef __WXMSW__
	delete m_pHeaderImageList;
#endif
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

	int item;
#ifndef __WXMSW__
	// GetNextItem is O(n) if nothing is selected, GetSelectedItemCount() is O(1)
	if (!GetSelectedItemCount())
		item = -1;
	else
#endif
	{
		item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}

	bool beep = false;
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

	while (item != -1)
	{
		SetItemState(item, 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
	SetItemState(newPos, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);

#ifdef __WXMSW__
	// SetItemState does not move the selection mark, that is the item from
	// which a multiple selection starts (e.g. shift+up/down)
	HWND hWnd = (HWND)GetHandle();
	::SendMessage(hWnd, LVM_SETSELECTIONMARK, 0, newPos);
#endif

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
		event.Skip();
		return;
	}

	if (event.AltDown() && !event.ControlDown()) // Alt but not AltGr
	{
		event.Skip();
		return;
	}

	wxChar key;

	switch (code)
	{
	case WXK_NUMPAD0:
	case WXK_NUMPAD1:
	case WXK_NUMPAD2:
	case WXK_NUMPAD3:
	case WXK_NUMPAD4:
	case WXK_NUMPAD5:
	case WXK_NUMPAD6:
	case WXK_NUMPAD7:
	case WXK_NUMPAD8:
	case WXK_NUMPAD9:
		key = '0' + code - WXK_NUMPAD0;
		break;
	case WXK_NUMPAD_ADD:
		key = '+';
		break;
	case WXK_NUMPAD_SUBTRACT:
		key = '-';
		break;
	case WXK_NUMPAD_MULTIPLY:
		key = '*';
		break;
	case WXK_NUMPAD_DIVIDE:
		key = '/';
		break;
	default:
		key = 0;
		break;
	}
	if (key)
	{
		if (event.GetModifiers())
		{
			// Numpad keys can not have modifiers
			event.Skip();
		}
		HandlePrefixSearch(key);
		return;
	}

#if defined(__WXMSW__) && wxUSE_UNICODE

	if (code >= 300 && code != WXK_NUMPAD_DECIMAL)
	{
		event.Skip();
		return;
	}

	// Get the actual key
	BYTE state[256];
	GetKeyboardState(state);
	wxChar buffer[1];
	int res = ToUnicode(event.GetRawKeyCode(), 0, state, buffer, 1, 0);
	if (res != 1)
	{
		event.Skip();
		return;
	}
	
	key = buffer[0];

	if (key < 32)
	{
		event.Skip();
		return;
	}
	if (key == 32 && event.HasModifiers())
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
	if (count < GetItemCount())
	{
		int focused = GetNextItem(count - 1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
		if (focused != -1)
			SetItemState(focused, 0, wxLIST_STATE_FOCUSED);
	}
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
		// Insert new column
		int pos = 0;
		for (unsigned int i = 0; i < m_columnInfo.size(); i++)
		{
			if (i == col)
				continue;
			t_columnInfo& info = m_columnInfo[i];
			if (info.shown && info.order < m_columnInfo[col].order)
				pos++;
		}
		for (int i = GetColumnCount() - 1; i >= pos; i--)
			m_pVisibleColumnMapping[i + 1] = m_pVisibleColumnMapping[i];
		m_pVisibleColumnMapping[pos] = col;

		t_columnInfo& info = m_columnInfo[col];
		InsertColumn(pos, info.name, info.align, info.width);
	}
	else
	{
		int i;
		for (i = 0; i < GetColumnCount(); i++)
		{
			if (m_pVisibleColumnMapping[i] == col)
				break;
		}
		wxASSERT(m_columnInfo[col].order >= (unsigned int)i);
		for (int j = i + 1; j < GetColumnCount(); j++)
			m_pVisibleColumnMapping[j - 1] = m_pVisibleColumnMapping[j];

		wxASSERT(i < GetColumnCount());

		m_columnInfo[col].width = GetColumnWidth(i);
		DeleteColumn(i);
	}
}

void wxListCtrlEx::LoadColumnSettings(int widthsOptionId, int visibilityOptionId, int sortOptionId)
{
	wxASSERT(!GetColumnCount());

	if (widthsOptionId != -1)
		ReadColumnWidths(widthsOptionId);

	delete [] m_pVisibleColumnMapping;
	m_pVisibleColumnMapping = new unsigned int[m_columnInfo.size()];

	if (visibilityOptionId != -1)
	{
		wxString visibleColumns = COptions::Get()->GetOption(visibilityOptionId);
		if (visibleColumns.Len() >= m_columnInfo.size())
		{
			for (unsigned int i = 0; i < m_columnInfo.size(); i++)
			{
				if (!m_columnInfo[i].fixed)
					m_columnInfo[i].shown = visibleColumns[i] == '1';
			}
		}
	}

	if (sortOptionId != -1)
	{
		wxString strorder = COptions::Get()->GetOption(sortOptionId);
		wxStringTokenizer tokens(strorder, _T(","));

		unsigned int count = tokens.CountTokens();
		if (count == m_columnInfo.size())
		{
			unsigned long *order = new unsigned long[count];
			bool *order_set = new bool[count];
			memset(order_set, 0, sizeof(bool) * count);

			unsigned int i = 0;
			while (tokens.HasMoreTokens())
			{
				if (!tokens.GetNextToken().ToULong(&order[i]))
					break;
				if (order[i] >= count || order_set[order[i]])
					break;
				order_set[order[i]] = true;
				i++;
			}
			if (i == count)
			{
				bool valid = true;
				for (unsigned int j = 0; j < m_columnInfo.size(); j++)
				{
					if (!m_columnInfo[j].fixed)
						continue;

					if (j != order[j])
					{
						valid = false;
						break;
					}
				}

				if (valid)
					for (unsigned int i = 0; i < m_columnInfo.size(); i++)
						m_columnInfo[i].order = order[i];
			}

			delete [] order;
			delete [] order_set;
		}
	}

	CreateVisibleColumnMapping();
}

void wxListCtrlEx::SaveColumnSettings(int widthsOptionId, int visibilityOptionId, int sortOptionId)
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

	if (sortOptionId != -1)
	{
		wxString order;
		for (unsigned int i = 0; i < m_columnInfo.size(); i++)
		{
			if (i)
				order += _T(",");
			order += wxString::Format(_T("%d"), m_columnInfo[i].order);
		}
		COptions::Get()->SetOption(sortOptionId, order);
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
		else if (newWidths[i] < MIN_COLUMN_WIDTH)
			newWidths[i] = MIN_COLUMN_WIDTH;
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
		int width = 0;

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


void wxListCtrlEx::AddColumn(const wxString& name, int align, int initialWidth, bool fixed /*=false*/)
{
	wxASSERT(!GetColumnCount());

	t_columnInfo info;
	info.name = name;
	info.align = align;
	info.width = initialWidth;
	info.shown = true;
	info.order = m_columnInfo.size();
	info.fixed = fixed;

	m_columnInfo.push_back(info);
}

// Moves column. Target position includes both hidden
// as well as shown columns
void wxListCtrlEx::MoveColumn(unsigned int col, unsigned int before)
{
	if (m_columnInfo[col].order == before)
		return;

	for (unsigned int i = 0; i < m_columnInfo.size(); i++)
	{
		if (i == col)
			continue;

		t_columnInfo& info = m_columnInfo[i];
		if (info.order > col)
			info.order--;
		if (info.order >= before)
			info.order++;
	}

	int icon = -1;

	t_columnInfo& info = m_columnInfo[col];
	if (info.shown)
	{
		// Remove old column
		for (unsigned int i = 0; i < (unsigned int)GetColumnCount(); i++)
		{
			if (m_pVisibleColumnMapping[i] != col)
				continue;

			for (unsigned int j = i + 1; j < (unsigned int)GetColumnCount(); j++)
				m_pVisibleColumnMapping[j - 1] = m_pVisibleColumnMapping[j];
			info.width = GetColumnWidth(i);

			icon = GetHeaderSortIconIndex(i);
			DeleteColumn(i);

			break;
		}

		// Insert new column
		unsigned int pos = 0;
		for (unsigned int i = 0; i < m_columnInfo.size(); i++)
		{
			if (i == col)
				continue;
			t_columnInfo& info = m_columnInfo[i];
			if (info.shown && info.order < before)
				pos++;
		}
		for (unsigned int i = (int)GetColumnCount(); i > pos; i--)
			m_pVisibleColumnMapping[i] = m_pVisibleColumnMapping[i - 1];
		m_pVisibleColumnMapping[pos] = col;

		InsertColumn(pos, info.name, info.align, info.width);

		SetHeaderSortIconIndex(pos, icon);
	}
	m_columnInfo[col].order = before;
}

void wxListCtrlEx::CreateVisibleColumnMapping()
{
	int pos = 0;
	for (unsigned int j = 0; j < m_columnInfo.size(); j++)
	{
		for (unsigned int i = 0; i < m_columnInfo.size(); i++)
		{
			const t_columnInfo &column = m_columnInfo[i];

			if (!column.shown)
				continue;

			if (column.order != j)
				continue;

			m_pVisibleColumnMapping[pos] = i;
			InsertColumn(pos++, column.name, column.align, column.width);
		}
	}
}

class CColumnEditDialog : public wxDialogEx
{
public:
	int *m_order;
	DECLARE_EVENT_TABLE()

protected:
	void OnUp(wxCommandEvent& event)
	{
		wxCheckListBox* pListBox = XRCCTRL(*this, "ID_ACTIVE", wxCheckListBox);
		int sel = pListBox->GetSelection();
		if (sel < 2)
			return;

		int tmp;
		tmp = m_order[sel - 1];
		m_order[sel - 1] = m_order[sel];
		m_order[sel] = tmp;

		wxString name = pListBox->GetString(sel);
		bool checked = pListBox->IsChecked(sel);
		pListBox->Delete(sel);
		pListBox->Insert(name, sel - 1);
		pListBox->Check(sel - 1, checked);
		pListBox->SetSelection(sel - 1);

		wxCommandEvent evt;
		OnSelChanged(evt);
	}

	void OnDown(wxCommandEvent& event)
	{
		wxCheckListBox* pListBox = XRCCTRL(*this, "ID_ACTIVE", wxCheckListBox);
		int sel = pListBox->GetSelection();
		if (sel < 1)
			return;
		if (sel >= (int)pListBox->GetCount() - 1)
			return;

		int tmp;
		tmp = m_order[sel + 1];
		m_order[sel + 1] = m_order[sel];
		m_order[sel] = tmp;

		wxString name = pListBox->GetString(sel);
		bool checked = pListBox->IsChecked(sel);
		pListBox->Delete(sel);
		pListBox->Insert(name, sel + 1);
		pListBox->Check(sel + 1, checked);
		pListBox->SetSelection(sel + 1);

		wxCommandEvent evt;
		OnSelChanged(evt);
	}

	void OnSelChanged(wxCommandEvent& event)
	{
		wxCheckListBox* pListBox = XRCCTRL(*this, "ID_ACTIVE", wxCheckListBox);
		int sel = pListBox->GetSelection();
		XRCCTRL(*this, "ID_UP", wxButton)->Enable(sel > 1);
		XRCCTRL(*this, "ID_DOWN", wxButton)->Enable(sel > 0 && sel < (int)pListBox->GetCount() - 1);
	}

	void OnCheck(wxCommandEvent& event)
	{
		if (!event.GetSelection() && !event.IsChecked())
		{
			wxCheckListBox* pListBox = XRCCTRL(*this, "ID_ACTIVE", wxCheckListBox);
			pListBox->Check(0);
			wxMessageBox(_("The filename column can neither be hidden nor moved."), _("Column properties"));
		}
	}
};

BEGIN_EVENT_TABLE(CColumnEditDialog, wxDialogEx)
EVT_BUTTON(XRCID("ID_UP"), CColumnEditDialog::OnUp)
EVT_BUTTON(XRCID("ID_DOWN"), CColumnEditDialog::OnDown)
EVT_LISTBOX(wxID_ANY, CColumnEditDialog::OnSelChanged)
EVT_CHECKLISTBOX(wxID_ANY, CColumnEditDialog::OnCheck)
END_EVENT_TABLE()

void wxListCtrlEx::ShowColumnEditor()
{
	CColumnEditDialog dlg;

	dlg.Load(this, _T("ID_COLUMN_SETUP"));

	wxCheckListBox* pListBox = XRCCTRL(dlg, "ID_ACTIVE", wxCheckListBox);

	dlg.m_order = new int[m_columnInfo.size()];
	for (unsigned int j = 0; j < m_columnInfo.size(); j++)
	{
		for (unsigned int i = 0; i < m_columnInfo.size(); i++)
		{
			if (m_columnInfo[i].order != j)
				continue;
			dlg.m_order[j] = i;
			pListBox->Append(m_columnInfo[i].name);
			if (m_columnInfo[i].shown)
				pListBox->Check(j);
		}
	}
	wxASSERT(pListBox->GetCount() == m_columnInfo.size());

	dlg.GetSizer()->Fit(&dlg);

	if (dlg.ShowModal() != wxID_OK)
	{
		delete [] dlg.m_order;
		return;
	}

	for (unsigned int i = 0; i < m_columnInfo.size(); i++)
	{
		int col = dlg.m_order[i];
		bool isChecked = pListBox->IsChecked(i);
		if (!isChecked && !col)
		{
			isChecked = true;
			wxMessageBox(_("The filename column cannot be hidden."));
		}
		MoveColumn(col, i);
		if (m_columnInfo[col].shown != isChecked)
			ShowColumn(col, isChecked);
	}

	delete [] dlg.m_order;

	// Generic wxListCtrl needs manual refresh
	Refresh();
}

int wxListCtrlEx::GetColumnVisibleIndex(int col)
{
	if (!m_pVisibleColumnMapping)
		return -1;

	for (int i = 0; i < GetColumnCount(); i++)
	{
		if (m_pVisibleColumnMapping[i] == (unsigned int)col)
			return i;
	}

	return -1;
}

int wxListCtrlEx::GetHeaderSortIconIndex(int col)
{
	if (col < 0 || col >= GetColumnCount())
		return -1;

#ifdef __WXMSW__
	HWND hWnd = (HWND)GetHandle();
	HWND header = (HWND)SendMessage(hWnd, LVM_GETHEADER, 0, 0);

	HDITEM item;
	item.mask = HDI_IMAGE | HDI_FORMAT;
	SendMessage(header, HDM_GETITEM, col, (LPARAM)&item);

	if (!(item.fmt & HDF_IMAGE))
		return -1;

	return item.iImage;
#else
	wxListItem item;
	if (!GetColumn(col, item))
		return -1;

	return item.GetImage();
#endif
}

void wxListCtrlEx::InitHeaderSortImageList()
{
#ifdef __WXMSW__
	if (wxPlatformInfo::Get().GetOSMajorVersion() >= 6)
		return;

	// Initialize imagelist for list header
	m_pHeaderImageList = new wxImageListEx(8, 8, true, 3);

	wxBitmap bmp;

	bmp.LoadFile(wxGetApp().GetResourceDir() + _T("up.png"), wxBITMAP_TYPE_PNG);
	m_pHeaderImageList->Add(bmp);
	bmp.LoadFile(wxGetApp().GetResourceDir() + _T("down.png"), wxBITMAP_TYPE_PNG);
	m_pHeaderImageList->Add(bmp);

	HWND hWnd = (HWND)GetHandle();
	if (!hWnd)
	{
		delete m_pHeaderImageList;
		m_pHeaderImageList = 0;
		return;
	}

	HWND header = (HWND)SendMessage(hWnd, LVM_GETHEADER, 0, 0);
	if (!header)
	{
		delete m_pHeaderImageList;
		m_pHeaderImageList = 0;
		return;
	}

	TCHAR buffer[1000] = {0};
	HDITEM item;
	item.mask = HDI_TEXT;
	item.pszText = buffer;
	item.cchTextMax = 999;
	SendMessage(header, HDM_GETITEM, 0, (LPARAM)&item);

	SendMessage(header, HDM_SETIMAGELIST, 0, (LPARAM)m_pHeaderImageList->GetHandle());

	m_header_icon_index.up = 0;
	m_header_icon_index.down = 1;
#else

	wxColour colour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);

	wxString lightness;
	if (colour.Red() + colour.Green() + colour.Blue() > 3 * 128)
		lightness = _T("DARK");
	else
		lightness = _T("LIGHT");

	wxBitmap bmp;

	bmp = wxArtProvider::GetBitmap(_T("ART_SORT_UP_") + lightness, wxART_OTHER, CThemeProvider::GetIconSize(iconSizeSmall));
	m_header_icon_index.up = m_pImageList->Add(bmp);
	bmp = wxArtProvider::GetBitmap(_T("ART_SORT_DOWN_") + lightness, wxART_OTHER, CThemeProvider::GetIconSize(iconSizeSmall));
	m_header_icon_index.down = m_pImageList->Add(bmp);
#endif
}

void wxListCtrlEx::SetHeaderSortIconIndex(int col, int icon)
{
	if (col < 0 || col >= GetColumnCount())
		return;

#ifdef __WXMSW__
	HWND hWnd = (HWND)GetHandle();
	HWND header = (HWND)SendMessage(hWnd, LVM_GETHEADER, 0, 0);

	wxChar buffer[100];
	HDITEM item = {0};
	item.mask = HDI_TEXT | HDI_FORMAT;
	item.pszText = buffer;
	item.cchTextMax = 99;
	SendMessage(header, HDM_GETITEM, col, (LPARAM)&item);
	if (icon != -1)
	{
		if (wxPlatformInfo::Get().GetOSMajorVersion() >= 6)
		{
			item.fmt &= ~(HDF_IMAGE | HDF_BITMAP_ON_RIGHT | HDF_SORTUP | HDF_SORTDOWN);
			item.iImage = -1;
			if (icon)
				item.fmt |= HDF_SORTDOWN;
			else
				item.fmt |= HDF_SORTUP;
		}
		else
		{
			item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
			item.fmt |= HDF_IMAGE | HDF_BITMAP_ON_RIGHT;
			item.iImage = icon ? m_header_icon_index.down : m_header_icon_index.up;
			item.mask |= HDI_IMAGE;
		}
	}
	else
	{
		item.fmt &= ~(HDF_IMAGE | HDF_BITMAP_ON_RIGHT | HDF_SORTUP | HDF_SORTDOWN);
		item.iImage = -1;
	}
	SendMessage(header, HDM_SETITEM, col, (LPARAM)&item);
#else
	wxListItem item;
	if (!GetColumn(col, item))
		return;

	if( icon != -1 )
		icon = icon ? m_header_icon_index.down : m_header_icon_index.up;

	item.SetImage(icon);
	SetColumn(col, item);
#endif
}

void wxListCtrlEx::RefreshListOnly(bool eraseBackground /*=true*/)
{
	// See comment in wxGenericListCtrl::Refresh
#ifdef __WXMSW__
	Refresh(eraseBackground);
#else
	GetMainWindow()->Refresh(eraseBackground);
#endif
}

void wxListCtrlEx::CancelLabelEdit()
{
#ifdef __WXMSW__
	if (GetEditControl())
		ListView_CancelEditLabel((HWND)GetHandle());
#else
	m_editing = false;
	wxTextCtrl* pEdit = GetEditControl();
	if (pEdit)
	{
		wxKeyEvent evt(wxEVT_CHAR);
		evt.m_keyCode = WXK_ESCAPE;
		pEdit->GetEventHandler()->ProcessEvent(evt);
	}
#endif

}

void wxListCtrlEx::OnBeginLabelEdit(wxListEvent& event)
{
#ifndef __WXMSW__
	if (m_editing)
	{
		event.Veto();
		return;
	}
#endif
	if (m_blockedLabelEditing)
	{
		event.Veto();
		return;
	}

	if (!OnBeginRename(event))
		event.Veto();
#ifndef __WXMSW__
	else
		m_editing = true;
#endif
}

void wxListCtrlEx::OnEndLabelEdit(wxListEvent& event)
{
#ifdef __WXMAC__
	int item = event.GetIndex();
	if (item != -1)
	{
		int to = item + 1;
		if (to < GetItemCount())
		{
			int from = item;
			if (from)
				from--;
			RefreshItems(from, to);
		}
		else
			RefreshListOnly();
	}
#endif

#ifndef __WXMSW__
	if (!m_editing)
	{
		event.Veto();
		return;
	}
	m_editing = false;
#endif

	if (event.IsEditCancelled())
		return;

	if (!OnAcceptRename(event))
		event.Veto();
}

bool wxListCtrlEx::OnBeginRename(const wxListEvent& event)
{
	return false;
}

bool wxListCtrlEx::OnAcceptRename(const wxListEvent& event)
{
	return false;
}

void wxListCtrlEx::SetLabelEditBlock(bool block)
{
	if (block)
	{
		CancelLabelEdit();
		++m_blockedLabelEditing;
	}
	else
	{
		wxASSERT(m_blockedLabelEditing);
		if (m_blockedLabelEditing > 0)
			m_blockedLabelEditing--;
	}
}

CLabelEditBlocker::CLabelEditBlocker(wxListCtrlEx& listCtrl)
	: m_listCtrl(listCtrl)
{
	m_listCtrl.SetLabelEditBlock(true);
}

CLabelEditBlocker::~CLabelEditBlocker()
{
	m_listCtrl.SetLabelEditBlock(false);
}

void wxListCtrlEx::OnColumnDragging(wxListEvent& event)
{
	if (event.GetItem().GetWidth() < MIN_COLUMN_WIDTH)
		event.Veto();
}

#ifdef __WXMSW__
bool wxListCtrlEx::MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM *result)
{
	// MSW doesn't generate HDN_TRACK on all header styles, so handle it
	// ourselves using HDN_ITEMCHANGING.
	NMHDR *nmhdr = (NMHDR *)lParam;
	HWND hwndHdr = ListView_GetHeader((HWND)GetHandle());

    if (nmhdr->hwndFrom != hwndHdr)
		return wxListCtrl::MSWOnNotify(idCtrl, lParam, result);

	HD_NOTIFY *nmHDR = (HD_NOTIFY *)nmhdr;

	switch ( nmhdr->code )
	{
		// See comment in src/msw/listctrl.cpp of wx why both A and W are needed
	case HDN_BEGINTRACKA:
	case HDN_BEGINTRACKW:
		m_columnDragging = true;
		break;
	case HDN_ENDTRACKA:
	case HDN_ENDTRACKW:
		m_columnDragging = true;
		break;
	case HDN_ITEMCHANGINGA:
	case HDN_ITEMCHANGINGW:
		if (m_columnDragging)
		{
			if (nmHDR->pitem->mask & HDI_WIDTH && nmHDR->pitem->cxy < 10)
				*result = 1;
			else
				*result = 0;
		}
		return true;
	}

	return wxListCtrl::MSWOnNotify(idCtrl, lParam, result);
}
#endif

bool wxListCtrlEx::HasSelection() const
{
#ifndef __WXMSW__
	// GetNextItem is O(n) if nothing is selected, GetSelectedItemCount() is O(1)
	return GetSelectedItemCount() != 0;
#else
	return GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) != -1;
#endif
}
