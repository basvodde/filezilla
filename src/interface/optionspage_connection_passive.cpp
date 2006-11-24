#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_connection_passive.h"

bool COptionsPageConnectionPassive::LoadPage()
{
	bool failure = false;

	SetRCheck(XRCID("ID_PASSIVE_FALLBACK1"), m_pOptions->GetOptionVal(OPTION_PASVREPLYFALLBACKMODE) == 0, failure);
	SetRCheck(XRCID("ID_PASSIVE_FALLBACK2"), m_pOptions->GetOptionVal(OPTION_PASVREPLYFALLBACKMODE) != 0, failure);

	return !failure;
}

bool COptionsPageConnectionPassive::SavePage()
{
	m_pOptions->SetOption(OPTION_PASVREPLYFALLBACKMODE, XRCCTRL(*this, "ID_PASSIVE_FALLBACK1", wxRadioButton)->GetValue() ? 0 : 1);

	return true;
}

bool COptionsPageConnectionPassive::Validate()
{
	return true;
}
