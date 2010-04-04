#ifndef __STATUSLINECTRL_H__
#define __STATUSLINECTRL_H__

class CQueueView;
class CStatusLineCtrl : public wxWindow
{
public:
	CStatusLineCtrl(CQueueView* pParent, const t_EngineData* const pEngineData, const wxRect& initialPosition);
	~CStatusLineCtrl();

	const CFileItem* GetItem() const { return m_pEngineData->pItem; }

	void SetEngineData(const t_EngineData* const pEngineData);

	void SetTransferStatus(const CTransferStatus* pStatus);
	wxLongLong GetLastOffset() const { return m_pStatus ? m_pStatus->currentOffset : m_lastOffset; }
	wxLongLong GetTotalSize() const { return m_pStatus ? m_pStatus->totalSize : -1; }
	wxFileOffset GetSpeed(int elapsedSeconds);
	wxFileOffset GetCurrentSpeed();

	virtual bool Show(bool show = true);

protected:
	void DrawRightAlignedText(wxDC& dc, wxString text, int x, int y);
	void DrawProgressBar(wxDC& dc, int x, int y, int height);

	CQueueView* m_pParent;
	const t_EngineData* m_pEngineData;
	CTransferStatus* m_pStatus;

	wxString m_statusText;
	wxTimer m_transferStatusTimer;

	static int m_fieldOffsets[4];
	static wxCoord m_textHeight;
	static bool m_initialized;

	bool m_madeProgress;

	wxLongLong m_lastOffset; // Stores the last transfer offset so that the total queue size can be accurately calculated.

	// This is used by GetSpeed to forget about the first 10 seconds on longer transfers
	// since at the very start the speed is hardly accurate (e.g. due to TCP slow start)
	struct _past_data
	{
		int elapsed;
		wxFileOffset offset;
	} m_past_data[10];
	int m_past_data_index;

	//Used by getCurrentSpeed
	wxDateTime m_gcLastTimeStamp;
	wxFileOffset m_gcLastOffset;
	wxFileOffset m_gcLastSpeed;

	DECLARE_EVENT_TABLE()
	void OnPaint(wxPaintEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnEraseBackground(wxEraseEvent& event);
};

#endif // __STATUSLINECTRL_H__
