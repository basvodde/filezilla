#include "filezilla.h"
#include "statusview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(CStatusView, wxWindow)
    EVT_SIZE(CStatusView::OnSize)
END_EVENT_TABLE()

CStatusView::CStatusView(wxWindow* parent, wxWindowID id)
	: wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
{
	m_pHtmlWindow = NULL;
	m_pHtmlWindow = new wxHtmlWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
	m_pHtmlWindow->SetBorders(3);
	int sizes[7];
	for (int i = 0; i < 7; i++)
		sizes[i] = GetFont().GetPointSize();
	m_pHtmlWindow->SetFonts(GetFont().GetFaceName(), GetFont().GetFaceName(), sizes);
	m_pHtmlWindow->LoadPage("C:\\doc\\wxWindows-2.5.1\\docs\\htmlhelp\\test_table.htm");
}

CStatusView::~CStatusView()
{
}

void CStatusView::OnSize(wxSizeEvent &event)
{
	if (m_pHtmlWindow)
	{
		m_pHtmlWindow->SetSize(GetClientSize());
	}
}