#include "FileZilla.h"
#include "fileexistsdlg.h"

BEGIN_EVENT_TABLE(CFileExistsDlg, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CFileExistsDlg::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CFileExistsDlg::OnCancel)
END_EVENT_TABLE()

CFileExistsDlg::CFileExistsDlg(CFileExistsNotification *pNotification)
{
	m_pNotification = pNotification;
	m_pAction1 = m_pAction2 = m_pAction3 = m_pAction4 = m_pAction5 = 0;
	m_action = 0;
}

bool CFileExistsDlg::Create(wxWindow* parent)
{
	SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
	SetParent(parent);
	CreateControls();
	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);
	
	return TRUE;
}

void CFileExistsDlg::CreateControls()
{	
	wxXmlResource::Get()->LoadDialog(this, GetParent(), _T("ID_FILEEXISTSDLG"));
	m_pAction1 = wxDynamicCast(FindWindow(XRCID("ID_ACTION1")), wxRadioButton);
	m_pAction2 = wxDynamicCast(FindWindow(XRCID("ID_ACTION2")), wxRadioButton);
	m_pAction3 = wxDynamicCast(FindWindow(XRCID("ID_ACTION3")), wxRadioButton);
	m_pAction4 = wxDynamicCast(FindWindow(XRCID("ID_ACTION4")), wxRadioButton);
	m_pAction5 = wxDynamicCast(FindWindow(XRCID("ID_ACTION5")), wxRadioButton);

	if (m_pNotification->download)
	{
		wxStaticText *pStatText;
		
		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE1_NAME")));
		if (pStatText)
			pStatText->SetLabel(m_pNotification->localFile);
		
		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE1_SIZE")));
		if (pStatText)
		{
			if (m_pNotification->localSize != -1)
				pStatText->SetLabel(m_pNotification->localSize.ToString() + _T(" ") + _("bytes"));
			else
				pStatText->SetLabel(_("Size unknown"));
		}
		
		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE1_TIME")));
		if (pStatText)
		{
			if (m_pNotification->localTime.IsValid())
				pStatText->SetLabel(m_pNotification->localTime.Format());
			else
				pStatText->SetLabel(_("Date/time unknown"));
		}

		LoadIcon(XRCID("ID_FILE1_ICON"), m_pNotification->localFile);

		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE2_NAME")));
		if (pStatText)
			pStatText->SetLabel(m_pNotification->remotePath.GetPath() + m_pNotification->remoteFile);
		
		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE2_SIZE")));
		if (pStatText)
		{
			if (m_pNotification->remoteSize != -1)
				pStatText->SetLabel(m_pNotification->remoteSize.ToString() + _T(" ") + _("bytes"));
			else
				pStatText->SetLabel(_("Size unknown"));
		}
		
		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE2_TIME")));
		if (pStatText)
		{
			if (m_pNotification->remoteTime.IsValid())
				pStatText->SetLabel(m_pNotification->remoteTime.Format());
			else
				pStatText->SetLabel(_("Date/time unknown"));
		}

		LoadIcon(XRCID("ID_FILE2_ICON"), m_pNotification->remoteFile);

		wxCheckBox *pCheckBox = reinterpret_cast<wxCheckBox *>(FindWindow(XRCID("ID_UPDOWNONLY")));
		if (pCheckBox)
			pCheckBox->SetLabel(_("A&pply only to downloads"));
	}
	else
	{
		wxWindow *pStatText;

		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE1_NAME")));
		if (pStatText)
			pStatText->SetLabel(m_pNotification->remotePath.GetPath() + m_pNotification->remoteFile);
		
		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE1_SIZE")));
		if (pStatText)
		{
			if (m_pNotification->remoteSize != -1)
				pStatText->SetLabel(m_pNotification->remoteSize.ToString() + _T(" ") + _("bytes"));
			else
				pStatText->SetLabel(_("Size unknown"));
		}
		
		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE1_TIME")));
		if (pStatText)
		{
			if (m_pNotification->remoteTime.IsValid())
				pStatText->SetLabel(m_pNotification->remoteTime.Format());
			else
				pStatText->SetLabel(_("Date/time unknown"));
		}

		LoadIcon(XRCID("ID_FILE1_ICON"), m_pNotification->remoteFile);

		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE2_NAME")));
		if (pStatText)
			pStatText->SetLabel(m_pNotification->localFile);
		
		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE2_SIZE")));
		if (pStatText)
		{
			if (m_pNotification->localSize != -1)
				pStatText->SetLabel(m_pNotification->localSize.ToString() + _T(" ") + _("bytes"));
			else
				pStatText->SetLabel(_("Size unknown"));
		}
		
		pStatText = reinterpret_cast<wxStaticText *>(FindWindow(XRCID("ID_FILE2_TIME")));
		if (pStatText)
		{
			if (m_pNotification->localTime.IsValid())
				pStatText->SetLabel(m_pNotification->localTime.Format());
			else
				pStatText->SetLabel(_("Date/time unknown"));
		}

		LoadIcon(XRCID("ID_FILE2_ICON"), m_pNotification->localFile);

		wxCheckBox *pCheckBox = reinterpret_cast<wxCheckBox *>(FindWindow(XRCID("ID_UPDOWNONLY")));
		if (pCheckBox)
			pCheckBox->SetLabel(_("A&pply only to uploads"));
	}
}

void CFileExistsDlg::LoadIcon(int id, const wxString &file)
{
	wxStaticBitmap *pStatBmp = reinterpret_cast<wxStaticBitmap *>(FindWindow(id));
	if (!pStatBmp)
		return;
	
#ifdef __WXMSW__
	SHFILEINFO fileinfo;
	memset(&fileinfo,0,sizeof(fileinfo));
	if (SHGetFileInfo(file, FILE_ATTRIBUTE_NORMAL, &fileinfo, sizeof(fileinfo), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES))
	{
		wxBitmap bmp;
		bmp.Create(32, 32);

		wxMemoryDC *dc = new wxMemoryDC;

		wxPen pen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
		wxBrush brush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

		dc->SelectObject(bmp);

		dc->SetPen(pen);
		dc->SetBrush(brush);
		dc->DrawRectangle(0, 0, 32, 32);

		wxIcon icon;
		icon.SetHandle(fileinfo.hIcon);
		icon.SetSize(32, 32);

		dc->DrawIcon(icon, 0, 0);
		delete dc;

		pStatBmp->SetBitmap(bmp);

		return;
	}

#endif //__WXMSW__

	wxFileName fn(file);
	wxString ext = fn.GetExt();
	wxFileType *pType = wxTheMimeTypesManager->GetFileTypeFromExtension(ext);
	if (pType)
	{
		wxIconLocation loc;
		if (pType->GetIcon(&loc) && loc.IsOk())
		{
			wxLogNull *tmp = new wxLogNull;
			wxIcon icon(loc);
			delete tmp;
						
			int width = icon.GetWidth();
			int height = icon.GetHeight();
			if (width && height)
			{
				wxBitmap bmp;
				bmp.Create(icon.GetWidth(), icon.GetHeight());

				wxMemoryDC *dc = new wxMemoryDC;

				wxPen pen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
				wxBrush brush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

				dc->SelectObject(bmp);

				dc->SetPen(pen);
				dc->SetBrush(brush);
				dc->DrawRectangle(0, 0, width, height);

				dc->DrawIcon(icon, 0, 0);
				delete dc;

				pStatBmp->SetBitmap(bmp);

				return;
			}
		}
		delete pType;
	}
}

void CFileExistsDlg::OnOK(wxCommandEvent& event)
{
	if (m_pAction1 && m_pAction1->GetValue())
		m_action = 0;
	else if (m_pAction2 && m_pAction2->GetValue())
		m_action = 1;
	else if (m_pAction3 && m_pAction3->GetValue())
		m_action = 2;
	else if (m_pAction4 && m_pAction4->GetValue())
		m_action = 3;
	else if (m_pAction5 && m_pAction5->GetValue())
		m_action = 4;
	else
		m_action = 0;
	
	EndModal(wxID_OK);
}

int CFileExistsDlg::GetAction() const
{
	return m_action;
}

void CFileExistsDlg::OnCancel(wxCommandEvent& event)
{
	m_action = 4;
	EndModal(wxID_CANCEL);
}
