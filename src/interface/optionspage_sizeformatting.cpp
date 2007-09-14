#include "FileZilla.h"

#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_sizeformatting.h"

bool COptionsPageSizeFormatting::LoadPage()
{
	bool failure = false;

	const int format = m_pOptions->GetOptionVal(OPTION_SIZE_FORMAT);
	switch (format)
	{
	case 1:
		SetRCheck(XRCID("ID_SIZEFORMAT_IEC"), true, failure);
		break;
	case 2:
		SetRCheck(XRCID("ID_SIZEFORMAT_SI_BINARY"), true, failure);
		break;
	case 3:
		SetRCheck(XRCID("ID_SIZEFORMAT_SI_DECIMAL"), true, failure);
		break;
	default:
		SetRCheck(XRCID("ID_SIZEFORMAT_BYTES"), true, failure);
		break;
	}

	SetCheck(XRCID("ID_SIZEFORMAT_SEPARATE_THOUTHANDS"), m_pOptions->GetOptionVal(OPTION_SIZE_USETHOUSANDSEP) != 0, failure);

	return !failure;
}

bool COptionsPageSizeFormatting::SavePage()
{
	int format;
	if (GetRCheck(XRCID("ID_SIZEFORMAT_IEC")))
		format = 1;
	else if (GetRCheck(XRCID("ID_SIZEFORMAT_SI_BINARY")))
		format = 2;
	else if (GetRCheck(XRCID("ID_SIZEFORMAT_SI_DECIMAL")))
		format = 3;
	else
		format = 0;

	m_pOptions->SetOption(OPTION_SIZE_FORMAT, format);

	m_pOptions->SetOption(OPTION_SIZE_USETHOUSANDSEP, GetCheck(XRCID("ID_SIZEFORMAT_SEPARATE_THOUTHANDS")) ? 1 : 0);

	return true;
}

bool COptionsPageSizeFormatting::Validate()
{
	return true;
}
