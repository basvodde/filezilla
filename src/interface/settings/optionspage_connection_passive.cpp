#include <filezilla.h>
#include "../Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_connection_passive.h"

bool COptionsPageConnectionPassive::LoadPage()
{
	bool failure = false;

	const int pasv_fallback = m_pOptions->GetOptionVal(OPTION_PASVREPLYFALLBACKMODE);
	SetRCheck(XRCID("ID_PASSIVE_FALLBACK1"), pasv_fallback == 0, failure);
	SetRCheck(XRCID("ID_PASSIVE_FALLBACK2"), pasv_fallback != 0, failure);

	return !failure;
}

bool COptionsPageConnectionPassive::SavePage()
{
	m_pOptions->SetOption(OPTION_PASVREPLYFALLBACKMODE, GetRCheck(XRCID("ID_PASSIVE_FALLBACK1")) ? 0 : 1);

	return true;
}

bool COptionsPageConnectionPassive::Validate()
{
	return true;
}
