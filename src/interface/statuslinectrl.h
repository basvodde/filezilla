#ifndef __STATUSLINECTRL_H__
#define __STATUSLINECTRL_H__

class CQueueView;
class CStatusLineCtrl : public wxWindow
{
public:
	CStatusLineCtrl(CQueueView* pParent, const CQueueView::t_EngineData& engineData);
	~CStatusLineCtrl();

	const CFileItem* GetItem() const { return m_engineData.pItem; }

	void SetTransferStatus(const CTransferStatus* pStatus);

protected:
	void DrawRightAlignedText(wxDC& dc, wxString text, int x, int y);
	void DrawProgressBar(wxDC& dc, int x, int y, int height);

	CQueueView* m_pParent;
	const CQueueView::t_EngineData& m_engineData;
	CTransferStatus* m_pStatus;

	wxString m_statusText;
	wxTimer m_transferStatusTimer;

	static int m_fieldOffsets[4];
	static wxCoord m_textHeight;
	static bool m_initialized;

	DECLARE_EVENT_TABLE()
	void OnPaint(wxPaintEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnEraseBackground(wxEraseEvent& event);
};

#endif // __STATUSLINECTRL_H__