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
	m_pTextCtrl = 0;
	m_pTextCtrl = new wxTextCtrl(this, -1, _T(""), wxDefaultPosition, wxDefaultSize,
								wxNO_BORDER | wxVSCROLL | wxTE_MULTILINE |
								wxTE_READONLY | wxTE_RICH | wxTE_RICH2 | wxTE_NOHIDESEL | wxTE_LINEWRAP);
	m_pTextCtrl->SetFont(GetFont());

	m_nLineCount = 0;

	InitDefAttr();
}

CStatusView::~CStatusView()
{
}

void CStatusView::OnSize(wxSizeEvent &event)
{
	if (m_pTextCtrl)
	{
		m_pTextCtrl->SetSize(GetClientSize());
	}
}

void CStatusView::AddToLog(CLogmsgNotification *pNotification)
{
	AddToLog(pNotification->msgType, pNotification->msg);
}

void CStatusView::AddToLog(enum MessageType messagetype, wxString message)
{
	m_pTextCtrl->Freeze();
	wxTextAttr attr = m_defAttr;
	wxString prefix;
	if (m_nLineCount)
		prefix = _T("\n");
	if (m_nLineCount == 1000)
	{
		int len = m_pTextCtrl->GetLineLength(0);
		m_pTextCtrl->Remove(0, len + 1);
	}
	else
		m_nLineCount++;

	switch (messagetype)
	{
	case Error:
		prefix += _("Error:");
		attr.SetTextColour(wxColour(255, 0, 0));
		break;
	case Command:
		prefix += _("Command:");
		attr.SetTextColour(wxColour(0, 0, 128));
		break;
	case Response:
		prefix += _("Response:");
		attr.SetTextColour(wxColour(0, 128, 0));
		break;
	case Debug_Warning:
	case Debug_Info:
	case Debug_Verbose:
	case Debug_Debug:
		prefix += _("Trace:");
		attr.SetTextColour(wxColour(128, 0, 128));
		break;
	case RawList:
		prefix += _("Listing:");
		attr.SetTextColour(wxColour(0, 128, 128));
		break;
	default:
		prefix += _("Status:");
		attr.SetTextColour(wxColour(0, 0, 0));
		break;
	}
	prefix += _T("\t");

	m_pTextCtrl->SetDefaultStyle(attr);
	m_pTextCtrl->AppendText(prefix + message);
	m_pTextCtrl->Thaw();
}

void CStatusView::InitDefAttr()
{
	// Measure withs of all types
    wxClientDC dc(this);

	dc.SetMapMode(wxMM_METRIC);

	int maxWidth = 0;
	wxCoord width = 0;
	wxCoord height = 0;
	dc.GetTextExtent(_("Error:"), &width, &height);
	maxWidth = width;
	dc.GetTextExtent(_("Command:"), &width, &height);
	if (width > maxWidth)
		maxWidth = width;
	dc.GetTextExtent(_("Response:"), &width, &height);
	if (width > maxWidth)
		maxWidth = width;
	dc.GetTextExtent(_("Trace:"), &width, &height);
	if (width > maxWidth)
		maxWidth = width;
	dc.GetTextExtent(_("Listing:"), &width, &height);
	if (width > maxWidth)
		maxWidth = width;
	dc.GetTextExtent(_("Status:"), &width, &height);
	if (width > maxWidth)
		maxWidth = width;

	maxWidth = maxWidth * 10 + 20;
	wxArrayInt array;
	array.Add(maxWidth);
	m_defAttr.SetTabs(array);
	m_defAttr.SetLeftIndent(0, maxWidth);

	m_defAttr.SetBackgroundColour(dc.GetTextBackground());
}
