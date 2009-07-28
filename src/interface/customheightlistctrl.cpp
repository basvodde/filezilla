#include "FileZilla.h"
#include "customheightlistctrl.h"

BEGIN_EVENT_TABLE(wxCustomHeightListCtrl, wxScrolledWindow)
EVT_MOUSE_EVENTS(wxCustomHeightListCtrl::OnMouseEvent)
END_EVENT_TABLE();

wxCustomHeightListCtrl::wxCustomHeightListCtrl(wxWindow* parent, wxWindowID id /*=-1*/, const wxPoint& pos /*=wxDefaultPosition*/, const wxSize& size /*=wxDefaultSize*/, long style /*=wxHSCROLL|wxVSCROLL*/, const wxString& name /*=_T("scrolledWindow")*/)
	: wxScrolledWindow(parent, id, pos, size, style, name)
{
	m_lineHeight = 20;
	m_lineCount = 0;
	m_focusedLine = -1;

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
}

void wxCustomHeightListCtrl::SetLineHeight(int height)
{
	m_lineHeight = height;

	int posx, posy;
	GetViewStart(&posx, &posy);
	SetScrollbars(0, m_lineHeight, 0, m_lineCount, 0, posy);

	Refresh();
}

void wxCustomHeightListCtrl::SetLineCount(int count)
{
	if (count < m_lineCount)
	{
		std::set<int> selectedLines = m_selectedLines;
		m_selectedLines.clear();
		for (std::set<int>::const_iterator iter = selectedLines.begin(); iter != selectedLines.end(); iter++)
			if (*iter < count)
				m_selectedLines.insert(*iter);
	}
	m_lineCount = count;
	if (m_focusedLine >= count)
		m_focusedLine = -1;

	int posx, posy;
	GetViewStart(&posx, &posy);

#ifdef __WXGTK__
	// When decreasing scrollbar range, wxGTK does not seem to adjust child position
	// if viewport gets moved
	wxPoint old_view;
	GetViewStart(&old_view.x, &old_view.y);
#endif

	SetScrollbars(0, m_lineHeight, 0, m_lineCount, 0, posy);

#ifdef __WXGTK__
	wxPoint new_view;
	GetViewStart(&new_view.x, &new_view.y);
	int delta_y = m_lineHeight *(old_view.y - new_view.y);

	if (delta_y)
	{
		wxWindowList::compatibility_iterator iter = GetChildren().GetFirst();
		while (iter)
		{
			wxWindow* child = iter->GetData();
			wxPoint pos = child->GetPosition();
			pos.y -= delta_y;
			child->SetPosition(pos);

			iter = iter->GetNext();
		}
	}
#endif

	Refresh();
}

void wxCustomHeightListCtrl::SetFocus()
{
	wxWindow::SetFocus();
}

void wxCustomHeightListCtrl::OnDraw(wxDC& dc)
{
	wxSize size = GetClientSize();

	dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT)));
	dc.SetPen(*wxTRANSPARENT_PEN);

	for (std::set<int>::const_iterator iter = m_selectedLines.begin(); iter != m_selectedLines.end(); iter++)
	{
		if (*iter == m_focusedLine)
			dc.SetPen(wxPen(wxColour(0, 0, 0), 1, wxDOT));
		else
			dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(0, m_lineHeight * *iter, size.GetWidth(), m_lineHeight);
	}
	if (m_focusedLine != -1 && m_selectedLines.find(m_focusedLine) == m_selectedLines.end())
	{
		dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
		dc.SetPen(wxPen(wxColour(0, 0, 0), 1, wxDOT));
		dc.DrawRectangle(0, m_lineHeight * m_focusedLine, size.GetWidth(), m_lineHeight);
	}
}

void wxCustomHeightListCtrl::OnMouseEvent(wxMouseEvent& event)
{
	bool changed = false;
	if (event.ButtonDown())
	{
		wxPoint pos = event.GetPosition();
		int x, y;
		CalcUnscrolledPosition(pos.x, pos.y, &x, &y);
		if (y > m_lineHeight * m_lineCount)
		{
			m_focusedLine = -1;
			m_selectedLines.clear();
			changed = true;
		}
		else
		{
			int line = y / m_lineHeight;
			if (event.ShiftDown())
			{
				if (line < m_focusedLine)
				{
					for (int i = line; i <= m_focusedLine; i++)
					{
						if (m_selectedLines.find(i) == m_selectedLines.end())
						{
							changed = true;
							m_selectedLines.insert(i);
						}
					}
				}
				else
				{
					for (int i = line; i >= m_focusedLine; i--)
					{
						if (m_selectedLines.find(i) == m_selectedLines.end())
						{
							changed = true;
							m_selectedLines.insert(i);
						}
					}
				}
			}
			else if (event.ControlDown())
			{
				if (m_selectedLines.find(line) == m_selectedLines.end())
					m_selectedLines.insert(line);
				else
					m_selectedLines.erase(line);
				changed = true;
			}
			else
			{
				m_selectedLines.clear();
				m_selectedLines.insert(line);
				changed = true;
			}

			m_focusedLine = line;				

		}
		Refresh();
	}

	event.Skip();

	if (changed)
	{
		wxCommandEvent evt(wxEVT_COMMAND_LISTBOX_SELECTED, GetId());
		ProcessEvent(evt);
	}
}

void wxCustomHeightListCtrl::ClearSelection()
{
	m_selectedLines.clear();
	m_focusedLine = -1;

	Refresh();
}

std::set<int> wxCustomHeightListCtrl::GetSelection() const
{
	return m_selectedLines;
}

void wxCustomHeightListCtrl::SelectLine(int line)
{
	m_selectedLines.clear();
	m_focusedLine = line;
	if (line != -1)
		m_selectedLines.insert(line);

	Refresh();
}
