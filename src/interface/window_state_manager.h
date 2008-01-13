#ifndef __WINDOW_STATE_MANAGER_H__
#define __WINDOW_STATE_MANAGER_H__

// This class get used to remember toplevel window size and position across
// sessions.

class CWindowStateManager : public wxEvtHandler
{
public:
	CWindowStateManager(wxTopLevelWindow* pWindow);
	virtual ~CWindowStateManager();

	bool Restore(unsigned int optionId);
	void Remember(unsigned int optionId);

protected:
	wxTopLevelWindow* m_pWindow;

	bool m_lastMaximized;
	wxPoint m_lastWindowPosition;
	wxSize m_lastWindowSize;

	void OnSize(wxSizeEvent& event);
	void OnMove(wxMoveEvent& event);
};

#endif //__WINDOW_STATE_MANAGER_H__
