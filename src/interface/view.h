#ifndef __VIEW_H__
#define __VIEW_H__

class CViewHeader;
class CView : public wxWindow
{
public:
	CView(wxWindow* pParent);

	void SetWindow(wxWindow* pWnd) { m_pWnd = pWnd; }
	void SetHeader(CViewHeader* pWnd);
	CViewHeader* DetachHeader();

protected:
	wxWindow* m_pWnd;
	CViewHeader* m_pHeader;

	DECLARE_EVENT_TABLE();
	void OnSize(wxSizeEvent& event);
};

#endif //__VIEW_H__
