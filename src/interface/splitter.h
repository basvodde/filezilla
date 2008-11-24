#ifndef __SPLITTER_H__
#define __SPLITTER_H__

class CSplitterWindowEx : public wxSplitterWindow
{
public:
	CSplitterWindowEx();
	CSplitterWindowEx(wxWindow* parent, wxWindowID id, const wxPoint& point = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxSP_3D, const wxString& name = _T("splitterWindow"));

	bool Create(wxWindow* parent, wxWindowID id, const wxPoint& point = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxSP_3D, const wxString& name = _T("splitterWindow"));

	void SetSashGravity(double gravity);

	// If pane size goes below paneSize_soft, make sure both panes are equally large
	void SetMinimumPaneSize(int paneSize, int paneSize_soft = -1);

	int GetSashPosition() const;
	void SetSashPosition(int sash_position);
	void SetRelativeSashPosition(double relative_sash_position);
	double GetRelativeSashPosition() const { return m_relative_sash_position; }

	void Initialize(wxWindow *window);

	bool SplitHorizontally(wxWindow* window1, wxWindow* window2, int sashPosition = 0);
	bool SplitVertically(wxWindow* window1, wxWindow* window2, int sashPosition = 0);

	bool Unsplit(wxWindow* toRemove = NULL);

protected:
	virtual int OnSashPositionChanging(int newSashPosition);

	int CalcSoftLimit(int newSashPosition);

	DECLARE_CLASS(CSplitterWindowEx);

	DECLARE_EVENT_TABLE();
	void OnSize(wxSizeEvent& event);

	double m_relative_sash_position;

	int m_soft_min_pane_size;

	int m_lastSashPosition;
};

#endif //__SPLITTER_H__
