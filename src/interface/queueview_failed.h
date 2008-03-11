#ifndef __QUEUEVIEW_FAILED_H__
#define __QUEUEVIEW_FAILED_H__

class CQueueViewFailed : public CQueueViewBase
{
public:
	CQueueViewFailed(CQueue* parent, int index);
	CQueueViewFailed(CQueue* parent, int index, const wxString& title);

protected:

	DECLARE_EVENT_TABLE();
	void OnContextMenu(wxContextMenuEvent& event);
	void OnRemoveAll(wxCommandEvent& event);
	void OnRemoveSelected(wxCommandEvent& event);
	void OnRequeueSelected(wxCommandEvent& event);
	void OnChar(wxKeyEvent& event);
};

#endif //__QUEUEVIEW_FAILED_H__
