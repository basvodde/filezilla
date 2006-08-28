#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_debug.h"

bool COptionsPageDebug::LoadPage()
{
	bool failure = false;

	SetCheck(XRCID("ID_DEBUGMENU"), m_pOptions->GetOptionVal(OPTION_DEBUG_MENU) ? true : false, failure);

	wxChoice* pChoice = XRCCTRL(*this, "ID_DEBUGLEVEL", wxChoice);
	if (!pChoice)
		return false;
	pChoice->SetSelection(m_pOptions->GetOptionVal(OPTION_LOGGING_DEBUGLEVEL));

	SetCheck(XRCID("ID_RAWLISTING"), m_pOptions->GetOptionVal(OPTION_LOGGING_RAWLISTING) ? true : false, failure);

	return !failure;
}

bool COptionsPageDebug::SavePage()
{
	m_pOptions->SetOption(OPTION_DEBUG_MENU, GetCheck(XRCID("ID_DEBUGMENU")) ? 1 : 0);
	m_pOptions->SetOption(OPTION_LOGGING_RAWLISTING, GetCheck(XRCID("ID_RAWLISTING")) ? 1 : 0);

	wxChoice* pChoice = XRCCTRL(*this, "ID_DEBUGLEVEL", wxChoice);
	m_pOptions->SetOption(OPTION_LOGGING_DEBUGLEVEL, pChoice->GetSelection());



	return true;
}

bool COptionsPageDebug::Validate()
{
	return true;
}
