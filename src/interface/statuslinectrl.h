#ifndef __STATUSLINECTRL_H__
#define __STATUSLINECTRL_H__

class CQueueView;
class CQueueItem;
class CStatusLineCtrl : public wxWindow
{
public:
	CStatusLineCtrl(CQueueView* pParent, const CQueueItem* pQueueItem);
	~CStatusLineCtrl();

	const CQueueItem* GetItem() const { return m_pQueueItem; }

	int m_itemIndex;

protected:
	CQueueView* m_pParent;
	const CQueueItem* m_pQueueItem;

	DECLARE_EVENT_TABLE()
	void OnPaint(wxPaintEvent& event);
};

#endif // __STATUSLINECTRL_H__