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
	case CFileExistsNotification::overwrite:
		SetRCheck(XRCID("ID_DL_OVERWRITE"), true, failure);
		break;
	case CFileExistsNotification::overwriteNewer:
		SetRCheck(XRCID("ID_DL_OVERWRITEIFNEWER"), true, failure);
		break;
	case CFileExistsNotification::resume:
		SetRCheck(XRCID("ID_DL_RESUME"), true, failure);
		break;
	case CFileExistsNotification::rename:
		SetRCheck(XRCID("ID_DL_RENAME"), true, failure);
		break;
	case CFileExistsNotification::skip:
		SetRCheck(XRCID("ID_DL_SKIP"), true, failure);
		break;
	default:
		SetRCheck(XRCID("ID_DL_ASK"), true, failure);
		break;
	};

	const int ulAction = m_pOptions->GetOptionVal(OPTION_FILEEXISTS_UPLOAD);
	switch (ulAction)
	{
	case CFileExistsNotification::overwrite:
		SetRCheck(XRCID("ID_UL_OVERWRITE"), true, failure);
		break;
	case CFileExistsNotification::overwriteNewer:
		SetRCheck(XRCID("ID_UL_OVERWRITEIFNEWER"), true, failure);
		break;
	case CFileExistsNotification::resume:
		SetRCheck(XRCID("ID_UL_RESUME"), true, failure);
		break;
	case CFileExistsNotification::rename:
		SetRCheck(XRCID("ID_UL_RENAME"), true, failure);
		break;
	case CFileExistsNotification::skip:
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
	enum CFileExistsNotification::OverwriteAction value;
	if (GetRCheck(XRCID("ID_DL_OVERWRITE")))
		value = CFileExistsNotification::overwrite;
	else if (GetRCheck(XRCID("ID_DL_OVERWRITEIFNEWER")))
		value = CFileExistsNotification::overwriteNewer;
	else if (GetRCheck(XRCID("ID_DL_RESUME")))
		value = CFileExistsNotification::resume;
	else if (GetRCheck(XRCID("ID_DL_RENAME")))
		value = CFileExistsNotification::rename;
	else if (GetRCheck(XRCID("ID_DL_SKIP")))
		value = CFileExistsNotification::skip;
	else
		value = CFileExistsNotification::ask;
	m_pOptions->SetOption(OPTION_FILEEXISTS_DOWNLOAD, value);

	if (GetRCheck(XRCID("ID_UL_OVERWRITE")))
		value = CFileExistsNotification::overwrite;
	else if (GetRCheck(XRCID("ID_UL_OVERWRITEIFNEWER")))
		value = CFileExistsNotification::overwriteNewer;
	else if (GetRCheck(XRCID("ID_UL_RESUME")))
		value = CFileExistsNotification::resume;
	else if (GetRCheck(XRCID("ID_UL_RENAME")))
		value = CFileExistsNotification::rename;
	else if (GetRCheck(XRCID("ID_UL_SKIP")))
		value = CFileExistsNotification::skip;
	else
		value = CFileExistsNotification::ask;
	m_pOptions->SetOption(OPTION_FILEEXISTS_UPLOAD, value);

	m_pOptions->SetOption(OPTION_ASCIIRESUME, GetCheck(XRCID("ID_ASCIIRESUME")));
	return true;
}

bool COptionsPageFileExists::Validate()
{
	return true;
}
