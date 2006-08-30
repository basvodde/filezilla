#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_fileexists.h"

bool COptionsPageFileExists::LoadPage()
{
	bool failure = false;

	const int dlAction = m_pOptions->GetOptionVal(OPTION_FILEEXISTS_DOWNLOAD);
	switch (dlAction)
	{
	case 1:
		SetRCheck(XRCID("ID_DL_OVERWRITE"), true, failure);
		break;
	case 2:
		SetRCheck(XRCID("ID_DL_OVERWRITEIFNEWER"), true, failure);
		break;
	case 3:
		SetRCheck(XRCID("ID_DL_RESUME"), true, failure);
		break;
	case 4:
		SetRCheck(XRCID("ID_DL_RENAME"), true, failure);
		break;
	case 5:
		SetRCheck(XRCID("ID_DL_SKIP"), true, failure);
		break;
	default:
		SetRCheck(XRCID("ID_DL_ASK"), true, failure);
		break;
	};

	const int ulAction = m_pOptions->GetOptionVal(OPTION_FILEEXISTS_UPLOAD);
	switch (ulAction)
	{
	case 1:
		SetRCheck(XRCID("ID_UL_OVERWRITE"), true, failure);
		break;
	case 2:
		SetRCheck(XRCID("ID_UL_OVERWRITEIFNEWER"), true, failure);
		break;
	case 3:
		SetRCheck(XRCID("ID_UL_RESUME"), true, failure);
		break;
	case 4:
		SetRCheck(XRCID("ID_UL_RENAME"), true, failure);
		break;
	case 5:
		SetRCheck(XRCID("ID_UL_SKIP"), true, failure);
		break;
	default:
		SetRCheck(XRCID("ID_UL_ASK"), true, failure);
		break;
	};

	SetCheck(XRCID("ID_ASCIIRESUME"), m_pOptions->GetOptionVal(OPTION_ASCIIRESUME) ? true : false, failure);
	
	return !failure;
}

bool COptionsPageFileExists::SavePage()
{
	int value;
	if (GetRCheck(XRCID("ID_DL_OVERWRITE")))
		value = 1;
	else if (GetRCheck(XRCID("ID_DL_OVERWRITEIFNEWER")))
		value = 2;
	else if (GetRCheck(XRCID("ID_DL_RESUME")))
		value = 3;
	else if (GetRCheck(XRCID("ID_DL_RENAME")))
		value = 4;
	else if (GetRCheck(XRCID("ID_DL_ASK")))
		value = 5;
	else
		value = 0;
	m_pOptions->SetOption(OPTION_FILEEXISTS_DOWNLOAD, value);

	if (GetRCheck(XRCID("ID_UL_OVERWRITE")))
		value = 1;
	else if (GetRCheck(XRCID("ID_UL_OVERWRITEIFNEWER")))
		value = 2;
	else if (GetRCheck(XRCID("ID_UL_RESUME")))
		value = 3;
	else if (GetRCheck(XRCID("ID_UL_RENAME")))
		value = 4;
	else if (GetRCheck(XRCID("ID_UL_ASK")))
		value = 5;
	else
		value = 0;
	m_pOptions->SetOption(OPTION_FILEEXISTS_UPLOAD, value);

	m_pOptions->SetOption(OPTION_ASCIIRESUME, GetCheck(XRCID("ID_ASCIIRESUME")));
	return true;
}

bool COptionsPageFileExists::Validate()
{
	return true;
}
