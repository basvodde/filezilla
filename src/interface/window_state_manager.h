#ifndef __WINDOW_STATE_MANAGER_H__
#define __WINDOW_STATE_MANAGER_H__

// This class get used to remember toplevel window size and position across
// sessions.

class CWindowStateManager : public wxEvtHandler
{
public:
	CWindowStateManager(wxTopLevelWindow* pWindow);
	virtual ~CWindowStateManager();

	bool Restore(const unsigned int optionId, const wxSize& default_size = wxSize(-1, -1));
	void Remember(unsigned int optionId);

	static wxRect GetScreenDimensions();

#ifdef __WXGTK__
	// Set nonzero if Restore maximized the window.
	// Reason is that under wxGTK, maximization request may take some time.
	// It is actually done asynchronously by the window manager.
	unsigned int m_maximize_requested;
#endif

protected:
	bool ReadDefaults(const unsigned int optionId, bool& maximized, wxPoint& position, wxSize& size);

	wxTopLevelWindow* m_pWindow;

	bool m_lastMaximized;
	wxPoint m_lastWindowPosition;
	wxSize m_lastWindowSize;

	void OnSize(wxSizeEvent& event);
	void OnMove(wxMoveEvent& event);
};

#endif //__WINDOW_STATE_MANAGER_H__
