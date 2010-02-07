#include <filezilla.h>
#include "window_state_manager.h"
#include "Options.h"
#if wxUSE_DISPLAY
#include <wx/display.h>
#endif
#include <wx/tokenzr.h>

CWindowStateManager::CWindowStateManager(wxTopLevelWindow* pWindow)
{
	m_pWindow = pWindow;

	m_lastMaximized = false;

	m_pWindow->Connect(wxID_ANY, wxEVT_SIZE, wxSizeEventHandler(CWindowStateManager::OnSize), 0, this);
	m_pWindow->Connect(wxID_ANY, wxEVT_MOVE, wxMoveEventHandler(CWindowStateManager::OnMove), 0, this);

#ifdef __WXGTK__
	m_maximize_requested = 0;
#endif
}

CWindowStateManager::~CWindowStateManager()
{
	m_pWindow->Disconnect(wxID_ANY, wxEVT_SIZE, wxSizeEventHandler(CWindowStateManager::OnSize), 0, this);
	m_pWindow->Disconnect(wxID_ANY, wxEVT_MOVE, wxMoveEventHandler(CWindowStateManager::OnMove), 0, this);
}

void CWindowStateManager::Remember(unsigned int optionId)
{
	if (!m_lastWindowSize.IsFullySpecified())
		return;

	wxString posString;

	// is_maximized
	posString += wxString::Format(_T("%d "), m_lastMaximized ? 1 : 0);

	// pos_x pos_y
	posString += wxString::Format(_T("%d %d "), m_lastWindowPosition.x, m_lastWindowPosition.y);

	// pos_width pos_height
	posString += wxString::Format(_T("%d %d "), m_lastWindowSize.x, m_lastWindowSize.y);

	COptions::Get()->SetOption(optionId, posString);
}

bool CWindowStateManager::ReadDefaults(const unsigned int optionId, bool& maximized, wxPoint& position, wxSize& size)
{
	if (wxGetKeyState(WXK_SHIFT) && wxGetKeyState(WXK_ALT) && wxGetKeyState(WXK_CONTROL))
		return false;
	
	// Fields:
	// - maximized (1 or 0)
	// - x position
	// - y position
	// - width
	// - height
	const wxString layout = COptions::Get()->GetOption(optionId);
	wxStringTokenizer tokens(layout, _T(" "));
	int count = tokens.CountTokens();
	if (count != 5)
		return false;

	long values[5];
	for (int i = 0; i < count; i++)
	{
		wxString token = tokens.GetNextToken();
		if (!token.ToLong(values + i))
			return false;
	}
	if (values[3] <= 0 || values[4] <= 0)
		return false;

	const wxRect screen_size = GetScreenDimensions();

	// Make sure position is (somewhat) sane
	position.x = wxMin(screen_size.GetRight() - 30, values[1]);
	position.y = wxMin(screen_size.GetBottom() - 30, values[2]);
	size.x = values[3];
	size.y = values[4];

	if (position.x + size.x - 30 < screen_size.GetLeft())
		position.x = screen_size.GetLeft();
	if (position.y + size.y - 30 < screen_size.GetTop())
		position.y = screen_size.GetTop();

	maximized = values[0] != 0;

	return true;
}

bool CWindowStateManager::Restore(const unsigned int optionId, const wxSize& default_size /*=wxSize(-1, -1)*/)
{
	wxPoint position(-1, -1);
	wxSize size = default_size;
	bool maximized = false;

	bool read = ReadDefaults(optionId, maximized, position, size);

	if (maximized)
	{
		// We need to move so it appears on the proper display on multi-monitor systems
		m_pWindow->Move(position.x, position.y);

		// We need to call SetClientSize here too. Since window isn't yet shown here, Maximize
		// doesn't actually resize the window till it is shown

		// The slight off-size is needed to ensure the client sizes gets changed at least once.
		// Otherwise all the splitters would have default size still.
		m_pWindow->SetClientSize(size.x + 1, size.y);

		// A 2nd call is neccessary, for some reason the first call
		// doesn't fully set the height properly at least under wxMSW
		m_pWindow->SetClientSize(size.x, size.y);

#ifdef __WXMSW__
		m_pWindow->Show();
#endif
		m_pWindow->Maximize();
#ifdef __WXGTK__
		if (!m_pWindow->IsMaximized())
			m_maximize_requested = 4;
#endif //__WXGTK__
	}
	else
	{
		if (read)
			m_pWindow->Move(position.x, position.y);

		if (size.IsFullySpecified())
		{
			// The slight off-size is needed to ensure the client sizes gets changed at least once.
			// Otherwise all the splitters would have default size still.
			m_pWindow->SetClientSize(size.x + 1, size.x);

			// A 2nd call is neccessary, for some reason the first call
			// doesn't fully set the height properly at least under wxMSW
			m_pWindow->SetClientSize(size.x, size.y);
		}

		if (!read)
			m_pWindow->CentreOnScreen();
	}

	return true;
}

void CWindowStateManager::OnSize(wxSizeEvent& event)
{
#ifdef __WXGTK__
	if (m_maximize_requested)
		m_maximize_requested--;
#endif
	if (!m_pWindow->IsIconized())
	{
		m_lastMaximized = m_pWindow->IsMaximized();
		if (!m_lastMaximized)
		{
			m_lastWindowPosition = m_pWindow->GetPosition();
			m_lastWindowSize = m_pWindow->GetClientSize();
		}
	}
	event.Skip();
}

void CWindowStateManager::OnMove(wxMoveEvent& event)
{
	if (!m_pWindow->IsIconized() && !m_pWindow->IsMaximized())
	{
		m_lastWindowPosition = m_pWindow->GetPosition();
		m_lastWindowSize = m_pWindow->GetClientSize();
	}
	event.Skip();
}

wxRect CWindowStateManager::GetScreenDimensions()
{
#if wxUSE_DISPLAY
	wxRect screen_size(0, 0, 0, 0);

	// Get bounding rectangle of virtual screen
	for (unsigned int i = 0; i < wxDisplay::GetCount(); i++)
	{
		wxDisplay display(i);
		wxRect rect = display.GetGeometry();
		screen_size.Union(rect);
	}
	if (screen_size.GetWidth() <= 0 || screen_size.GetHeight() <= 0)
		screen_size = wxRect(0, 0, 1000000000, 1000000000);
#else
	wxRect screen_size(0, 0, 1000000000, 1000000000);
#endif

	return screen_size;
}
