#include "FileZilla.h"
#include "led.h"
#include "filezillaapp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(CLed, wxWindow)
	EVT_PAINT(CLed::OnPaint)
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

	m_state = LED_OFF;

	wxLogNull *tmp = new wxLogNull;
	m_bitmap.LoadFile(wxGetApp().GetResourceDir() + _T("leds.png"), wxBITMAP_TYPE_PNG);
	delete tmp;
	if (m_bitmap.Ok())
	{
		m_dc = new wxMemoryDC;
		m_dc->SelectObject(m_bitmap);
	}
}

CLed::~CLed()
{
	delete m_dc;
}

void CLed::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	if (!m_dc)
		return;

	dc.Blit(0, 0, 11, 11, m_dc, m_state * 11, 11 * m_index, wxCOPY, true);
}

void CLed::Set()
{
	if (m_state != LED_ON)
	{
		m_state = LED_ON;
		Refresh(false);
	}
}

void CLed::Unset()
{
	if (m_state != LED_OFF)
	{
		m_state = LED_OFF;
		Refresh(false);
	}
}

