#include "FileZilla.h"
#include "queue.h"
#include "statuslinectrl.h"
#include <wx/dcbuffer.h>
#include "Options.h"

#define TRANSFERSTATUS_TIMER_ID (wxID_HIGHEST + 1)

BEGIN_EVENT_TABLE(CStatusLineCtrl, wxWindow)
EVT_PAINT(CStatusLineCtrl::OnPaint)
EVT_TIMER(TRANSFERSTATUS_TIMER_ID, CStatusLineCtrl::OnTimer)
EVT_ERASE_BACKGROUND(CStatusLineCtrl::OnEraseBackground)
END_EVENT_TABLE()

int CStatusLineCtrl::m_fieldOffsets[4];
wxCoord CStatusLineCtrl::m_textHeight;
bool CStatusLineCtrl::m_initialized = false;

#define PROGRESSBAR_WIDTH 100

CStatusLineCtrl::CStatusLineCtrl(CQueueView* pParent, const t_EngineData* const pEngineData, const wxRect& initialPosition)
	: m_pEngineData(pEngineData)
{
	wxASSERT(pEngineData);

	Create(pParent, wxID_ANY, initialPosition.GetPosition(), initialPosition.GetSize());
	SetOwnFont(pParent->GetFont());
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	SetBackgroundColour(pParent->GetBackgroundColour());
	
	m_transferStatusTimer.SetOwner(this, TRANSFERSTATUS_TIMER_ID);

	m_madeProgress = false;
	m_pParent = pParent;
	m_pStatus = 0;
	m_lastOffset = -1;

	SetTransferStatus(0);


	// Calculate field widths so that the contents fit under every language.
	if (!m_initialized)
	{
		m_initialized = true;
		wxClientDC dc(this);
		dc.SetFont(GetFont());

		wxCoord w, h;
		wxTimeSpan elapsed(100, 0, 0);
		dc.GetTextExtent(elapsed.Format(_("%H:%M:%S elapsed")), &w, &h);
		m_textHeight = h;
		m_fieldOffsets[0] = 50 + w;

		dc.GetTextExtent(elapsed.Format(_("%H:%M:%S left")), &w, &h);
		m_fieldOffsets[1] = m_fieldOffsets[0] + 20 + w;

		m_fieldOffsets[2] = m_fieldOffsets[1] + 20;
		m_fieldOffsets[3] = m_fieldOffsets[2] + PROGRESSBAR_WIDTH + 20;
	}
}

CStatusLineCtrl::~CStatusLineCtrl()
{
	if (m_pStatus && m_pStatus->totalSize >= 0)
		m_pEngineData->pItem->SetSize(m_pStatus->totalSize);

	if (m_transferStatusTimer.IsRunning())
		m_transferStatusTimer.Stop();
	delete m_pStatus;
}

wxString FormatBytes(const wxLongLong& size)
{
	if (!COptions::Get()->GetOptionVal(OPTION_SIZE_USETHOUSANDSEP))
		return size.ToString();

#ifdef __WXMSW__
	wxChar sep[5];
	int count = ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, sep, 5);
	if (!count)
		return size.ToString();
#else
	char* chr = nl_langinfo(THOUSEP);
	if (!chr || !*chr)
		return size.ToString();
	wxChar sep = chr[0];
#endif

	wxString tmp = size.ToString();
	const int len = tmp.Len();
	if (len <= 3)
		return tmp;

	wxString result;
	int i = (len - 1) % 3 + 1;
	result = tmp.Left(i);
	while (i < len)
	{
		result += sep + tmp.Mid(i, 3);
		i += 3;
	}

	return result;
}

void CStatusLineCtrl::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);

	wxRect rect = GetRect();

	dc.SetFont(GetFont());
	dc.SetPen(GetBackgroundColour());
	dc.SetBrush(GetBackgroundColour());

	// Get character height so that we can center the text vertically.
	wxCoord h = (rect.GetHeight() - m_textHeight) / 2;

	// Clear background
	dc.DrawRectangle(0, 0, rect.GetWidth(), rect.GetHeight());

	if (!m_pStatus)
	{
		dc.DrawText(m_statusText, 50, h);
		return;
	}

	int elapsedSeconds;
	if (m_pStatus->started.IsValid())
	{
		wxTimeSpan elapsed = wxDateTime::Now().Subtract(m_pStatus->started);

		DrawRightAlignedText(dc, elapsed.Format(_("%H:%M:%S elapsed")), m_fieldOffsets[0], h);

		elapsedSeconds = elapsed.GetSeconds().GetLo(); // Assume GetHi is always 0
	}
	else
		elapsedSeconds = 0;

	const wxString bytes = FormatBytes(m_pStatus->currentOffset);
	if (elapsedSeconds)
	{
		wxFileOffset rate = (m_pStatus->currentOffset - m_pStatus->startOffset) / elapsedSeconds;

        if (rate > (1000*1000))
			dc.DrawText(wxString::Format(_("%s bytes (%d.%d MB/s)"), bytes.c_str(), (int)(rate / 1000 / 1000), (int)((rate / 1000 / 100) % 10)), m_fieldOffsets[3], h);
		else if (rate > 1000)
			dc.DrawText(wxString::Format(_("%s bytes (%d.%d KB/s)"), bytes.c_str(), (int)(rate / 1000), (int)((rate / 100) % 10)), m_fieldOffsets[3], h);
		else
			dc.DrawText(wxString::Format(_("%s bytes (%d B/s)"), bytes.c_str(), (int)rate), m_fieldOffsets[3], h);

		if (m_pStatus->totalSize > 0 && rate > 0)
		{
			int left = ((m_pStatus->totalSize - m_pStatus->startOffset) / rate) - elapsedSeconds;
			if (left < 0)
				left = 0;
			wxTimeSpan timeLeft(0, 0, left);
			DrawRightAlignedText(dc, timeLeft.Format(_("%H:%M:%S left")), m_fieldOffsets[1], h);
		}
		else
		{
			DrawRightAlignedText(dc, _("--:--:-- left"), m_fieldOffsets[1], h);
		}
	}
	else
	{
		DrawRightAlignedText(dc, _("--:--:-- left"), m_fieldOffsets[1], h);
		dc.DrawText(wxString::Format(_("%s bytes (? B/s)"), bytes.c_str()), m_fieldOffsets[3], h);
	}

	DrawProgressBar(dc, m_fieldOffsets[2], 1, rect.GetHeight() - 2);
}

void CStatusLineCtrl::SetTransferStatus(const CTransferStatus* pStatus)
{
	if (!pStatus)
	{
		if (m_pStatus && m_pStatus->totalSize >= 0)
			m_pParent->UpdateItemSize(m_pEngineData->pItem, m_pStatus->totalSize);
		delete m_pStatus;
		m_pStatus = 0;

		switch (m_pEngineData->state)
		{
		case t_EngineData::disconnect:
			m_statusText = _("Disconnecting from previous server");
			break;
		case t_EngineData::cancel:
			m_statusText = _("Waiting for transfer to be canceled");
			break;
		case t_EngineData::connect:
			m_statusText = wxString::Format(_("Connecting to %s"), m_pEngineData->lastServer.FormatServer().c_str());
			break;
		default:
			m_statusText = _("Transferring");
			break;
		}

		if (m_transferStatusTimer.IsRunning())
			m_transferStatusTimer.Stop();
	}
	else
	{
		if (!m_pStatus)
			m_pStatus = new CTransferStatus(*pStatus);
		else
			*m_pStatus = *pStatus;

		m_lastOffset = pStatus->currentOffset;

		if (!m_madeProgress && !pStatus->list && pStatus->currentOffset - pStatus->startOffset > 16384)
			m_madeProgress = true;

		if (!m_transferStatusTimer.IsRunning())
			m_transferStatusTimer.Start(100);
	}
	Refresh(false);
}

void CStatusLineCtrl::OnTimer(wxTimerEvent& event)
{
	bool changed;
	CTransferStatus status;
	if (!m_pEngineData->pEngine->GetTransferStatus(status, changed))
		SetTransferStatus(0);
	else if (changed)
		SetTransferStatus(&status);
	else
		m_transferStatusTimer.Stop();
}

void CStatusLineCtrl::DrawRightAlignedText(wxDC& dc, wxString text, int x, int y)
{
	wxCoord w, h;
	dc.GetTextExtent(text, &w, &h);
	x -= w;

	dc.DrawText(text, x, y);
}

void CStatusLineCtrl::OnEraseBackground(wxEraseEvent& event)
{
	// Don't erase background, only causes status line to flicker.
}

void CStatusLineCtrl::DrawProgressBar(wxDC& dc, int x, int y, int height)
{
	wxASSERT(m_pStatus);

	if (m_pStatus->totalSize <= 0)
		return;

	int barSplit = wxLongLong(m_pStatus->currentOffset * (PROGRESSBAR_WIDTH - 2) / m_pStatus->totalSize).GetLo();
	if (barSplit > PROGRESSBAR_WIDTH)
		barSplit = PROGRESSBAR_WIDTH;

	// Draw right part
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	dc.DrawRectangle(x + 1 + barSplit, y + 1, PROGRESSBAR_WIDTH - barSplit - 1, height - 2);

	if (barSplit && height > 2)
	{	
		// Draw pretty gradient

		int greenmin = 128;
		int greenmax = 255;
		int colourCount = ((height + 1) / 2);

		for (int i = 0; i < colourCount; i++)
		{
			int curGreen = greenmax - ((greenmax - greenmin) * i / (colourCount - 1));
			dc.SetPen(wxPen(wxColour(0, curGreen, 0)));
			dc.DrawLine(x + 1, y + colourCount - i, x + 1 + barSplit, y + colourCount - i);
			dc.DrawLine(x + 1, y + height - colourCount + i - 1, x + 1 + barSplit, y + height - colourCount + i - 1);
		}
	}

	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawRectangle(x, y, PROGRESSBAR_WIDTH, height);

	// Draw percentage-done text
	wxString prefix;
	int perMill;
	if (m_pStatus->currentOffset > m_pStatus->totalSize)
	{
		perMill = 1000;
		prefix = _T("> ");
	}
	else
		perMill = wxLongLong(m_pStatus->currentOffset * 1000 / m_pStatus->totalSize).GetLo();

	wxString text = wxString::Format(_T("%s%d.%d%%"), prefix.c_str(), perMill / 10, perMill % 10);

	wxCoord w, h;
	dc.GetTextExtent(text, &w, &h);
	dc.DrawText(text, x + PROGRESSBAR_WIDTH / 2 - w / 2, y + height / 2 - h / 2);
}

wxLongLong CStatusLineCtrl::GetSpeed() const
{
	if (!m_pStatus)
		return -1;

	wxTimeSpan elapsed = wxDateTime::Now().Subtract(m_pStatus->started);
	int elapsedSeconds = elapsed.GetSeconds().GetLo(); // Assume GetHi is always 0
	if (elapsedSeconds > 0)
		return (m_pStatus->currentOffset - m_pStatus->startOffset) / elapsedSeconds;
	else
		return -1;
}
