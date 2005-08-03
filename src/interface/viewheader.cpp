#include "FileZilla.h"
#include "viewheader.h"
#include "state.h"
#include "commandqueue.h"

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
EVT_PAINT(CViewHeader::OnPaint)
END_EVENT_TABLE()

CViewHeader::CViewHeader(wxWindow* pParent, const wxString& label)
	: wxWindow(pParent, wxID_ANY)
{
	m_alreadyInPaint = false;
	m_pComboBox = new CComboBoxEx(this);
	wxSize size = GetSize();
#ifdef __WXMSW__
	size.SetHeight(m_pComboBox->GetBestSize().GetHeight());
#else
	size.SetHeight(m_pComboBox->GetBestSize().GetHeight() + 10);
#endif
	SetSize(size);

#ifdef __WXMSW__
	m_pComboBox->Connect(wxID_ANY, wxEVT_PAINT, (wxObjectEventFunction)(wxEventFunction)(wxPaintEventFunction)&CViewHeader::OnComboPaint, 0, this);
	m_pComboBox->Connect(wxID_ANY, wxEVT_LEFT_DOWN, (wxObjectEventFunction)(wxEventFunction)(wxMouseEventFunction)&CViewHeader::OnComboMouseEvent, 0, this);
	m_pComboBox->Connect(wxID_ANY, wxEVT_LEFT_UP, (wxObjectEventFunction)(wxEventFunction)(wxMouseEventFunction)&CViewHeader::OnComboMouseEvent, 0, this);
	m_bLeftMousePressed = false;
#endif //__WXMSW__
	SetLabel(label);
}

void CViewHeader::OnSize(wxSizeEvent& event)
{
	wxRect rect = GetClientRect();
	
	rect.SetWidth(rect.GetWidth() - m_cbOffset + 2);
	rect.SetX(m_cbOffset);
#ifndef __WXMSW__
	rect.Deflate(0, 5);
	rect.SetWidth(rect.GetWidth() - 5);
#endif
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

#endif //__WXMSW__

void CViewHeader::OnPaint(wxPaintEvent& event)
{
	wxRect rect = GetClientRect();
	wxPaintDC dc(this);
	dc.SetPen(*wxBLACK_PEN);
	
#ifdef __WXMSW__
	dc.DrawLine(rect.GetLeft(), rect.GetBottom(), m_cbOffset, rect.GetBottom());
#else
	dc.DrawLine(rect.GetLeft(), rect.GetBottom(), rect.GetRight(), rect.GetBottom());
#endif

	dc.SetFont(GetFont());
	dc.DrawText(m_label, 5, (rect.GetHeight() - m_labelHeight) / 2 - 1);
}

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

void CViewHeader::CopyDataFrom(CViewHeader* pHeader)
{
	m_label = pHeader->m_label;
	m_cbOffset = pHeader->m_cbOffset;
	m_labelHeight = pHeader->m_labelHeight;
}

BEGIN_EVENT_TABLE(CLocalViewHeader, CViewHeader)
EVT_TEXT(wxID_ANY, CLocalViewHeader::OnTextChanged)
EVT_TEXT_ENTER(wxID_ANY, CLocalViewHeader::OnTextEnter)
END_EVENT_TABLE()

CLocalViewHeader::CLocalViewHeader(wxWindow* pParent, CState* pState)
	: CViewHeader(pParent, _("Local site:"))
{
	m_pState = pState;
	m_pState->SetLocalViewHeader(this);
	SetDir(m_pState->GetLocalDir());
}

void CLocalViewHeader::OnTextChanged(wxCommandEvent& event)
{
	wxString str = m_pComboBox->GetValue();
	if (str == _T("") || str.Right(1) == _T("/"))
	{
		m_oldValue = str;
		return;
	}
#ifdef __WXMSW__
	if (str.Right(1) == _T("\\"))
	{
		m_oldValue = str;
		return;
	}
#endif

	if (str == m_oldValue)
		return;

	if (str.Left(m_oldValue.Length()) != m_oldValue)
	{
		m_oldValue = str;
		return;
	}

	wxFileName fn(str);
	if (!fn.IsOk())
	{
		m_oldValue = str;
		return;
	}
	wxDir dir(fn.GetPath());
	wxString name = fn.GetFullName();
	if (!dir.IsOpened() || name == _T(""))
	{
		m_oldValue = str;
		return;
	}
	wxString found;
	if (!dir.GetFirst(&found, name + _T("*"), wxDIR_DIRS))
	{
		m_oldValue = str;
		return;
	}

	wxString tmp;
	if (dir.GetNext(&tmp))
	{
		m_oldValue = str;
		return;
	}

#ifdef __WXMSW__
	if (found.Left(name.Length()).CmpNoCase(name))
#else
	if (found.Left(name.Length()) != name)
#endif
	{
		m_oldValue = str;
		return;
	}

#ifdef __WXMSW__
	m_pComboBox->SetValue(str + found.Mid(name.Length()) + _T("\\"));
#else
	m_pComboBox->SetValue(str + found.Mid(name.Length()) + _T("/"));
#endif
	m_pComboBox->SetSelection(str.Length(), m_pComboBox->GetValue().Length() + 1);

	m_oldValue = str;
}

void CLocalViewHeader::OnTextEnter(wxCommandEvent& event)
{
	wxString dir = m_pComboBox->GetValue();
	if (!wxDir::Exists(dir))
	{
		wxBell();
		return;
	}
	
	m_pState->SetLocalDir(dir);
}

void CLocalViewHeader::SetDir(wxString dir)
{
	m_pComboBox->SetValue(dir);
	m_pComboBox->SetSelection(m_pComboBox->GetValue().Length(), m_pComboBox->GetValue().Length());
}

BEGIN_EVENT_TABLE(CRemoteViewHeader, CViewHeader)
EVT_TEXT_ENTER(wxID_ANY, CRemoteViewHeader::OnTextEnter)
END_EVENT_TABLE()

CRemoteViewHeader::CRemoteViewHeader(wxWindow* pParent, CState* pState, CCommandQueue* pCommandQueue)
	: CViewHeader(pParent, _("Remote site:"))
{
	m_pCommandQueue = pCommandQueue;
	m_pState = pState;
	m_pState->SetRemoteViewHeader(this);
}

void CRemoteViewHeader::SetDir(const CServerPath& path)
{
	m_path = path;
	if (path.IsEmpty())
	{
		m_pComboBox->SetValue(_T(""));
		m_pComboBox->Disable();
	}
	else
	{
		m_pComboBox->Enable();
		m_pComboBox->SetValue(path.GetPath());
		m_pComboBox->SetSelection(m_pComboBox->GetValue().Length(), m_pComboBox->GetValue().Length());
	}
}

void CRemoteViewHeader::OnTextEnter(wxCommandEvent& event)
{
	CServerPath path = m_path;
	wxString value = m_pComboBox->GetValue();
	if (value == _T("") || !path.ChangePath(value))
	{
		wxBell();
		return;
	}

	if (!m_pCommandQueue->Idle())
	{
		wxBell();
		return;
	}

	m_pCommandQueue->ProcessCommand(new CListCommand(path));
}
