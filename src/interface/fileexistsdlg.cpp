#include "FileZilla.h"
#include "fileexistsdlg.h"

BEGIN_EVENT_TABLE( CFileExistsDlg, wxDialog )
END_EVENT_TABLE()

CFileExistsDlg::CFileExistsDlg(CFileExistsNotification *pNotification)
{
	m_pNotification = pNotification;
	m_pAction = 0;
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
	if (FindWindow(XRCID("ID_ACTION1")))
		 m_pAction = wxDynamicCast(FindWindow(XRCID("ID_ACTION1")), wxRadioButton);

	if (m_pNotification->download)
	{
		wxWindow *pWnd;
		
		pWnd = FindWindow(XRCID("ID_FILE1_NAME"));
		if (pWnd)
			pWnd->SetTitle(m_pNotification->localFile);
		
		pWnd = FindWindow(XRCID("ID_FILE1_SIZE"));
		if (pWnd)
		{
			if (m_pNotification->localSize != -1)
				pWnd->SetTitle(m_pNotification->localSize.ToString() + _T(" ") + _("bytes"));
			else
				pWnd->SetTitle(_("Size unknown"));
		}
		
		pWnd = FindWindow(XRCID("ID_FILE1_TIME"));
		if (pWnd)
		{
			if (m_pNotification->localTime.IsValid())
				pWnd->SetTitle(m_pNotification->localTime.Format());
			else
				pWnd->SetTitle(_("Date/time unknown"));
		}

		LoadIcon(XRCID("ID_FILE1_ICON"), m_pNotification->localFile);

		pWnd = FindWindow(XRCID("ID_FILE2_NAME"));
		if (pWnd)
			pWnd->SetTitle(m_pNotification->remotePath.GetPath() + m_pNotification->remoteFile);
		
		pWnd = FindWindow(XRCID("ID_FILE2_SIZE"));
		if (pWnd)
		{
			if (m_pNotification->remoteSize != -1)
				pWnd->SetTitle(m_pNotification->remoteSize.ToString() + _T(" ") + _("bytes"));
			else
				pWnd->SetTitle(_("Size unknown"));
		}
		
		pWnd = FindWindow(XRCID("ID_FILE2_TIME"));
		if (pWnd)
		{
			if (m_pNotification->remoteTime.IsValid())
				pWnd->SetTitle(m_pNotification->remoteTime.Format());
			else
				pWnd->SetTitle(_("Date/time unknown"));
		}

		LoadIcon(XRCID("ID_FILE2_ICON"), "c:\\programme\\filezilla\\filezilla.exe");//m_pNotification->remoteFile);

		pWnd = FindWindow(XRCID("ID_UPDOWNONLY"));
		if (pWnd)
			pWnd->SetTitle(_("A&pply only to downloads"));
	}
	else
	{
		wxWindow *pWnd;

		pWnd = FindWindow(XRCID("ID_FILE1_NAME"));
		if (pWnd)
			pWnd->SetTitle(m_pNotification->remotePath.GetPath() + m_pNotification->remoteFile);
		
		pWnd = FindWindow(XRCID("ID_FILE1_SIZE"));
		if (pWnd)
		{
			if (m_pNotification->remoteSize != -1)
				pWnd->SetTitle(m_pNotification->remoteSize.ToString() + _T(" ") + _("bytes"));
			else
				pWnd->SetTitle(_("Size unknown"));
		}
		
		pWnd = FindWindow(XRCID("ID_FILE1_TIME"));
		if (pWnd)
		{
			if (m_pNotification->remoteTime.IsValid())
				pWnd->SetTitle(m_pNotification->remoteTime.Format());
			else
				pWnd->SetTitle(_("Date/time unknown"));
		}

		LoadIcon(XRCID("ID_FILE1_ICON"), m_pNotification->remoteFile);

		pWnd = FindWindow(XRCID("ID_FILE2_NAME"));
		if (pWnd)
			pWnd->SetTitle(m_pNotification->localFile);
		
		pWnd = FindWindow(XRCID("ID_FILE2_SIZE"));
		if (pWnd)
		{
			if (m_pNotification->localSize != -1)
				pWnd->SetTitle(m_pNotification->localSize.ToString() + _T(" ") + _("bytes"));
			else
				pWnd->SetTitle(_("Size unknown"));
		}
		
		pWnd = FindWindow(XRCID("ID_FILE2_TIME"));
		if (pWnd)
		{
			if (m_pNotification->localTime.IsValid())
				pWnd->SetTitle(m_pNotification->localTime.Format());
			else
				pWnd->SetTitle(_("Date/time unknown"));
		}

		LoadIcon(XRCID("ID_FILE2_ICON"), m_pNotification->localFile);

		pWnd = FindWindow(XRCID("ID_UPDOWNONLY"));
		if (pWnd)
			pWnd->SetTitle(_("A&pply only to uploads"));
	}
}

void CFileExistsDlg::LoadIcon(int id, const wxString &file)
{
	wxStaticBitmap *pStatBmp = reinterpret_cast<wxStaticBitmap *>(FindWindow(id));
	if (!pStatBmp)
		return;
	
#ifdef __WXMSW__457654
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

	wxString ext;
	int pos = file.Find('.', true);
	if (pos != -1)
		ext = file.Mid(pos + 1);
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
	}
}