#include "FileZilla.h"
#include "statusbar.h"
#include "Options.h"

static const int statbarWidths[6] = {
	-3, 0, 0, 20, 0, 35
};
#define FIELD_QUEUESIZE 4

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

void wxStatusBarEx::AddChild(int field, wxWindow* pChild, int cx)
{
	t_statbar_child data;
	data.field = GetFieldIndex(field);
	data.pChild = pChild;
	data.cx = cx;

	m_children.push_back(data);

	PositionChild(data);
}

void wxStatusBarEx::RemoveChild(int field, wxWindow* pChild)
{
	field = GetFieldIndex(field);

	for (std::list<struct t_statbar_child>::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if (pChild != iter->pChild)
			continue;

		if (field != iter->field)
			continue;

		m_children.erase(iter);
		break;
	}
}

void wxStatusBarEx::PositionChild(const struct wxStatusBarEx::t_statbar_child& data)
{
	const wxSize size = data.pChild->GetSize();

	wxRect rect;
	GetFieldRect(data.field, rect);

	data.pChild->SetSize(rect.x + data.cx, rect.GetTop() + (rect.GetHeight() - size.x + 1) / 2, -1, -1);
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

	for (std::list<struct t_statbar_child>::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
		PositionChild(*iter);
}

CStatusBar::CStatusBar(wxTopLevelWindow* pParent)
	: wxStatusBarEx(pParent)
{
	m_pDataTypeIndicator = 0;
	m_pEncryptionIndicator = 0;

	const int count = 6;
	SetFieldsCount(count);
	int array[count];
	for (int i = 0; i < count - 2; i++)
		array[i] = wxSB_FLAT;
	array[count - 2] = wxSB_NORMAL;
	array[count - 1] = wxSB_FLAT;
	SetStatusStyles(count, array);

	SetStatusWidths(count, statbarWidths);

	MeasureQueueSizeWidth();

	UpdateSizeFormat();
}

void CStatusBar::DisplayQueueSize(wxLongLong totalSize, bool hasUnknown)
{
	if (totalSize == 0 && !hasUnknown)
	{
		SetStatusText(_("Queue: empty"), FIELD_QUEUESIZE);
		return;
	}

	int divider;
	if (m_sizeFormat == 3)
		divider = 1000;
	else
		divider = 1024;

	// We always round up. Set to true if there's a reminder
	bool r2 = false;

	int p = 0; // Exponent (2^(10p) or 10^(3p) depending on option
	while (totalSize > divider && p < 6)
	{
		const wxLongLong rr = totalSize / divider;
		if (rr * divider != totalSize)
			r2 = true;
		totalSize = rr;
		p++;
	}
	if (r2)
		totalSize++;

	wxString queueSize;
	if (!p)
		queueSize.Printf(_("Queue: %s%d bytes"), hasUnknown ? _T(">") : _T(""), totalSize.GetLo());
	else
	{
		// We stop at Exa. If someone has files bigger than that, he can afford to
		// make a donation to have this changed ;)
		const wxChar prefix[] = { ' ', 'K', 'M', 'G', 'T', 'P', 'E' };

		queueSize.Printf(_("Queue: %s%d %c"), hasUnknown ? _T(">") : _T(""), totalSize.GetLo(), prefix[p]);

		if (m_sizeFormat == 1)
			queueSize += _T("iB");
		else
			queueSize += _T("B");
	}
	SetStatusText(queueSize, FIELD_QUEUESIZE);
}

void CStatusBar::DisplayDataType(const CServer* const pServer)
{
	if (!pServer || !CServer::ProtocolHasDataTypeConcept(pServer->GetProtocol()))
	{
		if (m_pDataTypeIndicator)
		{
			RemoveChild(-4, m_pDataTypeIndicator);
			SetFieldWidth(-4, 0);
			m_pDataTypeIndicator->Destroy();
			m_pDataTypeIndicator = 0;
			Refresh();
		}
	}
	else
	{
		wxString name;
		wxString desc;

		int type = COptions::Get()->GetOptionVal(OPTION_ASCIIBINARY);
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
			desc = _("Current transfer type is to automatic detection.");
		}

		wxBitmap bmp = wxArtProvider::GetBitmap(name, wxART_OTHER, wxSize(16, 16));
		if (!m_pDataTypeIndicator)
		{
			m_pDataTypeIndicator = new wxStaticBitmap(this, wxID_ANY, bmp);
			SetFieldWidth(-4, 20);
			AddChild(-4, m_pDataTypeIndicator, 2);
		}
		else
			m_pDataTypeIndicator->SetBitmap(bmp);
		m_pDataTypeIndicator->SetToolTip(desc);

		Refresh();
	}
}

void CStatusBar::MeasureQueueSizeWidth()
{
	wxClientDC dc(this);
	dc.SetFont(GetFont());

	wxSize s = dc.GetTextExtent(_("Queue: empty"));
	s.IncTo(dc.GetTextExtent(wxString::Format(_("Queue: %s%d MiB"), ">", 8888)));
	s.IncTo(dc.GetTextExtent(wxString::Format(_("Queue: %s%d bytes"), ">", 8888)));

	SetFieldWidth(-2, s.x + 10);
}

void CStatusBar::DisplayEncrypted(const CServer* const pServer)
{
	if (!pServer || (pServer->GetProtocol() != FTPS && pServer->GetProtocol() != FTPES && pServer->GetProtocol() != SFTP))
	{
		if (m_pEncryptionIndicator)
		{
			RemoveChild(-5, m_pEncryptionIndicator);
			SetFieldWidth(-5, 0);
			m_pEncryptionIndicator->Destroy();
			m_pEncryptionIndicator = 0;
			Refresh();
		}
	}
	else
	{
		if (m_pEncryptionIndicator)
			return;
		wxBitmap bmp = wxArtProvider::GetBitmap(_T("ART_LOCK"), wxART_OTHER, wxSize(16, 16));
		m_pEncryptionIndicator = new wxStaticBitmap(this, wxID_ANY, bmp);
		SetFieldWidth(-5, 20);
		AddChild(-5, m_pEncryptionIndicator, 2);

		m_pEncryptionIndicator->SetToolTip(_T("The connection is encrypted. Click icon for details."));

		Refresh();
	}
}

void CStatusBar::UpdateSizeFormat()
{
	// 0 equals bytes, however just use IEC binary prefixes instead, 
	// exact byte counts for queue make no sense.
	m_sizeFormat = COptions::Get()->GetOptionVal(OPTION_SIZE_FORMAT);
	if (!m_sizeFormat)
		m_sizeFormat = 1;
}
