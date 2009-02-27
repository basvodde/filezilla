#include "FileZilla.h"
#include "led.h"
#include "filezillaapp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMER_ID (wxID_HIGHEST + 1)

BEGIN_EVENT_TABLE(CLed, wxWindow)
	EVT_PAINT(CLed::OnPaint)
	EVT_TIMER(TIMER_ID, CLed::OnTimer)
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

	m_timer.SetOwner(this, TIMER_ID);

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
	if (event.GetId() != TIMER_ID)
	{
		event.Skip();
		return;
	}

	if (!m_timer.IsRunning())
		return;

	std::list<CFileZillaEngine *> old;
	m_pinging_engines.swap(old);
	for (std::list<CFileZillaEngine *>::const_iterator iter = old.begin(); iter != old.end(); iter++)
	{
		if ((*iter)->IsActive(m_index == 1))
			m_pinging_engines.push_back(*iter);
	}

	if (m_pinging_engines.empty())
	{
		Unset();
		m_timer.Stop();
	}

	return;
}

void CLed::Ping(CFileZillaEngine* pEngine)
{
	if (!m_loaded)
		return;

	std::list<CFileZillaEngine*>::const_iterator iter;
	for (iter = m_pinging_engines.begin(); iter != m_pinging_engines.end(); iter++)
	{
		if (*iter == pEngine)
			break;
	}
	if (iter == m_pinging_engines.end())
		m_pinging_engines.push_back(pEngine);

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

void CLed::Stop()
{
	m_loaded = false;
	m_timer.Stop();
}
