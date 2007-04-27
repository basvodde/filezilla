#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_debug.h"

bool COptionsPageDebug::LoadPage()
{
	bool failure = false;

	SetCheck(XRCID("ID_DEBUGMENU"), m_pOptions->GetOptionVal(OPTION_DEBUG_MENU) ? true : false, failure);
	SetChoice(XRCID("ID_DEBUGLEVEL"), m_pOptions->GetOptionVal(OPTION_LOGGING_DEBUGLEVEL), failure);
	SetCheck(XRCID("ID_RAWLISTING"), m_pOptions->GetOptionVal(OPTION_LOGGING_RAWLISTING) ? true : false, failure);

	return !failure;
}

bool COptionsPageDebug::SavePage()
{
	m_pOptions->SetOption(OPTION_DEBUG_MENU, GetCheck(XRCID("ID_DEBUGMENU")) ? 1 : 0);
	m_pOptions->SetOption(OPTION_LOGGING_RAWLISTING, GetCheck(XRCID("ID_RAWLISTING")) ? 1 : 0);
	m_pOptions->SetOption(OPTION_LOGGING_DEBUGLEVEL, GetChoice(XRCID("ID_DEBUGLEVEL")));

	return true;
}

bool COptionsPageDebug::Validate()
{
	return true;
}
