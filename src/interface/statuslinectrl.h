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
	wxLongLong GetLastOffset() const { return m_pStatus ? m_pStatus->currentOffset : m_lastOffset; }
	wxLongLong GetTotalSize() const { return m_pStatus ? m_pStatus->totalSize : -1; }
	wxLongLong GetSpeed() const;

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

	wxLongLong m_lastOffset; // Stores the last transfer offset so that the total queue size can be accurately calculated.

	DECLARE_EVENT_TABLE()
	void OnPaint(wxPaintEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnEraseBackground(wxEraseEvent& event);
};

#endif // __STATUSLINECTRL_H__
