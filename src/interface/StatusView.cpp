#include "FileZilla.h"
#include "StatusView.h"

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

void CStatusView::AddToLog(enum MessageType messagetype, wxString message)
{
	m_Content += "<tr><td valign='top'>";
	wxString font = "<font color=";

	wxString typeStr;
	switch (messagetype)
	{
	case Error:
		font += "red";
		typeStr = _("Error");
		break;
	case Command:
		font += "navy";
		typeStr = _("Command");
		break;
	case Response:
		font += "green";
		typeStr = _("Response");
		break;
	case Debug_Warning:
	case Debug_Info:
	case Debug_Verbose:
	case Debug_Debug:
		font += "purple";
		typeStr = _("Trace");
		break;
	case RawList:
		font += "fuchsia";
		typeStr = _("Listing");
		break;
	default:
		font += "black";
		typeStr = _("Status");
	}
	font += ">";

	m_Content += font;
	m_Content += typeStr + _T(":");
	
	m_Content += "</font></td><td width='5'></td><td>" + font;
	
	message.Replace("\n", "<br>");
	m_Content += message;
	
	m_Content += "</font>";

	m_Content += "</td></tr>";

	m_pHtmlWindow->SetPage("<html><body><table cellspacing='0' cellpadding = '0'>" + m_Content + "</table></body></html>");
}
