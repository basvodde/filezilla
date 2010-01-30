#include "FileZilla.h"
#include "statusbar.h"
#include "Options.h"
#include "verifycertdialog.h"
#include "sftp_crypt_info_dlg.h"
#include "speedlimits_dialog.h"

static const int statbarWidths[3] = {
	-3, 0, 35
};
#define FIELD_QUEUESIZE 1

BEGIN_EVENT_TABLE(wxStatusBarEx, wxStatusBar)
EVT_SIZE(wxStatusBarEx::OnSize)
END_EVENT_TABLE()

wxStatusBarEx::wxStatusBarEx(wxTopLevelWindow* pParent)
{
	m_pParent = pParent;
	m_columnWidths = 0;
	Create(pParent, wxID_ANY);

#ifdef __WXMSW__
	m_parentWasMaximized = false;

	if (GetLayoutDirection() != wxLayout_RightToLeft)
		SetDoubleBuffered(true);
#endif
}

wxStatusBarEx::~wxStatusBarEx()
{
	delete [] m_columnWidths;
}

void wxStatusBarEx::SetFieldsCount(int number /*=1*/, const int* widths /*=NULL*/)
{
	wxASSERT(number > 0);

	int oldCount = GetFieldsCount();
	int* oldWidths = m_columnWidths;
	
	m_columnWidths = new int[number];
	if (!widths)
	{
		if (oldWidths)
		{
			const int min = wxMin(oldCount, number);
			for (int i = 0; i < min; i++)
				m_columnWidths[i] = oldWidths[i];
			for (int i = min; i < number; i++)
				m_columnWidths[i] = -1;
			delete [] oldWidths;
		}
		else
		{
			for (int i = 0; i < number; i++)
				m_columnWidths[i] = -1;
		}
	}
	else
	{
		delete [] oldWidths;
		for (int i = 0; i < number; i++)
			m_columnWidths[i] = widths[i];

		FixupFieldWidth(number - 1);
	}

	wxStatusBar::SetFieldsCount(number, m_columnWidths);
}

void wxStatusBarEx::SetStatusWidths(int n, const int *widths)
{
	wxASSERT(n == GetFieldsCount());
	wxASSERT(widths);
	for (int i = 0; i < n; i++)
		m_columnWidths[i] = widths[i];

	FixupFieldWidth(n - 1);

	wxStatusBar::SetStatusWidths(n, m_columnWidths);
}

void wxStatusBarEx::FixupFieldWidth(int field)
{
	if (field != GetFieldsCount() - 1)
		return;

#ifdef __WXMSW__
	// Gripper overlaps last field if not maximized
	if (!m_parentWasMaximized && m_columnWidths[field] > 0)
		m_columnWidths[field] += 6;
#elif __WXGTK20__
	// Gripper overlaps last all the time
	if (m_columnWidths[field] > 0)
		m_columnWidths[field] += 15;
#endif
}

void wxStatusBarEx::SetFieldWidth(int field, int width)
{
	field = GetFieldIndex(field);
	m_columnWidths[field] = width;

	FixupFieldWidth(field);

	wxStatusBar::SetStatusWidths(GetFieldsCount(), m_columnWidths);
}

int wxStatusBarEx::GetFieldIndex(int field)
{
	if (field >= 0)
	{
		wxASSERT(field <= GetFieldsCount());
	}
	else
	{
		field = GetFieldsCount() + field;
		wxASSERT(field >= 0);
	}

	return field;
}

void wxStatusBarEx::OnSize(wxSizeEvent& event)
{
#ifdef __WXMSW__
	const int count = GetFieldsCount();
	if (count && m_columnWidths && m_columnWidths[count - 1] > 0)
	{
		// No sizegrip on maximized windows
		bool isMaximized = m_pParent->IsMaximized();
		if (isMaximized != m_parentWasMaximized)
		{
			m_parentWasMaximized = isMaximized;

			if (isMaximized)
				m_columnWidths[count - 1] -= 6;
			else
				m_columnWidths[count - 1] += 6;

			wxStatusBar::SetStatusWidths(count, m_columnWidths);
			Refresh();
		}
	}
#endif
}

#ifdef __WXGTK__
void wxStatusBarEx::SetStatusText(const wxString& text, int number /*=0*/)
{
	// Basically identical to the wx one, but not calling Update
	wxString oldText = m_statusStrings[number];
	if (oldText != text)
	{
		m_statusStrings[number] = text;

		wxRect rect;
		GetFieldRect(number, rect);

		Refresh(true, &rect);
	}
}
#endif

int wxStatusBarEx::GetGripperWidth()
{
#if defined(__WXMSW__)
	return m_pParent->IsMaximized() ? 0 : 6;
#elif defined(__WXGTK__)
	return 15;
#else
	return 0;
#endif
}



BEGIN_EVENT_TABLE(CWidgetsStatusBar, wxStatusBarEx)
EVT_SIZE(CWidgetsStatusBar::OnSize)
END_EVENT_TABLE()

CWidgetsStatusBar::CWidgetsStatusBar(wxTopLevelWindow* parent)
	: wxStatusBarEx(parent)
{
}

CWidgetsStatusBar::~CWidgetsStatusBar()
{
}

void CWidgetsStatusBar::OnSize(wxSizeEvent& event)
{
	wxStatusBarEx::OnSize(event);

	for (int i = 0; i < GetFieldsCount(); i++)
		PositionChildren(i);

#ifdef __WXMSW__
	if (GetLayoutDirection() != wxLayout_RightToLeft)
		Update();
#endif
}

void CWidgetsStatusBar::AddChild(int field, int idx, wxWindow* pChild)
{
	field = GetFieldIndex(field);

	t_statbar_child data;
	data.field = field;
	data.pChild = pChild;

	m_children[idx] = data;

	PositionChildren(field);
}

void CWidgetsStatusBar::RemoveChild(int idx)
{
	std::map<int, struct t_statbar_child>::iterator iter = m_children.find(idx);
	if (iter != m_children.end())
	{
		int field = iter->second.field;
		m_children.erase(iter);
		PositionChildren(field);
	}
}

void CWidgetsStatusBar::PositionChildren(int field)
{
	wxRect rect;
	GetFieldRect(field, rect);
	
	int offset = 2;

	if (field + 1 == GetFieldsCount())
	{
		rect.SetWidth(m_columnWidths[field]);	
		offset += 5 + GetGripperWidth();
	}

	for (std::map<int, struct t_statbar_child>::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if (iter->second.field != field)
			continue;

		const wxSize size = iter->second.pChild->GetSize();
		int position = rect.GetRight() - size.x - offset;

		iter->second.pChild->SetSize(position, rect.GetTop() + (rect.GetHeight() - size.y + 1) / 2, -1, -1);

		offset += size.x + 3;
	}
}

#ifdef __WXMSW__
class wxStaticBitmapEx : public wxStaticBitmap
{
public:
	wxStaticBitmapEx(wxWindow* parent, int id, const wxBitmap& bmp)
		: wxStaticBitmap(parent, id, bmp)
	{
	};

protected:
	DECLARE_EVENT_TABLE();

	// Make sure it is truly transparent, i.e. also works with
	// themed status bars.
	void OnErase(wxEraseEvent& event)
	{
	}

	void OnPaint(wxPaintEvent& event)
	{
		wxPaintDC dc(this);
		dc.DrawBitmap(GetBitmap(), 0, 0, true);
	}
};

BEGIN_EVENT_TABLE(wxStaticBitmapEx, wxStaticBitmap)
EVT_ERASE_BACKGROUND(wxStaticBitmapEx::OnErase)
EVT_PAINT(wxStaticBitmapEx::OnPaint)
END_EVENT_TABLE()
#else
#define wxStaticBitmapEx wxStaticBitmap
#endif

class CIndicator : public wxStaticBitmapEx
{
public:
	CIndicator(CStatusBar* pStatusBar, const wxBitmap& bmp)
		: wxStaticBitmapEx(pStatusBar, wxID_ANY, bmp)
	{
		m_pStatusBar = pStatusBar;
	}

protected:
	CStatusBar* m_pStatusBar;

	DECLARE_EVENT_TABLE()
	void OnLeftMouseUp(wxMouseEvent& event)
	{
		m_pStatusBar->OnHandleLeftClick(this);
	}
	void OnRightMouseUp(wxMouseEvent& event)
	{
		m_pStatusBar->OnHandleRightClick(this);
	}
};

BEGIN_EVENT_TABLE(CIndicator, wxStaticBitmapEx)
EVT_LEFT_UP(CIndicator::OnLeftMouseUp)
EVT_RIGHT_UP(CIndicator::OnRightMouseUp)
END_EVENT_TABLE()

CStatusBar::CStatusBar(wxTopLevelWindow* pParent)
	: CWidgetsStatusBar(pParent)
{
	RegisterOption(OPTION_SPEEDLIMIT_ENABLE);
	RegisterOption(OPTION_SPEEDLIMIT_INBOUND);
	RegisterOption(OPTION_SPEEDLIMIT_OUTBOUND);

	m_pDataTypeIndicator = 0;
	m_pEncryptionIndicator = 0;
	m_pCertificate = 0;
	m_pSftpEncryptionInfo = 0;
	m_pSpeedLimitsIndicator = 0;

	m_size = 0;
	m_hasUnknownFiles = false;

	const int count = 3;
	SetFieldsCount(count);
	int array[count];
	array[0] = wxSB_FLAT;
	array[1] = wxSB_NORMAL;
	array[2] = wxSB_FLAT;
	SetStatusStyles(count, array);

	SetStatusWidths(count, statbarWidths);

	UpdateSizeFormat();

	UpdateSpeedLimitsIcon();
}

CStatusBar::~CStatusBar()
{
	delete m_pCertificate;
	delete m_pSftpEncryptionInfo;
}

// Defined in LocalListView.cpp
extern wxString FormatSize(const wxLongLong& size, bool add_bytes_suffix, int format, bool thousands_separator, int num_decimal_places);

void CStatusBar::DisplayQueueSize(wxLongLong totalSize, bool hasUnknown)
{
	m_size = totalSize;
	m_hasUnknownFiles = hasUnknown;

	if (totalSize == 0 && !hasUnknown)
	{
		SetStatusText(_("Queue: empty"), FIELD_QUEUESIZE);
		return;
	}

	wxString queueSize = wxString::Format(_("Queue: %s%s"), hasUnknown ? _T(">") : _T(""),
			FormatSize(totalSize, true, m_sizeFormat, m_sizeFormatThousandsSep, m_sizeFormatDecimalPlaces).c_str());

	SetStatusText(queueSize, FIELD_QUEUESIZE);
}

void CStatusBar::DisplayDataType(const CServer* const pServer)
{
	if (!pServer || !CServer::ProtocolHasDataTypeConcept(pServer->GetProtocol()))
	{
		if (m_pDataTypeIndicator)
		{
			RemoveChild(widget_datatype);
			m_pDataTypeIndicator->Destroy();
			m_pDataTypeIndicator = 0;
		}
	}
	else
	{
		wxString name;
		wxString desc;

		const int type = COptions::Get()->GetOptionVal(OPTION_ASCIIBINARY);
		if (type == 1)
		{
			name = _T("ART_ASCII");
			desc = _("Current transfer type is set to ASCII.");
		}
		else if (type == 2)
		{
			name = _T("ART_BINARY");
			desc = _("Current transfer type is set to binary.");
		}
		else
		{
			name = _T("ART_AUTO");
			desc = _("Current transfer type is set to automatic detection.");
		}

		wxBitmap bmp = wxArtProvider::GetBitmap(name, wxART_OTHER, wxSize(16, 16));
		if (!m_pDataTypeIndicator)
		{
			m_pDataTypeIndicator = new CIndicator(this, bmp);
			AddChild(0, widget_datatype, m_pDataTypeIndicator);
		}
		else
			m_pDataTypeIndicator->SetBitmap(bmp);
		m_pDataTypeIndicator->SetToolTip(desc);
	}
}

void CStatusBar::MeasureQueueSizeWidth()
{
	wxClientDC dc(this);
	dc.SetFont(GetFont());

	wxSize s = dc.GetTextExtent(_("Queue: empty"));
	
	wxString tmp = _T(">8888");
	if (m_sizeFormatDecimalPlaces)
	{
		tmp += _T(".");
		for (int i = 0; i < m_sizeFormatDecimalPlaces; i++)
			tmp += _T("8");
	}
	s.IncTo(dc.GetTextExtent(wxString::Format(_("Queue: %s MiB"), tmp.c_str())));

	SetFieldWidth(FIELD_QUEUESIZE, s.x + 10);
}

void CStatusBar::DisplayEncrypted(const CServer* const pServer)
{
	delete m_pCertificate;
	m_pCertificate = 0;
	delete m_pSftpEncryptionInfo;
	m_pSftpEncryptionInfo = 0;
	if (!pServer || (pServer->GetProtocol() != FTPS && pServer->GetProtocol() != FTPES && pServer->GetProtocol() != SFTP))
	{
		if (m_pEncryptionIndicator)
		{
			RemoveChild(widget_encryption);
			m_pEncryptionIndicator->Destroy();
			m_pEncryptionIndicator = 0;
		}
	}
	else
	{
		if (m_pEncryptionIndicator)
			return;
		wxBitmap bmp = wxArtProvider::GetBitmap(_T("ART_LOCK"), wxART_OTHER, wxSize(16, 16));
		m_pEncryptionIndicator = new CIndicator(this, bmp);
		AddChild(0, widget_encryption, m_pEncryptionIndicator);

		m_pEncryptionIndicator->SetToolTip(_("The connection is encrypted. Click icon for details."));
	}
}

void CStatusBar::UpdateSizeFormat()
{
	// 0 equals bytes, however just use IEC binary prefixes instead, 
	// exact byte counts for queue make no sense.
	m_sizeFormat = COptions::Get()->GetOptionVal(OPTION_SIZE_FORMAT);
	if (!m_sizeFormat)
		m_sizeFormat = 1;

	m_sizeFormatThousandsSep = COptions::Get()->GetOptionVal(OPTION_SIZE_USETHOUSANDSEP) != 0;
	m_sizeFormatDecimalPlaces = COptions::Get()->GetOptionVal(OPTION_SIZE_DECIMALPLACES);

	MeasureQueueSizeWidth();

	DisplayQueueSize(m_size, m_hasUnknownFiles);
}

void CStatusBar::OnHandleLeftClick(wxWindow* pWnd)
{
	if (pWnd == m_pEncryptionIndicator)
	{
		if (m_pCertificate)
		{
			CVerifyCertDialog dlg;
			dlg.ShowVerificationDialog(m_pCertificate, true);
		}
		else if (m_pSftpEncryptionInfo)
		{
			CSftpEncryptioInfoDialog dlg;
			dlg.ShowDialog(m_pSftpEncryptionInfo);
		}
		else
			wxMessageBox(_("Certificate and session data are not available yet."), _("Security information"));
	}
	else if (pWnd == m_pSpeedLimitsIndicator)
	{
		CSpeedLimitsDialog dlg;
		dlg.Run(m_pParent);
		UpdateSpeedLimitsIcon();
	}
}

void CStatusBar::OnHandleRightClick(wxWindow* pWnd)
{
	if (pWnd != m_pDataTypeIndicator)
		return;

	wxMenu* pMenu = wxXmlResource::Get()->LoadMenu(_T("ID_MENU_TRANSFER_TYPE_CONTEXT"));
	if (!pMenu)
		return;
	
	const int type = COptions::Get()->GetOptionVal(OPTION_ASCIIBINARY);
	switch (type)
	{
	case 1:
		pMenu->Check(XRCID("ID_MENU_TRANSFER_TYPE_ASCII"), true);
		break;
	case 2:
		pMenu->Check(XRCID("ID_MENU_TRANSFER_TYPE_BINARY"), true);
		break;
	default:
		pMenu->Check(XRCID("ID_MENU_TRANSFER_TYPE_AUTO"), true);
		break;
	}

	PopupMenu(pMenu);
	delete pMenu;
}

void CStatusBar::SetCertificate(CCertificateNotification* pCertificate)
{
	delete m_pSftpEncryptionInfo;
	m_pSftpEncryptionInfo = 0;

	delete m_pCertificate;
	if (pCertificate)
		m_pCertificate = new CCertificateNotification(*pCertificate);
	else
		m_pCertificate = 0;
}

void CStatusBar::SetSftpEncryptionInfo(const CSftpEncryptionNotification* pEncryptionInfo)
{
	delete m_pCertificate;
	m_pCertificate = 0;

	delete m_pSftpEncryptionInfo;
	if (pEncryptionInfo)
		m_pSftpEncryptionInfo = new CSftpEncryptionNotification(*pEncryptionInfo);
	else
		m_pSftpEncryptionInfo = 0;
}

// defined in LocalListView.cpp
// TODO: Find a better place for this
extern wxString FormatSize(const wxLongLong& size, bool add_bytes_suffix, int format, bool thousands_separator, int num_decimal_places);

void CStatusBar::UpdateSpeedLimitsIcon()
{
	bool enable = COptions::Get()->GetOptionVal(OPTION_SPEEDLIMIT_ENABLE) != 0;

	wxBitmap bmp = wxArtProvider::GetBitmap(_T("ART_SPEEDLIMITS"), wxART_OTHER, wxSize(16, 16));
	wxString tooltip;

	int downloadLimit = COptions::Get()->GetOptionVal(OPTION_SPEEDLIMIT_INBOUND);
	int uploadLimit = COptions::Get()->GetOptionVal(OPTION_SPEEDLIMIT_OUTBOUND);
	if (!enable || (!downloadLimit && !uploadLimit))
	{
		wxImage img = bmp.ConvertToImage();
		img = img.ConvertToGreyscale();
		bmp = wxBitmap(img);
		tooltip = _("Speedlimits are disabled, click to change.");
	}
	else
	{
		int fmt = m_sizeFormat;
		if (fmt == 3)
			fmt = 1;
		tooltip = _("Speedlimits are enabled, click to change.");
		tooltip += _T("\n");
		if (downloadLimit)
			tooltip += wxString::Format(_("Download limit: %s."), FormatSize(downloadLimit * 1024, false, fmt, m_sizeFormatThousandsSep, 0).c_str());
		else
			tooltip += _("Downloads unlimited.");
		tooltip += _T("\n");
		if (uploadLimit)
			tooltip += wxString::Format(_("Upload limit: %s."), FormatSize(uploadLimit * 1024, false, fmt, m_sizeFormatThousandsSep, 0).c_str());
		else
			tooltip += _("Uploads unlimited.");
	}
	
	if (!m_pSpeedLimitsIndicator)
	{
		m_pSpeedLimitsIndicator = new CIndicator(this, bmp);
		AddChild(0, widget_speedlimit, m_pSpeedLimitsIndicator);
	}
	else
	{
		m_pSpeedLimitsIndicator->SetBitmap(bmp);
	}
	m_pSpeedLimitsIndicator->SetToolTip(tooltip);
}

void CStatusBar::OnOptionChanged(int option)
{
	switch (option)
	{
	case OPTION_SPEEDLIMIT_ENABLE:
	case OPTION_SPEEDLIMIT_INBOUND:
	case OPTION_SPEEDLIMIT_OUTBOUND:
		UpdateSpeedLimitsIcon();
		break;
	}
}
