#ifndef __LED_H__
#define __LED_H__

class CFileZillaEngine;
class CLed : public wxWindow
{
public:
	CLed(wxWindow *parent, unsigned int index);
	virtual ~CLed();

	void Ping(CFileZillaEngine *pEngine);

	void Stop();

protected:
	void Set();
	void Unset();

	int m_index;
	int m_ledState;

	std::list<CFileZillaEngine *> m_pinging_engines;

	wxBitmap m_leds[2];
	bool m_loaded;

	wxTimer m_timer;

	DECLARE_EVENT_TABLE()
	void OnPaint(wxPaintEvent& event);
	void OnTimer(wxTimerEvent& event);
#ifdef __WXMSW__
	void OnEraseBackground(wxEraseEvent& event);
#endif
};

#endif //__LED_H__
