#include "FileZilla.h"
#include "QueueView.h"
#include "statuslinectrl.h"

BEGIN_EVENT_TABLE(CStatusLineCtrl, wxWindow)
EVT_PAINT(CStatusLineCtrl::OnPaint)
END_EVENT_TABLE()

CStatusLineCtrl::CStatusLineCtrl(CQueueView* pParent, const CQueueItem* pQueueItem)
	: wxWindow(pParent, wxID_ANY)
{
	m_pParent = pParent;
	m_pQueueItem = pQueueItem;
}

CStatusLineCtrl::~CStatusLineCtrl()
{
}

void CStatusLineCtrl::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	dc.DrawText(_T("OK"), 30, 0);
}
