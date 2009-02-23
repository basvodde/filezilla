#include "FileZilla.h"
#include <wx/aui/aui.h>
#include "aui_notebook_ex.h"
#include <wx/dcmirror.h>

wxColor wxAuiStepColour(const wxColor& c, int ialpha);

// Special DC to filter some calls.
// We can (mis)use wxMirrorDC since it already
// forwards all calls to another DC.
//
// We use this class to change the colours wxAuiTabArt uses
// for drawing the tabs on dark themes.
class CFilterDC : public wxMirrorDC
{
public:
	CFilterDC(wxDC& dc, int type)
		: wxMirrorDC(dc, false), m_original_dc(&dc), m_type(type)
	{
		m_gradient_called = 0;
		m_rectangle_called = 0;
	}

	virtual void DoDrawRectangle(wxCoord x, wxCoord y, wxCoord width, wxCoord height)
	{
		if (!m_type)
		{
			m_original_dc->DrawRectangle(x, y, width, height);
			return;
		}
		else if (m_type == 1)
		{
			m_rectangle_called++;

			wxColour c = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME);
			if (c.Red() + c.Green() + c.Blue() >= 384)
				m_original_dc->DrawRectangle(x, y, width, height);
			else
			{
				if (m_rectangle_called != 2)
					m_original_dc->DrawRectangle(x, y, width, height);
				else
				{
					wxColour new_init = wxAuiStepColour(c, 150);
					wxColour new_dest = wxAuiStepColour(c, 100);
					m_original_dc->GradientFillLinear(wxRect(x, y, width + 1, height), new_init, new_dest, wxNORTH);
				}
			}
		}
		else
		{
			wxColour c = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME);
			if (c.Red() + c.Green() + c.Blue() < 384)
				m_original_dc->SetBrush(wxBrush(wxAuiStepColour(c, 70)));
			m_original_dc->DrawRectangle(x, y, width, height);
		}
	}
	virtual void DoGradientFillLinear(const wxRect& rect, const wxColour& initialColour, const wxColour& destColour, wxDirection nDirection = wxEAST)
	{
		m_gradient_called++;

		if (m_type == 1)
		{
			if (initialColour.Red() + initialColour.Green() + initialColour.Blue() >= 384)
			{
				// Light theme
				m_original_dc->GradientFillLinear(rect, initialColour, destColour, nDirection);
				return;
			}

			// Dark theme
			wxColour new_init = wxAuiStepColour(initialColour, 100);
			wxColour new_dest = wxAuiStepColour(initialColour, 120);
			m_original_dc->GradientFillLinear(rect, new_init, new_dest, nDirection);
		}
		else if (!m_type)
		{
			if (destColour.Red() + destColour.Green() + destColour.Blue() >= 384)
			{
				// Light theme
				m_original_dc->GradientFillLinear(rect, initialColour, destColour, nDirection);
				return;
			}

			// Dark theme
			if (m_gradient_called == 1)
			{
				wxColour new_init = wxAuiStepColour(destColour, 95);
				wxColour new_dest = wxAuiStepColour(destColour, 65);
				m_original_dc->GradientFillLinear(rect, new_init, new_dest, nDirection);
			}
			else
			{
				wxColour new_dest = wxAuiStepColour(destColour, 65);
				m_original_dc->GradientFillLinear(rect, new_dest, new_dest, nDirection);
			}
		}
		else if (m_type == 2)
		{
			wxColour c = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME);
			if (c.Red() + c.Green() + c.Blue() >= 384)
				m_original_dc->GradientFillLinear(rect, initialColour, destColour, nDirection);
			else
			{
				wxColour new_init = wxAuiStepColour(c, 70);
				wxColour new_dest = wxAuiStepColour(c, 100);
				m_original_dc->GradientFillLinear(rect, new_init, new_dest, nDirection);
			}
		}
	}

protected:
	int m_gradient_called;
	int m_rectangle_called;
	wxDC *m_original_dc;
	const int m_type;
};

class wxAuiTabArtEx : public wxAuiDefaultTabArt
{
public:
	wxAuiTabArtEx(wxAuiNotebookEx* pNotebook)
	{
		m_pNotebook = pNotebook;
		m_fonts_initialized = false;
	}

	virtual wxAuiTabArt* Clone()
	{
		return new wxAuiTabArtEx(m_pNotebook);
	}

	virtual wxSize GetTabSize(wxDC& dc, wxWindow* wnd, const wxString& caption, const wxBitmap& bitmap, bool active, int close_button_state, int* x_extent)
	{
		wxSize size = wxAuiDefaultTabArt::GetTabSize(dc, wnd, caption, bitmap, active, close_button_state, x_extent);

		wxString text = caption;
		int pos;
		if ((pos = caption.Find(_T(" ("))) != -1)
			text = text.Left(pos);
		std::map<wxString, int>::iterator iter = m_maxSizes.find(text);
		if (iter == m_maxSizes.end())
			m_maxSizes[text] = size.x;
		else
		{
			if (iter->second > size.x)
			{
				size.x = iter->second;
				*x_extent = size.x;
			}
			else
				iter->second = size.x;
		}

		return size;
	};


	virtual void DrawTab(wxDC& dc,
                         wxWindow* wnd,
                         const wxAuiNotebookPage& pane,
                         const wxRect& in_rect,
                         int close_button_state,
                         wxRect* out_tab_rect,
                         wxRect* out_button_rect,
                         int* x_extent)
	{
#ifndef __WXMAC__
		m_base_colour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME);
#endif
		if (!pane.active)
		{
			dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
			if (m_pNotebook->Highlighted(m_pNotebook->GetPageIndex(pane.window)))
			{
				if (!m_fonts_initialized)
				{
					m_fonts_initialized = true;
					m_original_normal_font = m_normal_font;
					m_highlighted_font = m_normal_font;
					m_highlighted_font.SetWeight(wxFONTWEIGHT_BOLD);
					m_highlighted_font.SetStyle(wxFONTSTYLE_ITALIC);
				}
				m_normal_font = m_highlighted_font;
			}
			else if (m_fonts_initialized)
				m_normal_font = m_original_normal_font;
		}

		CFilterDC filter_dc(dc, pane.active ? 1 : 0);
		wxAuiDefaultTabArt::DrawTab(*((wxDC*)&filter_dc), wnd, pane, in_rect, close_button_state, out_tab_rect, out_button_rect, x_extent);
	}

	virtual void DrawBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect)
	{
		CFilterDC filter_dc(dc, 2);
		wxAuiDefaultTabArt::DrawBackground(*((wxDC*)&filter_dc), wnd, rect);
	}
protected:
	wxAuiNotebookEx* m_pNotebook;

	static std::map<wxString, int> m_maxSizes;

	wxFont m_original_normal_font;
	wxFont m_highlighted_font;
	bool m_fonts_initialized;
};

std::map<wxString, int> wxAuiTabArtEx::m_maxSizes;

BEGIN_EVENT_TABLE(wxAuiNotebookEx, wxAuiNotebook)
EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, wxAuiNotebookEx::OnPageChanged)
END_EVENT_TABLE()

wxAuiNotebookEx::wxAuiNotebookEx()
{
}

wxAuiNotebookEx::~wxAuiNotebookEx()
{
}

void wxAuiNotebookEx::RemoveExtraBorders()
{
	wxAuiPaneInfoArray& panes = m_mgr.GetAllPanes();
	for (size_t i = 0; i < panes.Count(); i++)
	{
		panes[i].PaneBorder(false);
	}
	m_mgr.Update();
}

void wxAuiNotebookEx::SetExArtProvider()
{
	SetArtProvider(new wxAuiTabArtEx(this));
}

bool wxAuiNotebookEx::SetPageText(size_t page_idx, const wxString& text)
{
	// Basically identical to the AUI one, but not calling Update
	if (page_idx >= m_tabs.GetPageCount())
		return false;

	// update our own tab catalog
	wxAuiNotebookPage& page_info = m_tabs.GetPage(page_idx);
	page_info.caption = text;

	// update what's on screen
	wxAuiTabCtrl* ctrl;
	int ctrl_idx;
	if (FindTab(page_info.window, &ctrl, &ctrl_idx))
	{
		wxAuiNotebookPage& info = ctrl->GetPage(ctrl_idx);
		info.caption = text;
		ctrl->Refresh();
	}

	return true;
}

void wxAuiNotebookEx::Highlight(size_t page, bool highlight /*=true*/)
{
	if (GetSelection() == (int)page)
		return;

	wxASSERT(page < m_tabs.GetPageCount());
	if (page >= m_tabs.GetPageCount())
		return;

	if (page >= m_highlighted.size())
		m_highlighted.resize(page + 1, false);

	if (highlight == m_highlighted[page])
		return;

	m_highlighted[page] = highlight;

	GetActiveTabCtrl()->Refresh();
}

bool wxAuiNotebookEx::Highlighted(size_t page) const
{
	wxASSERT(page < m_tabs.GetPageCount());
	if (page >= m_highlighted.size())
		return false;

	return m_highlighted[page];
}

void wxAuiNotebookEx::OnPageChanged(wxAuiNotebookEvent& event)
{
	size_t page = (size_t)GetSelection();
	if (page >= m_highlighted.size())
		return;
	
	m_highlighted[page] = false;
}
