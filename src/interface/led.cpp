#include "filezilla.h"
#include "led.h"
#include "filezillaapp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(CLed, wxWindow)
	EVT_PAINT(CLed::OnPaint)
	EVT_TIMER(wxID_ANY, CLed::OnTimer)
#ifdef __WXMSW__
	EVT_ERASE_BACKGROUND(CLed::OnEraseBackground)
#endif
END_EVENT_TABLE()

#define LED_OFF 1
#define LED_ON 0

CLed::CLed(wxWindow *parent, unsigned int index)
	: wxWindow(parent, -1, wxDefaultPosition, wxSize(11, 11))
{
	if (index == 1)
		m_index = 1;
	else
		m_index = 0;

	m_ledState = LED_OFF;

	m_timer.SetOwner(this);

	m_loaded = false;

	wxImage image;
	if (!image.LoadFile(wxGetApp().GetResourceDir() + _T("leds.png"), wxBITMAP_TYPE_PNG))
		return;

	m_leds[0] = wxBitmap(image.GetSubImage(wxRect(0, index * 11, 11, 11)));
	m_leds[1] = wxBitmap(image.GetSubImage(wxRect(11, index * 11, 11, 11)));

	m_loaded = true;
}

CLed::~CLed()
{
	m_timer.Stop();
}

void CLed::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	if (!m_loaded)
		return;

	dc.DrawBitmap(m_leds[m_ledState], 0, 0, true);
}

void CLed::Set()
{
	if (m_ledState != LED_ON)
	{
		m_ledState = LED_ON;
		Refresh();
	}
}

void CLed::Unset()
{
	if (m_ledState != LED_OFF)
	{
		m_ledState = LED_OFF;
		Refresh();
	}
}

void CLed::OnTimer(wxTimerEvent& event)
{
	if (!m_timer.IsRunning())
		return;

	if (event.GetId() != m_timer.GetId())
	{
		event.Skip();
		return;
	}

	if (!CFileZillaEngine::IsActive((enum CFileZillaEngine::_direction)m_index))
	{
		Unset();
		m_timer.Stop();
	}

	return;
}

void CLed::Ping()
{
	if (!m_loaded)
		return;

	if (m_timer.IsRunning())
		return;
	
	Set();
	m_timer.Start(100);
}

#ifdef __WXMSW__
void CLed::OnEraseBackground(wxEraseEvent& event)
{
}
#endif
