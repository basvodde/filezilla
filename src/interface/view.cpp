#include "FileZilla.h"
#include "view.h"
#include "viewheader.h"

BEGIN_EVENT_TABLE(CView, wxWindow)
EVT_SIZE(CView::OnSize)
END_EVENT_TABLE()

CView::CView(wxWindow* pParent)
	: wxWindow(pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
{
	m_pWnd = 0;
	m_pHeader = 0;

	m_pStatusBar = 0;
}

void CView::SetStatusBar(wxStatusBar* pStatusBar)
{
	m_pStatusBar = pStatusBar;
}

void CView::OnSize(wxSizeEvent& event)
{
	wxSize size = GetClientSize();
	wxRect rect(size);
	if (m_pHeader)
	{
		wxRect headerRect = rect;
		headerRect.SetHeight(m_pHeader->GetSize().GetHeight());
		m_pHeader->SetSize(headerRect);
		rect.SetHeight(rect.GetHeight() - headerRect.GetHeight());
		rect.SetY(headerRect.GetHeight());
	}
	if (m_pStatusBar && m_pStatusBar->IsShown())
	{
		const int status_height = m_pStatusBar->GetSize().GetHeight();
		rect.height -= status_height;

		wxRect status_rect = rect;
		status_rect.y += rect.height;
		status_rect.height = status_height;
		m_pStatusBar->SetSize(status_rect);
#ifdef __WXMSW__
		m_pStatusBar->Update();
#endif
	}
	if (!m_pWnd)
		return;
    
	m_pWnd->SetSize(rect);
}

void CView::SetHeader(CViewHeader* pWnd)
{
	m_pHeader = pWnd;
	if (m_pHeader && m_pHeader->GetParent() != this)
		CViewHeader::Reparent(&m_pHeader, this);
}

CViewHeader* CView::DetachHeader()
{
	CViewHeader* pHeader = m_pHeader;
	m_pHeader = 0;
	return pHeader;
}
