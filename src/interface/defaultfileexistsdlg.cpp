#include "FileZilla.h"
#include "defaultfileexistsdlg.h"

int CDefaultFileExistsDlg::m_defaults[2] = {-1, -1};

CDefaultFileExistsDlg::CDefaultFileExistsDlg()
{
}

bool CDefaultFileExistsDlg::Load(wxWindow *parent)
{
	if (!wxDialogEx::Load(parent, _T("ID_DEFAULTFILEEXISTSDLG")))
		return false;

	XRCCTRL(*this, "ID_DESCRIPTION", wxStaticText)->SetLabel(_("Select default file exists action if the target file already exists. This selection is valid only for the current session."));

	WrapRecursive(this, 1.7, "DEFAULTFILEEXISTS");

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

void CDefaultFileExistsDlg::Run()
{
	ShowModal();

	if (XRCCTRL(*this, "ID_DL_ASK", wxRadioButton)->GetValue())
		m_defaults[0] = 0;
	else if (XRCCTRL(*this, "ID_DL_OVERWRITE", wxRadioButton)->GetValue())
		m_defaults[0] = 1;
	else if (XRCCTRL(*this, "ID_DL_OVERWRITEIFNEWER", wxRadioButton)->GetValue())
		m_defaults[0] = 2;
	else if (XRCCTRL(*this, "ID_DL_RESUME", wxRadioButton)->GetValue())
		m_defaults[0] = 3;
	else if (XRCCTRL(*this, "ID_DL_RENAME", wxRadioButton)->GetValue())
		m_defaults[0] = 4;
	else if (XRCCTRL(*this, "ID_DL_SKIP", wxRadioButton)->GetValue())
		m_defaults[0] = 5;
	else
		m_defaults[0] = -1;

	if (XRCCTRL(*this, "ID_UL_ASK", wxRadioButton)->GetValue())
		m_defaults[1] = 0;
	else if (XRCCTRL(*this, "ID_UL_OVERWRITE", wxRadioButton)->GetValue())
		m_defaults[1] = 1;
	else if (XRCCTRL(*this, "ID_UL_OVERWRITEIFNEWER", wxRadioButton)->GetValue())
		m_defaults[1] = 2;
	else if (XRCCTRL(*this, "ID_UL_RESUME", wxRadioButton)->GetValue())
		m_defaults[1] = 3;
	else if (XRCCTRL(*this, "ID_UL_RENAME", wxRadioButton)->GetValue())
		m_defaults[1] = 4;
	else if (XRCCTRL(*this, "ID_UL_SKIP", wxRadioButton)->GetValue())
		m_defaults[1] = 5;
	else
		m_defaults[1] = -1;
}

void CDefaultFileExistsDlg::SetDefault(bool download, int action)
{
	if (action < -1 || action > 5)
		action = -1;
	m_defaults[download ? 0 : 1] = action;
}
