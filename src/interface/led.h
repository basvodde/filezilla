#ifndef __LED_H__
#define __LED_H__

class CLed : public wxWindow
{
public:
	CLed(wxWindow *parent, unsigned int index);
	~CLed();

	void Set();
	void Unset();

protected:
	void OnPaint(wxPaintEvent& event);

	int m_index;
	int m_state;

	wxBitmap m_bitmap;
	wxMemoryDC *m_dc;

	DECLARE_EVENT_TABLE()
};

#endif //__LED_H__
