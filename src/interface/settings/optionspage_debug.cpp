#include "filezilla.h"
#include "../Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_debug.h"

bool COptionsPageDebug::LoadPage()
{
	bool failure = false;

	SetCheckFromOption(XRCID("ID_DEBUGMENU"), OPTION_DEBUG_MENU, failure);
	SetCheckFromOption(XRCID("ID_RAWLISTING"), OPTION_LOGGING_RAWLISTING, failure);
	SetChoice(XRCID("ID_DEBUGLEVEL"), m_pOptions->GetOptionVal(OPTION_LOGGING_DEBUGLEVEL), failure);

	return !failure;
}

bool COptionsPageDebug::SavePage()
{
	SetOptionFromCheck(XRCID("ID_DEBUGMENU"), OPTION_DEBUG_MENU);
	SetOptionFromCheck(XRCID("ID_RAWLISTING"), OPTION_LOGGING_RAWLISTING);
	m_pOptions->SetOption(OPTION_LOGGING_DEBUGLEVEL, GetChoice(XRCID("ID_DEBUGLEVEL")));

	return true;
}

bool COptionsPageDebug::Validate()
{
	return true;
}
