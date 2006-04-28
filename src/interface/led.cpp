#include "FileZilla.h"
#include "led.h"
#include "filezillaapp.h"
#include "state.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMER_ID (wxID_HIGHEST + 1)

BEGIN_EVENT_TABLE(CLed, wxWindow)
	EVT_PAINT(CLed::OnPaint)
	EVT_TIMER(TIMER_ID, CLed::OnTimer)
END_EVENT_TABLE()

#define LED_OFF 1
#define LED_ON 0

CLed::CLed(wxWindow *parent, unsigned int index, CState* pState)
	: wxWindow(parent, -1, wxDefaultPosition, wxSize(11, 11))
{
	m_pState = pState;

	if (index == 1)
		m_index = 1;
	else
		m_index = 0;

	m_ledState = LED_OFF;

	wxLogNull *tmp = new wxLogNull;
	m_bitmap.LoadFile(wxGetApp().GetResourceDir() + _T("leds.png"), wxBITMAP_TYPE_PNG);
	delete tmp;
	if (m_bitmap.Ok())
	{
		m_dc = new wxMemoryDC;
		m_dc->SelectObject(m_bitmap);
	}

	m_timer.SetOwner(this, TIMER_ID);
}

CLed::~CLed()
{
	m_timer.Stop();
	delete m_dc;
}

void CLed::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	if (!m_dc)
		return;

	dc.Blit(0, 0, 11, 11, m_dc, m_ledState * 11, 11 * m_index, wxCOPY, true);
}

void CLed::Set()
{
	if (m_ledState != LED_ON)
	{
		m_ledState = LED_ON;
		Refresh(false);
	}
}

void CLed::Unset()
{
	if (m_ledState != LED_OFF)
	{
		m_ledState = LED_OFF;
		Refresh(false);
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

	if (!m_pState->m_pEngine)
	{
		m_timer.Stop();
		return;
	}

	if (!m_pState->m_pEngine->IsActive(m_index == 0))
	{
		Unset();
		m_timer.Stop();
	}

	return;
}

void CLed::Ping()
{
	if (m_timer.IsRunning())
		return;

	Set();
	m_timer.Start(100);
}
