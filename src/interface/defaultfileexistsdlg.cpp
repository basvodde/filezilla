#include "FileZilla.h"
#include "defaultfileexistsdlg.h"

enum CFileExistsNotification::OverwriteAction CDefaultFileExistsDlg::m_defaults[2] = {CFileExistsNotification::unknown, CFileExistsNotification::unknown};

CDefaultFileExistsDlg::CDefaultFileExistsDlg()
{
}

bool CDefaultFileExistsDlg::Load(wxWindow *parent, bool fromQueue)
{
	if (!wxDialogEx::Load(parent, _T("ID_DEFAULTFILEEXISTSDLG")))
		return false;

	if (fromQueue)
		XRCCTRL(*this, "ID_DESCRIPTION", wxStaticText)->SetLabel(_("Select default file exists action only for the currently selected files in the queue."));
	else
		XRCCTRL(*this, "ID_DESCRIPTION", wxStaticText)->SetLabel(_("Select default file exists action if the target file already exists. This selection is valid only for the current session."));

	WrapRecursive(this, 1.7, "DEFAULTFILEEXISTS");

	if (fromQueue)
		return true;

	SelectDefaults(&m_defaults[0], &m_defaults[1]);

	return true;
}

void CDefaultFileExistsDlg::SelectDefaults(enum CFileExistsNotification::OverwriteAction* downloadAction, enum CFileExistsNotification::OverwriteAction* uploadAction)
{
	if (downloadAction)
	{
		switch (*downloadAction)
		{
		case CFileExistsNotification::ask:
			XRCCTRL(*this, "ID_DL_ASK", wxRadioButton)->SetValue(true);
			break;	
		case CFileExistsNotification::overwrite:
			XRCCTRL(*this, "ID_DL_OVERWRITE", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::overwriteNewer:
			XRCCTRL(*this, "ID_DL_OVERWRITEIFNEWER", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::overwriteSize:
			XRCCTRL(*this, "ID_DL_OVERWRITESIZE", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::overwriteSizeOrNewer:
			XRCCTRL(*this, "ID_DL_OVERWRITESIZEORNEWER", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::resume:
			XRCCTRL(*this, "ID_DL_RESUME", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::rename:
			XRCCTRL(*this, "ID_DL_RENAME", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::skip:
			XRCCTRL(*this, "ID_DL_SKIP", wxRadioButton)->SetValue(true);
			break;
		default:
			XRCCTRL(*this, "ID_DL_DEFAULT", wxRadioButton)->SetValue(true);
			break;
		};
	}

	if (uploadAction)
	{
		switch (*uploadAction)
		{
		case CFileExistsNotification::ask:
			XRCCTRL(*this, "ID_UL_ASK", wxRadioButton)->SetValue(true);
			break;	
		case CFileExistsNotification::overwrite:
			XRCCTRL(*this, "ID_UL_OVERWRITE", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::overwriteNewer:
			XRCCTRL(*this, "ID_UL_OVERWRITEIFNEWER", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::overwriteSize:
			XRCCTRL(*this, "ID_UL_OVERWRITESIZE", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::overwriteSizeOrNewer:
			XRCCTRL(*this, "ID_UL_OVERWRITESIZEORNEWER", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::resume:
			XRCCTRL(*this, "ID_UL_RESUME", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::rename:
			XRCCTRL(*this, "ID_UL_RENAME", wxRadioButton)->SetValue(true);
			break;
		case CFileExistsNotification::skip:
			XRCCTRL(*this, "ID_UL_SKIP", wxRadioButton)->SetValue(true);
			break;
		default:
			XRCCTRL(*this, "ID_UL_DEFAULT", wxRadioButton)->SetValue(true);
			break;
		};
	}
}

enum CFileExistsNotification::OverwriteAction CDefaultFileExistsDlg::GetDefault(bool download)
{
	return m_defaults[download ? 0 : 1];
}

bool CDefaultFileExistsDlg::Run(enum CFileExistsNotification::OverwriteAction *downloadAction, enum CFileExistsNotification::OverwriteAction *uploadAction)
{
	SelectDefaults(downloadAction, uploadAction);

	// Remove one side of the dialog if not needed
	wxWindow* pDummy = XRCCTRL(*this, "ID_DUMMY", wxWindow);
	wxGridSizer* pSizer = (wxGridSizer*)pDummy->GetContainingSizer();
	if (!downloadAction && uploadAction)
	{
		pSizer->Hide((size_t)0);
		pSizer->Remove((int)0);
		pSizer->SetCols(1);
	}
	else if (downloadAction && !uploadAction)
	{
		pSizer->Hide((size_t)1);
		pSizer->Remove((int)1);
		pSizer->SetCols(1);
	}
	pDummy->Destroy();
	pSizer->SetRows(1);
	Layout();
	GetSizer()->Fit(this);

	if (ShowModal() != wxID_OK)
		return false;

	if (downloadAction || !uploadAction)
	{
		enum CFileExistsNotification::OverwriteAction action;
		if (XRCCTRL(*this, "ID_DL_ASK", wxRadioButton)->GetValue())
			action = CFileExistsNotification::ask;
		else if (XRCCTRL(*this, "ID_DL_OVERWRITE", wxRadioButton)->GetValue())
			action = CFileExistsNotification::overwrite;
		else if (XRCCTRL(*this, "ID_DL_OVERWRITEIFNEWER", wxRadioButton)->GetValue())
			action = CFileExistsNotification::overwriteNewer;
		else if (XRCCTRL(*this, "ID_DL_OVERWRITESIZE", wxRadioButton)->GetValue())
			action = CFileExistsNotification::overwriteSize;
		else if (XRCCTRL(*this, "ID_DL_OVERWRITESIZEORNEWER", wxRadioButton)->GetValue())
			action = CFileExistsNotification::overwriteSizeOrNewer;
		else if (XRCCTRL(*this, "ID_DL_RESUME", wxRadioButton)->GetValue())
			action = CFileExistsNotification::resume;
		else if (XRCCTRL(*this, "ID_DL_RENAME", wxRadioButton)->GetValue())
			action = CFileExistsNotification::rename;
		else if (XRCCTRL(*this, "ID_DL_SKIP", wxRadioButton)->GetValue())
			action = CFileExistsNotification::skip;
		else
			action = CFileExistsNotification::unknown;
	
		if (downloadAction)
			*downloadAction = action;
		else
			m_defaults[0] = action;
	}

	if (!downloadAction || uploadAction)
	{
		enum CFileExistsNotification::OverwriteAction action;
		if (XRCCTRL(*this, "ID_UL_ASK", wxRadioButton)->GetValue())
			action = CFileExistsNotification::ask;
		else if (XRCCTRL(*this, "ID_UL_OVERWRITE", wxRadioButton)->GetValue())
			action = CFileExistsNotification::overwrite;
		else if (XRCCTRL(*this, "ID_UL_OVERWRITEIFNEWER", wxRadioButton)->GetValue())
			action = CFileExistsNotification::overwriteNewer;
		else if (XRCCTRL(*this, "ID_UL_OVERWRITESIZE", wxRadioButton)->GetValue())
			action = CFileExistsNotification::overwriteSize;
		else if (XRCCTRL(*this, "ID_UL_OVERWRITESIZEORNEWER", wxRadioButton)->GetValue())
			action = CFileExistsNotification::overwriteSizeOrNewer;
		else if (XRCCTRL(*this, "ID_UL_RESUME", wxRadioButton)->GetValue())
			action = CFileExistsNotification::resume;
		else if (XRCCTRL(*this, "ID_UL_RENAME", wxRadioButton)->GetValue())
			action = CFileExistsNotification::rename;
		else if (XRCCTRL(*this, "ID_UL_SKIP", wxRadioButton)->GetValue())
			action = CFileExistsNotification::skip;
		else
			action = CFileExistsNotification::unknown;

		if (uploadAction)
			*uploadAction = action;
		else
			m_defaults[1] = action;
	}

	return true;
}

void CDefaultFileExistsDlg::SetDefault(bool download, enum CFileExistsNotification::OverwriteAction action)
{
	m_defaults[download ? 0 : 1] = action;
}
