#include <filezilla.h>
#include "queue.h"
#include "statuslinectrl.h"
#include <wx/dcbuffer.h>
#include "Options.h"
#include "sizeformatting.h"
#ifdef __WXGTK__
#include "cursor_resetter.h"
#endif

BEGIN_EVENT_TABLE(CStatusLineCtrl, wxWindow)
EVT_PAINT(CStatusLineCtrl::OnPaint)
EVT_TIMER(wxID_ANY, CStatusLineCtrl::OnTimer)
EVT_ERASE_BACKGROUND(CStatusLineCtrl::OnEraseBackground)
END_EVENT_TABLE()

int CStatusLineCtrl::m_fieldOffsets[4];
wxCoord CStatusLineCtrl::m_textHeight;
bool CStatusLineCtrl::m_initialized = false;

#define PROGRESSBAR_WIDTH 102

CStatusLineCtrl::CStatusLineCtrl(CQueueView* pParent, const t_EngineData* const pEngineData, const wxRect& initialPosition)
	: m_pEngineData(pEngineData)
{
	m_mdc = 0;
	m_pPreviousStatusText = 0;
	m_last_elapsed_seconds = 0;
	m_last_left = 0;
	m_last_bar_split = -1;
	m_last_permill = -1;

	wxASSERT(pEngineData);

#ifdef __WXMSW__
	Create(pParent, wxID_ANY, initialPosition.GetPosition(), initialPosition.GetSize());
#else
	Create(pParent->GetMainWindow(), wxID_ANY, initialPosition.GetPosition(), initialPosition.GetSize());
#endif

	SetOwnFont(pParent->GetFont());
	SetForegroundColour(pParent->GetForegroundColour());
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	SetBackgroundColour(pParent->GetBackgroundColour());

	m_transferStatusTimer.SetOwner(this);

	m_pParent = pParent;
	m_pStatus = 0;
	m_lastOffset = -1;
	
	m_gcLastTimeStamp = wxDateTime::Now();
	m_gcLastOffset = -1;
	m_gcLastSpeed = -1;

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

#ifdef __WXGTK__
	ResetCursor(this);
#endif
}

CStatusLineCtrl::~CStatusLineCtrl()
{
	delete m_mdc;
	delete m_pPreviousStatusText;

	if (m_pStatus && m_pStatus->totalSize >= 0)
		m_pEngineData->pItem->SetSize(m_pStatus->totalSize);

	if (m_transferStatusTimer.IsRunning())
		m_transferStatusTimer.Stop();
	delete m_pStatus;
}

void CStatusLineCtrl::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	wxRect rect = GetRect();

	int refresh = 0;
	if (!m_data.IsOk() || rect.GetWidth() != m_data.GetWidth() || rect.GetHeight() != m_data.GetHeight())
	{
		delete m_mdc;
		m_data = wxBitmap(rect.GetWidth(), rect.GetHeight());
		m_mdc = new wxMemoryDC(m_data);
		refresh = 15;
	}

	int elapsed_seconds = 0;
	wxTimeSpan elapsed;
	int left = -1;
	wxFileOffset rate;
	wxString bytes_and_rate;
	int bar_split = -1;
	int permill = -1;

	if (!m_pStatus)
	{
		if (!m_pPreviousStatusText || *m_pPreviousStatusText != m_statusText)
		{
			// Clear background
			m_mdc->SetFont(GetFont());
			m_mdc->SetPen(GetBackgroundColour());
			m_mdc->SetBrush(GetBackgroundColour());
			m_mdc->SetTextForeground(GetForegroundColour());
			m_mdc->DrawRectangle(0, 0, rect.GetWidth(), rect.GetHeight());
			wxCoord h = (rect.GetHeight() - m_textHeight) / 2;
			m_mdc->DrawText(m_statusText, 50, h);
			delete m_pPreviousStatusText;
			m_pPreviousStatusText = new wxString(m_statusText);
			refresh = 0;
		}
	}
	else
	{
		if (m_pPreviousStatusText)
		{
			delete m_pPreviousStatusText;
			m_pPreviousStatusText = 0;
			refresh = 15;
		}

		if (m_pStatus->started.IsValid())
		{
			elapsed = wxDateTime::Now().Subtract(m_pStatus->started);
			elapsed_seconds = elapsed.GetSeconds().GetLo(); // Assume GetHi is always 0
		}

		if (elapsed_seconds != m_last_elapsed_seconds)
		{
			refresh |= 1;
			m_last_elapsed_seconds = elapsed_seconds;
		}

		if (COptions::Get()->GetOptionVal(OPTION_SPEED_DISPLAY))
			rate = GetCurrentSpeed();
		else
		    rate = GetSpeed(elapsed_seconds);

		if (elapsed_seconds && rate > 0)
		{
			left = ((m_pStatus->totalSize - m_pStatus->startOffset) / rate) - elapsed_seconds;
			if (left < 0)
				left = 0;
		}

		if (m_last_left != left)
		{
			refresh |= 2;
			m_last_left = left;
		}

		const wxString bytestr = CSizeFormat::Format(m_pStatus->currentOffset, true, CSizeFormat::bytes, COptions::Get()->GetOptionVal(OPTION_SIZE_USETHOUSANDSEP) != 0, 0);
		if (elapsed_seconds && rate > -1)
		{
			if (rate > (1000*1000))
				bytes_and_rate.Printf(_("%s (%d.%d MB/s)"), bytestr.c_str(), (int)(rate / 1000 / 1000), (int)((rate / 1000 / 100) % 10));
			else if (rate > 1000)
				bytes_and_rate.Printf(_("%s (%d.%d KB/s)"), bytestr.c_str(), (int)(rate / 1000), (int)((rate / 100) % 10));
			else
				bytes_and_rate.Printf(_("%s (%d B/s)"), bytestr.c_str(), (int)rate);
		}
		else
			bytes_and_rate.Printf(_("%s (? B/s)"), bytestr.c_str());

		if (m_last_bytes_and_rate != bytes_and_rate)
		{
			refresh |= 8;
			m_last_bytes_and_rate = bytes_and_rate;
		}

		if (m_pStatus->totalSize > 0)
		{
			bar_split = wxLongLong(m_pStatus->currentOffset * (PROGRESSBAR_WIDTH - 2) / m_pStatus->totalSize).GetLo();
			if (bar_split > (PROGRESSBAR_WIDTH - 2))
				bar_split = PROGRESSBAR_WIDTH - 2;

			if (m_pStatus->currentOffset > m_pStatus->totalSize)
				permill = 1001;
			else
				permill = wxLongLong(m_pStatus->currentOffset * 1000 / m_pStatus->totalSize).GetLo();
		}

		if (m_last_bar_split != bar_split || m_last_permill != permill)
		{
			refresh |= 4;
			m_last_bar_split = bar_split;
			m_last_permill = permill;
		}
	}

	if (refresh)
	{
		m_mdc->SetFont(GetFont());
		m_mdc->SetPen(GetBackgroundColour());
		m_mdc->SetBrush(GetBackgroundColour());
		m_mdc->SetTextForeground(GetForegroundColour());
	
		// Get character height so that we can center the text vertically.
		wxCoord h = (rect.GetHeight() - m_textHeight) / 2;

		if (refresh & 1)
		{
			m_mdc->DrawRectangle(0, 0, m_fieldOffsets[0], rect.GetHeight());
			DrawRightAlignedText(*m_mdc, elapsed.Format(_("%H:%M:%S elapsed")), m_fieldOffsets[0], h);
		}
		if (refresh & 2)
		{
			m_mdc->DrawRectangle(m_fieldOffsets[0], 0, m_fieldOffsets[1] - m_fieldOffsets[0], rect.GetHeight());
			if (left != -1)
			{
				wxTimeSpan timeLeft(0, 0, left);
				DrawRightAlignedText(*m_mdc, timeLeft.Format(_("%H:%M:%S left")), m_fieldOffsets[1], h);
			}
			else
				DrawRightAlignedText(*m_mdc, _("--:--:-- left"), m_fieldOffsets[1], h);
		}
		if (refresh & 8)
		{
			m_mdc->DrawRectangle(m_fieldOffsets[3], 0, rect.GetWidth() - m_fieldOffsets[3], rect.GetHeight());
			m_mdc->DrawText(bytes_and_rate, m_fieldOffsets[3], h);
		}
		if (refresh & 4)
		{
			if (bar_split != -1)
				DrawProgressBar(*m_mdc, m_fieldOffsets[2], 1, rect.GetHeight() - 2, bar_split, permill);
			else
				m_mdc->DrawRectangle(m_fieldOffsets[2], 0, m_fieldOffsets[3] - m_fieldOffsets[2], rect.GetHeight());
		}
	}
	dc.Blit(0, 0, rect.GetWidth(), rect.GetHeight(), m_mdc, 0, 0);
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
			m_statusText = _("Waiting for transfer to be cancelled");
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

		m_past_data_index = -1;
	}
	else
	{
		if (!m_pStatus)
			m_pStatus = new CTransferStatus(*pStatus);
		else
			*m_pStatus = *pStatus;

		m_lastOffset = pStatus->currentOffset;

		if (!m_transferStatusTimer.IsRunning())
			m_transferStatusTimer.Start(100);
	}
	Refresh(false);
}

void CStatusLineCtrl::OnTimer(wxTimerEvent& event)
{
	bool changed;
	CTransferStatus status;

	if (!m_pEngineData || !m_pEngineData->pEngine)
	{
		m_transferStatusTimer.Stop();
		return;
	}

	if (!m_pEngineData->pEngine->GetTransferStatus(status, changed))
		SetTransferStatus(0);
	else if (changed)
	{
		if (status.madeProgress && !status.list &&
			m_pEngineData->pItem->GetType() == QueueItemType_File)
		{
			CFileItem* pItem = (CFileItem*)m_pEngineData->pItem;
			pItem->m_madeProgress = true;
		}
		SetTransferStatus(&status);
	}
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

void CStatusLineCtrl::DrawProgressBar(wxDC& dc, int x, int y, int height, int bar_split, int permill)
{
	wxASSERT(bar_split != -1);
	wxASSERT(permill != -1);

	// Draw right part
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	dc.DrawRectangle(x + 1 + bar_split, y + 1, PROGRESSBAR_WIDTH - bar_split - 1, height - 2);

	if (bar_split && height > 2)
	{
		// Draw pretty gradient

		int greenmin = 128;
		int greenmax = 255;
		int colourCount = ((height + 1) / 2);

		for (int i = 0; i < colourCount; i++)
		{
			int curGreen = greenmax - ((greenmax - greenmin) * i / (colourCount - 1));
			dc.SetPen(wxPen(wxColour(0, curGreen, 0)));
			dc.DrawLine(x + 1, y + colourCount - i, x + 1 + bar_split, y + colourCount - i);
			dc.DrawLine(x + 1, y + height - colourCount + i - 1, x + 1 + bar_split, y + height - colourCount + i - 1);
		}
	}

	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawRectangle(x, y, PROGRESSBAR_WIDTH, height);

	// Draw percentage-done text
	wxString prefix;
	if( permill > 1000)
	{
		prefix = _T("> ");
		permill = 1000;
	}
	
	wxString text = wxString::Format(_T("%s%d.%d%%"), prefix.c_str(), permill / 10, permill % 10);

	wxCoord w, h;
	dc.GetTextExtent(text, &w, &h);
	dc.DrawText(text, x + PROGRESSBAR_WIDTH / 2 - w / 2, y + height / 2 - h / 2);
}

wxFileOffset CStatusLineCtrl::GetSpeed(int elapsedSeconds)
{
	if (!m_pStatus)
		return -1;

	if (elapsedSeconds <= 0)
		return -1;

	if (m_past_data_index < 9)
	{

		if (m_past_data_index == -1 || m_past_data[m_past_data_index].elapsed < elapsedSeconds)
		{
			m_past_data_index++;
			m_past_data[m_past_data_index].elapsed = elapsedSeconds;
			m_past_data[m_past_data_index].offset = m_pStatus->currentOffset - m_pStatus->startOffset;
		}
	}

	_past_data forget = {0};
	for (int i = m_past_data_index; i >= 0; i--)
	{
		if (m_past_data[i].elapsed < elapsedSeconds)
		{
			forget = m_past_data[i];
			break;
		}
	}

	return (m_pStatus->currentOffset - m_pStatus->startOffset - forget.offset) / (elapsedSeconds - forget.elapsed);
}

wxFileOffset CStatusLineCtrl::GetCurrentSpeed()
{
	if (!m_pStatus)
		return -1;
	
	const wxTimeSpan timeDiff( wxDateTime::UNow().Subtract(m_gcLastTimeStamp) );
	
	if (timeDiff.GetMilliseconds().GetLo() <= 2000)
		return m_gcLastSpeed;

	m_gcLastTimeStamp = wxDateTime::UNow();

	if (m_gcLastOffset == -1)
	    m_gcLastOffset = m_pStatus->startOffset;
	
	const wxFileOffset fileOffsetDiff = m_pStatus->currentOffset - m_gcLastOffset;
	m_gcLastOffset = m_pStatus->currentOffset;
	m_gcLastSpeed = fileOffsetDiff * 1000 / timeDiff.GetMilliseconds().GetLo();
	
	return m_gcLastSpeed;
}

bool CStatusLineCtrl::Show(bool show /*=true*/)
{
	if (show)
	{
		if (!m_transferStatusTimer.IsRunning())
			m_transferStatusTimer.Start(100);
	}
	else
		m_transferStatusTimer.Stop();

	return wxWindow::Show(show);
}

void CStatusLineCtrl::SetEngineData(const t_EngineData* const pEngineData)
{
	wxASSERT(pEngineData);
	m_pEngineData = pEngineData;
}
