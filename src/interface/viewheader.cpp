#include "FileZilla.h"
#include "viewheader.h"
#include "commandqueue.h"

#ifdef __WXMSW__
#include "wx/msw/uxtheme.h"
#endif //__WXMSW__

// wxComboBox derived class which captures WM_CANCELMODE under Windows
class CComboBoxEx : public wxComboBox
{
public:
	CComboBoxEx(CViewHeader* parent)
		: wxComboBox(parent, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, wxArrayString(), wxCB_DROPDOWN | wxTE_PROCESS_ENTER | wxCB_SORT)
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

			if (!SendMessage((HWND)GetHandle(), CB_GETDROPPEDSTATE, 0, 0))
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
		if (!SendMessage((HWND)box->GetHandle(), CB_GETDROPPEDSTATE, 0, 0))
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
#if defined __WXMSW__ || defined __WXGTK__ || \
	(defined __WXMAC__ && !(defined __WXMAC_CLASSIC__))
	((wxWindow*)*pViewHeader)->Reparent(parent);
#else
#error CViewHeader::Reparent unimplemented
#endif
}

wxString CViewHeader::GetLabel() const
{
	return m_label;
}

void CViewHeader::AddRecentDirectory(const wxString &directory)
{
	// Check if directory is already in the list
	for (std::list<wxString>::const_iterator iter = m_recentDirectories.begin(); iter != m_recentDirectories.end(); iter++)
	{
		if (*iter == directory)
			return;
	}

	if (m_recentDirectories.size() == 20)
	{
		wxString dirToRemove = m_recentDirectories.front();
		m_recentDirectories.pop_front();

		int item = m_pComboBox->FindString(dirToRemove, true);
		if (item != wxNOT_FOUND)
			m_pComboBox->Delete(item);
	}

	m_recentDirectories.push_back(directory);
	m_pComboBox->Append(directory);
}

BEGIN_EVENT_TABLE(CLocalViewHeader, CViewHeader)
EVT_TEXT(wxID_ANY, CLocalViewHeader::OnTextChanged)
EVT_TEXT_ENTER(wxID_ANY, CLocalViewHeader::OnTextEnter)
EVT_COMBOBOX(wxID_ANY, CLocalViewHeader::OnSelectionChanged)
END_EVENT_TABLE()

CLocalViewHeader::CLocalViewHeader(wxWindow* pParent, CState* pState)
	: CViewHeader(pParent, _("Local site:")), CStateEventHandler(pState, STATECHANGE_LOCAL_DIR)
{
}

void CLocalViewHeader::OnTextChanged(wxCommandEvent& event)
{
	// This function handles auto-completion

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

	if (str.Left(2) == _T("\\\\"))
	{
		int pos = str.Mid(2).Find('\\');
		if (pos == -1)
		{
			// Partial UNC path, no full server yet, skip further processing
			return;
		}

		pos = str.Mid(pos + 3).Find('\\');
		if (pos == -1)
		{
			// Partial UNC path, no full share yet, skip further processing
			return;
		}
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
	
	{
		wxLogNull noLog;
		if (!dir.GetFirst(&found, name + _T("*"), wxDIR_DIRS))
		{
			m_oldValue = str;
			return;
		}
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

	m_pComboBox->SetValue(str + found.Mid(name.Length()) + wxFileName::GetPathSeparator());
	m_pComboBox->SetSelection(str.Length(), m_pComboBox->GetValue().Length() + 1);

	m_oldValue = str;
}

void CLocalViewHeader::OnSelectionChanged(wxCommandEvent& event)
{
	wxString dir = event.GetString();
	if (dir == _T(""))
		return;

	if (!wxDir::Exists(dir))
	{
		const wxString& current = m_pState->GetLocalDir();
		int item = m_pComboBox->FindString(current, true);
		if (item != wxNOT_FOUND)
			m_pComboBox->SetSelection(item);

		wxBell();
		return;
	}
	
	m_pState->SetLocalDir(dir);
}

void CLocalViewHeader::OnTextEnter(wxCommandEvent& event)
{
	wxString dir = m_pComboBox->GetValue();
	
	wxString error;
	if (!m_pState->SetLocalDir(dir, &error))
	{
		if (error != _T(""))
			wxMessageBox(error, _("Failed to change directory"), wxICON_INFORMATION);
		else
			wxBell();
		m_pComboBox->SetValue(m_pState->GetLocalDir());
	}
}

void CLocalViewHeader::OnStateChange(unsigned int event)
{
	if (event != STATECHANGE_LOCAL_DIR)
		return;

	const wxString& dir = m_pState->GetLocalDir();
	m_pComboBox->SetValue(dir);
	m_pComboBox->SetSelection(m_pComboBox->GetValue().Length(), m_pComboBox->GetValue().Length());

	AddRecentDirectory(dir);
}

BEGIN_EVENT_TABLE(CRemoteViewHeader, CViewHeader)
EVT_TEXT_ENTER(wxID_ANY, CRemoteViewHeader::OnTextEnter)
EVT_COMBOBOX(wxID_ANY, CRemoteViewHeader::OnSelectionChanged)
END_EVENT_TABLE()

CRemoteViewHeader::CRemoteViewHeader(wxWindow* pParent, CState* pState)
	: CViewHeader(pParent, _("Remote site:")), CStateEventHandler(pState, STATECHANGE_REMOTE_DIR)
{
	m_pComboBox->Disable();
}

void CRemoteViewHeader::OnStateChange(unsigned int event)
{
	if (event != STATECHANGE_REMOTE_DIR)
		return;

	m_path = m_pState->GetRemotePath();
	if (m_path.IsEmpty())
	{
		m_pComboBox->SetValue(_T(""));
		m_pComboBox->Clear();
		m_pComboBox->Disable();
	}
	else
	{
		m_pComboBox->Enable();
		m_pComboBox->SetValue(m_path.GetPath());
		m_pComboBox->SetSelection(m_pComboBox->GetValue().Length(), m_pComboBox->GetValue().Length());
		
		AddRecentDirectory(m_path.GetPath());
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

	if (!m_pState->m_pCommandQueue->Idle())
	{
		wxBell();
		return;
	}

	m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(path));
}

void CRemoteViewHeader::OnSelectionChanged(wxCommandEvent& event)
{
	const wxString& dir = event.GetString();
	if (dir == _T(""))
		return;

	CServerPath path = m_path;
	if (!path.SetPath(dir))
	{
		wxBell();
		return;
	}

	if (!m_pState->m_pCommandQueue->Idle())
	{
		wxBell();
		return;
	}

	m_pState->m_pCommandQueue->ProcessCommand(new CListCommand(path));
}
