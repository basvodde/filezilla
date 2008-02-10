#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_logging.h"

bool COptionsPageLogging::LoadPage()
{
	bool failure = false;

	SetCheck(XRCID("ID_TIMESTAMPS"), m_pOptions->GetOptionVal(OPTION_MESSAGELOG_TIMESTAMP) ? true : false, failure);

	return !failure;
}

bool COptionsPageLogging::SavePage()
{
	m_pOptions->SetOption(OPTION_MESSAGELOG_TIMESTAMP, GetCheck(XRCID("ID_TIMESTAMPS")) ? 1 : 0);

	return true;
}

bool COptionsPageLogging::Validate()
{
	return true;
}
