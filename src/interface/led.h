#ifndef __LED_H__
#define __LED_H__

class CState;
class CLed : public wxWindow
{
public:
	CLed(wxWindow *parent, unsigned int index, CState* pState);
	~CLed();

	void Ping();

protected:
	void Set();
	void Unset();

	int m_index;
	int m_ledState;

	CState* m_pState;

	wxBitmap m_leds[2];
	bool m_loaded;

	wxTimer m_timer;

	DECLARE_EVENT_TABLE()
	void OnPaint(wxPaintEvent& event);
	void OnTimer(wxTimerEvent& event);
};

#endif //__LED_H__
