#include <filezilla.h>
#include "../Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_filelists.h"
#include "../Mainfrm.h"

bool COptionsPageFilelists::LoadPage()
{
	bool failure = false;

	SetChoice(XRCID("ID_SORTMODE"), m_pOptions->GetOptionVal(OPTION_FILELIST_DIRSORT), failure);

	SetTextFromOption(XRCID("ID_COMPARISON_THRESHOLD"), OPTION_COMPARISON_THRESHOLD, failure);

	SetChoice(XRCID("ID_DOUBLECLICK_FILE"), m_pOptions->GetOptionVal(OPTION_DOUBLECLICK_ACTION_FILE), failure);
	SetChoice(XRCID("ID_DOUBLECLICK_DIRECTORY"), m_pOptions->GetOptionVal(OPTION_DOUBLECLICK_ACTION_DIRECTORY), failure);

	return !failure;
}

bool COptionsPageFilelists::SavePage()
{
	m_pOptions->SetOption(OPTION_FILELIST_DIRSORT, GetChoice(XRCID("ID_SORTMODE")));

	SetOptionFromText(XRCID("ID_COMPARISON_THRESHOLD"), OPTION_COMPARISON_THRESHOLD); 

	m_pOptions->SetOption(OPTION_DOUBLECLICK_ACTION_FILE, GetChoice(XRCID("ID_DOUBLECLICK_FILE")));
	m_pOptions->SetOption(OPTION_DOUBLECLICK_ACTION_DIRECTORY, GetChoice(XRCID("ID_DOUBLECLICK_DIRECTORY")));

	return true;
}

bool COptionsPageFilelists::Validate()
{
	wxString text = GetText(XRCID("ID_COMPARISON_THRESHOLD"));
	long minutes = 1;
	if (!text.ToLong(&minutes) || minutes < 0 || minutes > 1440)
		return DisplayError(_T("ID_COMPARISON_THRESHOLD"), _("Comparison threshold needs to be between 0 and 1440 minutes."));

	return true;
}
