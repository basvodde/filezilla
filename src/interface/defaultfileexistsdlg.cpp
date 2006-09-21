#include "FileZilla.h"
#include "defaultfileexistsdlg.h"

int CDefaultFileExistsDlg::m_defaults[2] = {-1, -1};

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

	switch (m_defaults[0])
	{
	case 0:
		XRCCTRL(*this, "ID_DL_ASK", wxRadioButton)->SetValue(true);
		break;	
	case 1:
		XRCCTRL(*this, "ID_DL_OVERWRITE", wxRadioButton)->SetValue(true);
		break;
	case 2:
		XRCCTRL(*this, "ID_DL_OVERWRITEIFNEWER", wxRadioButton)->SetValue(true);
		break;
	case 3:
		XRCCTRL(*this, "ID_DL_RESUME", wxRadioButton)->SetValue(true);
		break;
	case 4:
		XRCCTRL(*this, "ID_DL_RENAME", wxRadioButton)->SetValue(true);
		break;
	case 5:
		XRCCTRL(*this, "ID_DL_SKIP", wxRadioButton)->SetValue(true);
		break;
	default:
		XRCCTRL(*this, "ID_DL_DEFAULT", wxRadioButton)->SetValue(true);
		break;
	};

	switch (m_defaults[1])
	{
	case 0:
		XRCCTRL(*this, "ID_UL_ASK", wxRadioButton)->SetValue(true);
		break;	
	case 1:
		XRCCTRL(*this, "ID_UL_OVERWRITE", wxRadioButton)->SetValue(true);
		break;
	case 2:
		XRCCTRL(*this, "ID_UL_OVERWRITEIFNEWER", wxRadioButton)->SetValue(true);
		break;
	case 3:
		XRCCTRL(*this, "ID_UL_RESUME", wxRadioButton)->SetValue(true);
		break;
	case 4:
		XRCCTRL(*this, "ID_UL_RENAME", wxRadioButton)->SetValue(true);
		break;
	case 5:
		XRCCTRL(*this, "ID_UL_SKIP", wxRadioButton)->SetValue(true);
		break;
	default:
		XRCCTRL(*this, "ID_UL_DEFAULT", wxRadioButton)->SetValue(true);
		break;
	};

	return true;
}

int CDefaultFileExistsDlg::GetDefault(bool download)
{
	return m_defaults[download ? 0 : 1];
}

bool CDefaultFileExistsDlg::Run(int *downloadAction, int *uploadAction)
{
	wxASSERT(!downloadAction || uploadAction);
	wxASSERT(!uploadAction || downloadAction);

	if (ShowModal() != wxID_OK)
		return false;

	int action;
	if (XRCCTRL(*this, "ID_DL_ASK", wxRadioButton)->GetValue())
		action = 0;
	else if (XRCCTRL(*this, "ID_DL_OVERWRITE", wxRadioButton)->GetValue())
		action = 1;
	else if (XRCCTRL(*this, "ID_DL_OVERWRITEIFNEWER", wxRadioButton)->GetValue())
		action = 2;
	else if (XRCCTRL(*this, "ID_DL_RESUME", wxRadioButton)->GetValue())
		action = 3;
	else if (XRCCTRL(*this, "ID_DL_RENAME", wxRadioButton)->GetValue())
		action = 4;
	else if (XRCCTRL(*this, "ID_DL_SKIP", wxRadioButton)->GetValue())
		action = 5;
	else
		action = -1;
	
	if (downloadAction)
		*downloadAction = action;
	else
		m_defaults[0] = action;

	if (XRCCTRL(*this, "ID_UL_ASK", wxRadioButton)->GetValue())
		action = 0;
	else if (XRCCTRL(*this, "ID_UL_OVERWRITE", wxRadioButton)->GetValue())
		action = 1;
	else if (XRCCTRL(*this, "ID_UL_OVERWRITEIFNEWER", wxRadioButton)->GetValue())
		action = 2;
	else if (XRCCTRL(*this, "ID_UL_RESUME", wxRadioButton)->GetValue())
		action = 3;
	else if (XRCCTRL(*this, "ID_UL_RENAME", wxRadioButton)->GetValue())
		action = 4;
	else if (XRCCTRL(*this, "ID_UL_SKIP", wxRadioButton)->GetValue())
		action = 5;
	else
		action = -1;

	if (uploadAction)
		*uploadAction = action;
	else
		m_defaults[1] = action;

	return true;
}

void CDefaultFileExistsDlg::SetDefault(bool download, int action)
{
	if (action < -1 || action > 5)
		action = -1;
	m_defaults[download ? 0 : 1] = action;
}
