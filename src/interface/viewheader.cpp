#include "FileZilla.h"
#include "viewheader.h"

#ifdef __WXMSW__
#include "wx/msw/uxtheme.h"
#endif //__WXMSW__

// wxComboBox derived class which captures WM_CANCELMODE under Windows
class CComboBoxEx : public wxComboBox
{
public:
	CComboBoxEx(CViewHeader* parent)
		: wxComboBox(parent, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, wxArrayString(), wxCB_DROPDOWN)
	{
		m_parent = parent;
	}
#ifdef __WXMSW__
protected:
	CViewHeader* m_parent;
	virtual WXLRESULT MSWDefWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
	{
		if (nMsg == WM_CANCELMODE)
		{
			m_parent->m_bLeftMousePressed = false;
			Refresh();
		}
		else if (nMsg == WM_CAPTURECHANGED && !lParam)
		{
			WXLRESULT res = wxComboBox::MSWDefWindowProc(nMsg, wParam, lParam);

			if (!SendMessage((HWND)GetHWND(), CB_GETDROPPEDSTATE, 0, 0))
			{
				m_parent->m_bLeftMousePressed = false;
				Refresh();
			}
			return res;
		}
		return wxComboBox::MSWDefWindowProc(nMsg, wParam, lParam);
	}
#endif //__WXMSW__
};

BEGIN_EVENT_TABLE(CViewHeader, wxWindow)
EVT_SIZE(CViewHeader::OnSize)
#ifdef __WXMSW__
EVT_PAINT(CViewHeader::OnPaint)
#endif //__WXMSW__
END_EVENT_TABLE()

CViewHeader::CViewHeader(wxWindow* pParent, const wxString& label)
	: wxWindow(pParent, wxID_ANY)
{
	m_alreadyInPaint = false;
	m_pComboBox = new CComboBoxEx(this);
	wxSize size = GetSize();
	size.SetHeight(m_pComboBox->GetSize().GetHeight());
	SetSize(size);

#ifdef __WXMSW__
	m_pComboBox->Connect(wxID_ANY, wxEVT_PAINT, (wxObjectEventFunction)(wxEventFunction)&CViewHeader::OnComboPaint, 0, this);
	m_pComboBox->Connect(wxID_ANY, wxEVT_LEFT_DOWN, (wxObjectEventFunction)(wxEventFunction)&CViewHeader::OnComboMouseEvent, 0, this);
	m_pComboBox->Connect(wxID_ANY, wxEVT_LEFT_UP, (wxObjectEventFunction)(wxEventFunction)&CViewHeader::OnComboMouseEvent, 0, this);
	m_bLeftMousePressed = false;
#endif //__WXMSW__
	SetLabel(label);
}

void CViewHeader::OnSize(wxSizeEvent& event)
{
	wxRect rect = GetClientRect();
	rect.SetWidth(rect.GetWidth() - m_cbOffset + 2);
	rect.SetX(m_cbOffset);
	m_pComboBox->SetSize(rect);
	Refresh();
}

#ifdef __WXMSW__

void CViewHeader::OnComboPaint(wxPaintEvent& event)
{
	// We do a small trick to let the control handle the event before we can paint
	if (m_alreadyInPaint)
	{
		event.Skip();
		return;
	}
	
	wxComboBox* box = m_pComboBox;

	m_alreadyInPaint = true;
	box->Refresh();
	box->Update();
	m_alreadyInPaint = false;

	wxClientDC dc(box);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);

	int thumbWidth = ::GetSystemMetrics(SM_CXHTHUMB);

	if (m_bLeftMousePressed)
	{
		if (!SendMessage((HWND)box->GetHWND(), CB_GETDROPPEDSTATE, 0, 0))
			m_bLeftMousePressed = false;
	}
	
#if wxUSE_UXTHEME
	wxUxThemeEngine *p = wxUxThemeEngine::Get();
	if (p && p->IsThemeActive())
	{
	}
	else
#endif //wxUSE_UXTHEME
	{
		dc.SetPen(wxPen(wxSystemSettings::GetColour(box->IsEnabled() ? wxSYS_COLOUR_WINDOW : wxSYS_COLOUR_BTNFACE)));
		wxRect rect = box->GetClientRect();
		rect.Deflate(1);
		wxRect rect2 = rect;
		rect2.SetWidth(rect.GetWidth() - thumbWidth);
		dc.DrawRectangle(rect2);
	
		if (!m_bLeftMousePressed || !box->IsEnabled())
		{
			wxPoint topLeft = rect.GetTopLeft();
			wxPoint bottomRight = rect.GetBottomRight();
			bottomRight.x--;
			topLeft.x = bottomRight.x - thumbWidth + 1;

			dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
			dc.DrawLine(topLeft.x, topLeft.y, bottomRight.x + 1, topLeft.y);
			dc.DrawLine(topLeft.x, topLeft.y + 1, topLeft.x, bottomRight.y);
			dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DDKSHADOW)));
			dc.DrawLine(bottomRight.x, topLeft.y + 1, bottomRight.x, bottomRight.y + 1);
			dc.DrawLine(topLeft.x, bottomRight.y, bottomRight.x, bottomRight.y);

			topLeft.x++;
			topLeft.y++;
			bottomRight.x--;
			bottomRight.y--;
			dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT)));
			dc.DrawLine(topLeft.x, topLeft.y, bottomRight.x + 1, topLeft.y);
			dc.DrawLine(topLeft.x, topLeft.y + 1, topLeft.x, bottomRight.y);
			dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
			dc.DrawLine(bottomRight.x, topLeft.y + 1, bottomRight.x, bottomRight.y + 1);
			dc.DrawLine(topLeft.x, bottomRight.y, bottomRight.x, bottomRight.y);

			topLeft.x++;
			topLeft.y++;
			bottomRight.x--;
			bottomRight.y--;
			dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
			dc.DrawRectangle(wxRect(topLeft, bottomRight));
		}
		else
		{
			wxPoint topLeft = rect.GetTopLeft();
			wxPoint bottomRight = rect.GetBottomRight();
			bottomRight.x--;
			topLeft.x = bottomRight.x - thumbWidth + 1;

			dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
			dc.DrawRectangle(wxRect(topLeft, bottomRight));

			topLeft.x++;
			topLeft.y++;
			bottomRight.x--;
			bottomRight.y--;
			dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
			dc.DrawRectangle(wxRect(topLeft, bottomRight));
		}
	}

	// Cover up dark 3D shadow.
	wxRect rect = box->GetClientRect();
	dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DDKSHADOW)));
	dc.DrawRectangle(rect);

}

void CViewHeader::OnComboMouseEvent(wxMouseEvent& event)
{
	if (event.GetEventType() == wxEVT_LEFT_UP)
		m_bLeftMousePressed = false;
	else if (event.GetEventType() == wxEVT_LEFT_DOWN)
		m_bLeftMousePressed = true;

	event.Skip();
}

void CViewHeader::OnPaint(wxPaintEvent& event)
{
	wxRect rect = GetClientRect();
	wxPaintDC dc(this);
	dc.SetPen(*wxBLACK_PEN);
	dc.DrawLine(rect.GetLeft(), rect.GetBottom(), m_cbOffset, rect.GetBottom());

	dc.SetFont(GetFont());
	dc.DrawText(m_label, 5, (rect.GetHeight() - m_labelHeight) / 2 - 1);
}

#endif //__WXMSW__

void CViewHeader::SetLabel(const wxString& label)
{
	m_label = label;
	int w;
	GetTextExtent(label, &w, &m_labelHeight);
	m_cbOffset = w + 10;
}

void CViewHeader::Reparent(CViewHeader** pViewHeader, wxWindow* parent)
{
#if defined __WXMSW__ || defined __WXGTK__
	((wxWindow*)*pViewHeader)->Reparent(parent);
#else
#error CViewHeader::Reparent unimplemented
#endif
}
