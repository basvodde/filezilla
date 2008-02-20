#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_connection_ftp.h"

bool COptionsPageConnectionFTP::LoadPage()
{
	bool failure = false;
	SetRCheck(XRCID("ID_PASSIVE"), m_pOptions->GetOptionVal(OPTION_USEPASV) != 0, failure);
	SetRCheck(XRCID("ID_ACTIVE"), m_pOptions->GetOptionVal(OPTION_USEPASV) == 0, failure);
	SetCheck(XRCID("ID_FALLBACK"), m_pOptions->GetOptionVal(OPTION_ALLOW_TRANSFERMODEFALLBACK) != 0, failure);
	SetCheck(XRCID("ID_USEKEEPALIVE"), m_pOptions->GetOptionVal(OPTION_FTP_SENDKEEPALIVE) != 0, failure);
	return !failure;
}

bool COptionsPageConnectionFTP::SavePage()
{
	m_pOptions->SetOption(OPTION_USEPASV, GetRCheck(XRCID("ID_PASSIVE")) ? 1 : 0);
	m_pOptions->SetOption(OPTION_ALLOW_TRANSFERMODEFALLBACK, GetCheck(XRCID("ID_FALLBACK")) ? 1 : 0);
	m_pOptions->SetOption(OPTION_FTP_SENDKEEPALIVE, GetCheck(XRCID("ID_USEKEEPALIVE")) ? 1 : 0);
	return true;
}
