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
}
