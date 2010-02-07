#include "filezilla.h"
#include "splitter.h"

IMPLEMENT_CLASS(CSplitterWindowEx, wxSplitterWindow);

BEGIN_EVENT_TABLE(CSplitterWindowEx, wxSplitterWindow)
EVT_SIZE(CSplitterWindowEx::OnSize)
END_EVENT_TABLE()

CSplitterWindowEx::CSplitterWindowEx()
{
	m_relative_sash_position = 0.5;
	m_soft_min_pane_size = -1;
	m_lastSashPosition = -1;
}

CSplitterWindowEx::CSplitterWindowEx(wxWindow* parent, wxWindowID id, const wxPoint& point /*=wxDefaultPosition*/, const wxSize& size /*=wxDefaultSize*/, long style /*=wxSP_3D*/, const wxString& name /*=_T("splitterWindow")*/)
	: wxSplitterWindow(parent, id, point, size, style, name)
{
	m_relative_sash_position = 0.5;
	m_soft_min_pane_size = -1;
	m_lastSashPosition = -1;
}

bool CSplitterWindowEx::Create(wxWindow* parent, wxWindowID id, const wxPoint& point /*=wxDefaultPosition*/, const wxSize& size /*=wxDefaultSize*/, long style /*=wxSP_3D*/, const wxString& name /*=_T("splitterWindow")*/)
{
	return wxSplitterWindow::Create(parent, id, point, size, style, name);
}

void CSplitterWindowEx::SetSashGravity(double gravity)
{
	// Only support these three for now
	wxASSERT(gravity == 0.0 || gravity == 0.5 || gravity == 1.0);

	wxSplitterWindow::SetSashGravity(gravity);
}

void CSplitterWindowEx::OnSize(wxSizeEvent& event)
{
	// Code copied from wxWidgets and adjusted for better gravity handling

	// only process this message if we're not iconized - otherwise iconizing
	// and restoring a window containing the splitter has a funny side effect
	// of changing the splitter position!
	wxWindow *parent = wxGetTopLevelParent(this);
	bool iconized;

	wxTopLevelWindow *winTop = wxDynamicCast(parent, wxTopLevelWindow);
	if ( winTop )
	{
		iconized = winTop->IsIconized();
	}
	else
	{
		wxFAIL_MSG(wxT("should have a top level parent!"));

		iconized = false;
	}

	if ( iconized )
	{
		m_lastSize = wxSize(0,0);

		event.Skip();

		return;
	}

	if ( m_windowTwo )
	{
		int w, h;
		GetClientSize(&w, &h);

		int size = m_splitMode == wxSPLIT_VERTICAL ? w : h;

		int newPosition = m_sashPosition;

		int old_size = m_splitMode == wxSPLIT_VERTICAL ? m_lastSize.x : m_lastSize.y;
		if ( old_size != 0 )
		{
			if (m_sashGravity == 0.5)
				newPosition = (int)(size * m_relative_sash_position);
			else if (m_sashGravity == 1.0)
			{
				int delta = (int) ( (size - old_size) );
				if ( delta != 0 )
				{
					newPosition = m_sashPosition + delta;
					if( newPosition < m_minimumPaneSize )
						newPosition = m_minimumPaneSize;
				}
			}
			else
			{
				if (newPosition > size - m_minimumPaneSize - GetSashSize())
					newPosition = size - m_minimumPaneSize - GetSashSize();
			}
		}
		
		if ( newPosition >= size - 5 )
			newPosition = wxMax(10, size - 40);

		newPosition = CalcSoftLimit(newPosition);

		if (newPosition != m_sashPosition)
			SetSashPositionAndNotify(newPosition);

		m_lastSize = wxSize(w,h);
	}

	SizeWindows();
}

void CSplitterWindowEx::SetMinimumPaneSize(int paneSize, int paneSize_soft /*=-1*/)
{
	wxASSERT(paneSize_soft >= paneSize || paneSize_soft == -1);

	wxSplitterWindow::SetMinimumPaneSize(paneSize);

	m_soft_min_pane_size = paneSize_soft;
}

int CSplitterWindowEx::OnSashPositionChanging(int newSashPosition)
{
	newSashPosition = AdjustSashPosition(newSashPosition);
	newSashPosition = CalcSoftLimit(newSashPosition);

	newSashPosition = wxSplitterWindow::OnSashPositionChanging(newSashPosition);

	if (newSashPosition != -1)
	{
		int w, h;
		GetClientSize(&w, &h);

		int size = m_splitMode == wxSPLIT_VERTICAL ? w : h;

		m_relative_sash_position = (double)newSashPosition / size;
	}

	return newSashPosition;
}

int CSplitterWindowEx::CalcSoftLimit(int newSashPosition)
{
	if (m_soft_min_pane_size != -1)
	{
		int w, h;
		GetClientSize(&w, &h);

		int size = m_splitMode == wxSPLIT_VERTICAL ? w : h;

		int limit = size / 2;
		if (limit > m_soft_min_pane_size)
			limit = m_soft_min_pane_size;
		if (newSashPosition < limit)
			newSashPosition = limit;
		else if (newSashPosition > size - limit - GetSashSize())
			newSashPosition = wxMax(limit, size - limit - GetSashSize());
	}

	return newSashPosition;
}

void CSplitterWindowEx::SetRelativeSashPosition(double relative_sash_position)
{
	wxASSERT(relative_sash_position >= 0 && relative_sash_position <= 1);

	int w, h;
	GetClientSize(&w, &h);

	int size = m_splitMode == wxSPLIT_VERTICAL ? w : h;

	wxSplitterWindow::SetSashPosition((int)(size * relative_sash_position));

	m_relative_sash_position = relative_sash_position;
}

void CSplitterWindowEx::SetSashPosition(int sash_position)
{
	if (!m_windowTwo)
	{
		m_lastSashPosition = sash_position;
		return;
	}

	int w, h;
	GetClientSize(&w, &h);

	int size = m_splitMode == wxSPLIT_VERTICAL ? w : h;

	if (!sash_position)
		sash_position = size / 2;
	if (sash_position < 0 && m_sashGravity == 1.0)
		sash_position = size + sash_position - GetSashSize();

	wxSplitterWindow::SetSashPosition(sash_position);

	m_relative_sash_position = (double)sash_position / size;
}

bool CSplitterWindowEx::Unsplit(wxWindow* toRemove /*=NULL*/)
{
	if (m_sashGravity == 1)
	{
		int w, h;
		GetClientSize(&w, &h);

		int size = m_splitMode == wxSPLIT_VERTICAL ? w : h;

		m_lastSashPosition = m_sashPosition + GetSashSize() - size;
	}
	else
		m_lastSashPosition = m_sashPosition;

	return wxSplitterWindow::Unsplit(toRemove);
}

bool CSplitterWindowEx::SplitHorizontally(wxWindow* window1, wxWindow* window2, int sashPosition /*=0*/)
{
	int w, h;
	GetClientSize(&w, &h);

	int size = h;

	if (sashPosition == 0)
	{
		if (m_sashGravity == 0.5)
			sashPosition = (int)(size * m_relative_sash_position);
		else if (m_lastSashPosition != -1)
		{
			if (m_lastSashPosition < 0)
				sashPosition = size + m_lastSashPosition - GetSashSize();
			else
				sashPosition = m_lastSashPosition;
		}
	}

	// Needs to be set to avoid resizing oddity:
	// Maximize window -> Unsplit -> Restore ->	Split -> Resize window
	m_lastSize = wxSize(w, h);

	return wxSplitterWindow::SplitHorizontally(window1, window2, sashPosition);
}

bool CSplitterWindowEx::SplitVertically(wxWindow* window1, wxWindow* window2, int sashPosition /*=0*/)
{
	int w, h;
	GetClientSize(&w, &h);

	int size = w;

	if (sashPosition == 0)
	{
		if (m_sashGravity == 0.5)
		{
			sashPosition = (int)(size * m_relative_sash_position);
		}
		else if (m_lastSashPosition != -1)
		{
			if (m_lastSashPosition < 0)
				sashPosition = size + m_lastSashPosition - GetSashSize();
			else
				sashPosition = m_lastSashPosition;
		}
	}

	// Needs to be set to avoid resizing oddity:
	// Maximize window -> Unsplit -> Restore ->	Split -> Resize window
	m_lastSize = wxSize(w, h);

	return wxSplitterWindow::SplitVertically(window1, window2, sashPosition);
}

int CSplitterWindowEx::GetSashPosition() const
{
	if (m_windowTwo || m_lastSashPosition == -1)
	{
		int sashPosition = wxSplitterWindow::GetSashPosition();

		if (m_sashGravity == 1.0)
		{
			int w, h;
			GetClientSize(&w, &h);

			int size = m_splitMode == wxSPLIT_VERTICAL ? w : h;

			sashPosition = sashPosition + GetSashSize() - size;
		}

		return sashPosition;
	}

	return m_lastSashPosition;
}

void CSplitterWindowEx::Initialize(wxWindow *window)
{
	wxSplitterWindow::Initialize(window);

	SizeWindows();
}
