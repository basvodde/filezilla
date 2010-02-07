#include "filezilla.h"
#include "../Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_fileexists.h"

bool COptionsPageFileExists::LoadPage()
{
	bool failure = false;

	int dlAction = m_pOptions->GetOptionVal(OPTION_FILEEXISTS_DOWNLOAD);
	if (dlAction < 0 || dlAction >= CFileExistsNotification::ACTION_COUNT)
		dlAction = 0;
	SetChoice(XRCID("ID_DOWNLOAD_ACTION"), dlAction, failure);

	int ulAction = m_pOptions->GetOptionVal(OPTION_FILEEXISTS_UPLOAD);
	if (ulAction < 0 || ulAction >= CFileExistsNotification::ACTION_COUNT)
		ulAction = 0;
	SetChoice(XRCID("ID_UPLOAD_ACTION"), ulAction, failure);

	SetCheck(XRCID("ID_ASCIIRESUME"), m_pOptions->GetOptionVal(OPTION_ASCIIRESUME) ? true : false, failure);
	
	return !failure;
}

bool COptionsPageFileExists::SavePage()
{
	int dlAction = GetChoice(XRCID("ID_DOWNLOAD_ACTION"));
	if (dlAction < 0 || dlAction >= CFileExistsNotification::ACTION_COUNT)
		dlAction = 0;
	m_pOptions->SetOption(OPTION_FILEEXISTS_DOWNLOAD, dlAction);

	int ulAction = GetChoice(XRCID("ID_UPLOAD_ACTION"));
	if (ulAction < 0 || ulAction >= CFileExistsNotification::ACTION_COUNT)
		ulAction = 0;
	m_pOptions->SetOption(OPTION_FILEEXISTS_UPLOAD, ulAction);

	m_pOptions->SetOption(OPTION_ASCIIRESUME, GetCheck(XRCID("ID_ASCIIRESUME")));
	return true;
}

bool COptionsPageFileExists::Validate()
{
	return true;
}
