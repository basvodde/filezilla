#include "FileZilla.h"
#include "StatusView.h"
#include <wx/wupdlock.h>
#include "Options.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MAX_LINECOUNT 1000

BEGIN_EVENT_TABLE(CStatusView, wxWindow)
	EVT_SIZE(CStatusView::OnSize)
	EVT_MENU(XRCID("ID_CLEARALL"), CStatusView::OnClear)
	EVT_MENU(XRCID("ID_COPYTOCLIPBOARD"), CStatusView::OnCopy)
END_EVENT_TABLE()

class CFastTextCtrl : public wxTextCtrl
{
public:
	CFastTextCtrl(wxWindow* parent)
		: wxTextCtrl(parent, -1, _T(""), wxDefaultPosition, wxDefaultSize,
					 wxNO_BORDER | wxVSCROLL | wxTE_MULTILINE |
					 wxTE_READONLY | wxTE_RICH | wxTE_RICH2 | wxTE_NOHIDESEL |
					 wxTAB_TRAVERSAL)
	{
	}
#ifdef __WXMSW__
	// wxTextCtrl::Remove is somewhat slow, this is a faster version
	virtual void Remove(long from, long to)
	{
		DoSetSelection(from, to, false);

		m_updatesCount = -2;        // suppress any update event

		::SendMessage((HWND)GetHandle(), EM_REPLACESEL, 0, (LPARAM)_T(""));
	}
#endif

	DECLARE_EVENT_TABLE();

	void OnText(wxCommandEvent& event)
	{
		// Do nothing here.
		// Having this event handler prevents the event from propagating up the
		// window hierarchy which saves a few CPU cycles.
	}

#ifdef __WXMSW__
	void OnNavigationKey(wxNavigationKeyEvent& event)
	{
		wxWindow* parent = GetParent();
		event.SetEventObject(parent);
		parent->ProcessEvent(event);
	}
#else
	void OnKeyDown(wxKeyEvent& event)
	{
		if (event.GetKeyCode() != WXK_TAB)
		{
			event.Skip();
			return;
		}

		wxWindow* parent = GetParent();

		wxNavigationKeyEvent navEvent;
		navEvent.SetEventObject(parent);
		navEvent.SetDirection(!event.ShiftDown());
		navEvent.SetFromTab(true);
		navEvent.ResumePropagation(1);
		parent->ProcessEvent(navEvent);
	}
#endif
};

BEGIN_EVENT_TABLE(CFastTextCtrl, wxTextCtrl)
	EVT_TEXT(wxID_ANY, CFastTextCtrl::OnText)
#ifdef __WXMSW__
	EVT_NAVIGATION_KEY(CFastTextCtrl::OnNavigationKey)
#else
	EVT_KEY_DOWN(CFastTextCtrl::OnKeyDown)
#endif
END_EVENT_TABLE()


CStatusView::CStatusView(wxWindow* parent, wxWindowID id)
	: wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
{
	m_pTextCtrl = 0;
	m_pTextCtrl = new CFastTextCtrl(this);
	m_pTextCtrl->SetFont(GetFont());

	m_pTextCtrl->Connect(wxID_ANY, wxEVT_CONTEXT_MENU, wxContextMenuEventHandler(CStatusView::OnContextMenu), 0, this);

	m_nLineCount = 0;

	InitDefAttr();

	m_shown = IsShown();
}

CStatusView::~CStatusView()
{
}

void CStatusView::OnSize(wxSizeEvent &event)
{
	if (m_pTextCtrl)
	{
		wxSize s = GetClientSize();
		m_pTextCtrl->SetSize(0, 0, s.GetWidth(), s.GetHeight());
	}
}

void CStatusView::AddToLog(CLogmsgNotification *pNotification)
{
	AddToLog(pNotification->msgType, pNotification->msg, wxDateTime::Now());
}

void CStatusView::AddToLog(enum MessageType messagetype, const wxString& message, const wxDateTime& time)
{
	if (!m_shown)
	{
		struct t_line line;
		line.messagetype = messagetype;
		line.message = message;
		line.time = time;

		m_hiddenLines.push_back(line);
		if (m_hiddenLines.size() > MAX_LINECOUNT)
			m_hiddenLines.pop_front();
		return;
	}

	const int messageLength = message.Length();

#ifndef __WXGTK__
	wxWindowUpdateLocker *pLock = 0;
#endif //__WXGTK__
	wxString prefix;
	prefix.Alloc(25 + messageLength);

	if (m_nLineCount)
		prefix = _T("\n");

	if (m_showTimestamps)
	{
		if (time != m_lastTime)
		{
			m_lastTime = time;
			m_lastTimeString = time.Format(_T("%H:%M:%S\t"));
		}
		prefix += m_lastTimeString;
	}

	if (m_nLineCount == MAX_LINECOUNT)
	{
#ifndef __WXGTK__
		pLock = new wxWindowUpdateLocker(m_pTextCtrl);
#endif //__WXGTK__
		m_pTextCtrl->Remove(0, m_lineLengths.front() + 1);
		m_lineLengths.pop_front();
	}
	else
		m_nLineCount++;

	m_pTextCtrl->SetDefaultStyle(m_attributeCache[messagetype].attr);

	prefix += m_attributeCache[messagetype].prefix;

	int lineLength = m_attributeCache[messagetype].len + messageLength;

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

	prefix += message;
	m_pTextCtrl->AppendText(prefix);

#ifndef __WXGTK__
	delete pLock;
#endif //__WXGTK__
}

void CStatusView::InitDefAttr()
{
	m_showTimestamps = COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_TIMESTAMP) != 0;
	m_lastTime = wxDateTime::Now();
	m_lastTimeString = m_lastTime.Format(_T("%H:%M:%S\t"));

	// Measure withs of all types
	wxClientDC dc(this);

	int timestampWidth = 0;
	if (m_showTimestamps)
	{
		wxCoord width = 0;
		wxCoord height = 0;
		dc.GetTextExtent(_T("88:88:88"), &width, &height);
		timestampWidth = width;
	}

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
	if (timestampWidth != 0)
	{
		timestampWidth = dc.DeviceToLogicalX(timestampWidth) + 20;
		maxWidth += timestampWidth;
	}
	wxArrayInt array;
	if (timestampWidth != 0)
		array.Add(timestampWidth);
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

void CStatusView::SetFocus()
{
	m_pTextCtrl->SetFocus();
}

bool CStatusView::Show(bool show /*=true*/)
{
	m_shown = show;

	if (show)
	{
		if (m_hiddenLines.size() == MAX_LINECOUNT)
		{
			if (m_pTextCtrl)
				m_pTextCtrl->Clear();
			m_nLineCount = 0;
			m_lineLengths.clear();
		}

		for (std::list<t_line>::const_iterator iter = m_hiddenLines.begin(); iter != m_hiddenLines.end(); iter++)
		{
			AddToLog(iter->messagetype, iter->message, iter->time);
		}
		m_hiddenLines.clear();
	}

	return wxWindow::Show(show);
}
