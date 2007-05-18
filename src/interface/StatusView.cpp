#include "FileZilla.h"
#include "StatusView.h"
#include <wx/wupdlock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MAX_LINECOUNT 1000

BEGIN_EVENT_TABLE(CStatusView, wxWindow)
	EVT_SIZE(CStatusView::OnSize)
	EVT_MENU(XRCID("ID_CLEARALL"), CStatusView::OnClear)
	EVT_MENU(XRCID("ID_COPYTOCLIPBOARD"), CStatusView::OnCopy)
END_EVENT_TABLE()

CStatusView::CStatusView(wxWindow* parent, wxWindowID id)
	: wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
{
	m_pTextCtrl = 0;
	m_pTextCtrl = new wxTextCtrl(this, -1, _T(""), wxDefaultPosition, wxDefaultSize,
								wxNO_BORDER | wxVSCROLL | wxTE_MULTILINE |
								wxTE_READONLY | wxTE_RICH | wxTE_RICH2 | wxTE_NOHIDESEL);
	m_pTextCtrl->SetFont(GetFont());

	m_pTextCtrl->Connect(wxID_ANY, wxEVT_CONTEXT_MENU, wxContextMenuEventHandler(CStatusView::OnContextMenu), 0, this);

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
	wxWindowUpdateLocker *pLock = 0;
	wxString prefix;
	
	if (m_nLineCount)
		prefix = _T("\n");
	
	if (m_nLineCount == MAX_LINECOUNT)
	{
		pLock = new wxWindowUpdateLocker(m_pTextCtrl);
		m_pTextCtrl->Remove(0, m_lineLengths.front() + 1);
		m_lineLengths.pop_front();
	}
	else
		m_nLineCount++;

	m_pTextCtrl->SetDefaultStyle(m_attributeCache[messagetype].attr);

	prefix += m_attributeCache[messagetype].prefix;

	int lineLength = m_attributeCache[messagetype].len + message.Length();

	if (m_rtl)
	{
		// Unicode control characters that control reading direction
		const wxChar LTR_MARK = 0x200e;
		//const wxChar RTL_MARK = 0x200f;
		const wxChar LTR_EMBED = 0x202A;
		//const wxChar RTL_EMBED = 0x202B;
		//const wxChar POP = 0x202c;
		//const wxChar LTR_OVERRIDE = 0x202D;
		//const wxChar RTL_OVERRIDE = 0x202E;

		if (messagetype == Command || messagetype == Response || messagetype >= Debug_Warning)
		{
			// Commands, responses and debug message contain English text,
			// set LTR reading order for them.
			prefix += LTR_MARK;
			prefix += LTR_EMBED;
			lineLength += 2;
		}
	}

	m_lineLengths.push_back(lineLength);

	m_pTextCtrl->AppendText(prefix + message);

	delete pLock;
}

void CStatusView::InitDefAttr()
{
	// Measure withs of all types
	wxClientDC dc(this);

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

	dc.SetMapMode(wxMM_LOMETRIC);

	maxWidth = dc.DeviceToLogicalX(maxWidth) + 20;	
	wxArrayInt array;
	array.Add(maxWidth);
	wxTextAttr defAttr;
	defAttr.SetTabs(array);
	defAttr.SetLeftIndent(0, maxWidth);

	for (int i = 0; i < MessageTypeCount; i++)
	{
		m_attributeCache[i].attr = defAttr;
		switch (i)
		{
		case Error:
			m_attributeCache[i].prefix = _("Error:");
			m_attributeCache[i].attr.SetTextColour(wxColour(255, 0, 0));
			break;
		case Command:
			m_attributeCache[i].prefix = _("Command:");
			m_attributeCache[i].attr.SetTextColour(wxColour(0, 0, 128));
			break;
		case Response:
			m_attributeCache[i].prefix = _("Response:");
			m_attributeCache[i].attr.SetTextColour(wxColour(0, 128, 0));
			break;
		case Debug_Warning:
		case Debug_Info:
		case Debug_Verbose:
		case Debug_Debug:
			m_attributeCache[i].prefix = _("Trace:");
			m_attributeCache[i].attr.SetTextColour(wxColour(128, 0, 128));
			break;
		case RawList:
			m_attributeCache[i].prefix = _("Listing:");
			m_attributeCache[i].attr.SetTextColour(wxColour(0, 128, 128));
			break;
		default:
			m_attributeCache[i].prefix = _("Status:");
			m_attributeCache[i].attr.SetTextColour(wxColour(0, 0, 0));
			break;
		}
		m_attributeCache[i].prefix += _T("\t");
		m_attributeCache[i].len = m_attributeCache[i].prefix.Length();
	}

	m_rtl = wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft;
}

void CStatusView::OnContextMenu(wxContextMenuEvent& event)
{
	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_LOG"));
	if (!pMenu)
		return;

	PopupMenu(pMenu);
	delete pMenu;
}

void CStatusView::OnClear(wxCommandEvent& event)
{
	if (m_pTextCtrl)
		m_pTextCtrl->Clear();
	m_nLineCount = 0;
	m_lineLengths.clear();
}

void CStatusView::OnCopy(wxCommandEvent& event)
{
	if (!m_pTextCtrl)
		return;
	
	long from, to;
	m_pTextCtrl->GetSelection(&from, &to);
	if (from != to)
		m_pTextCtrl->Copy();
	else
	{
		m_pTextCtrl->Freeze();
		m_pTextCtrl->SetSelection(-1, -1);
		m_pTextCtrl->Copy();
		m_pTextCtrl->SetSelection(from, to);
		m_pTextCtrl->Thaw();
	}
}
