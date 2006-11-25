#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_connection.h"
#include "netconfwizard.h"

BEGIN_EVENT_TABLE(COptionsPageConnection, COptionsPage)
EVT_BUTTON(XRCID("ID_RUNWIZARD"), COptionsPageConnection::OnWizard)
END_EVENT_TABLE()

bool COptionsPageConnection::LoadPage()
{
	bool failure = false;
	SetRCheck(XRCID("ID_PASSIVE"), m_pOptions->GetOptionVal(OPTION_USEPASV) != 0, failure);
	SetRCheck(XRCID("ID_ACTIVE"), m_pOptions->GetOptionVal(OPTION_USEPASV) == 0, failure);
	SetCheck(XRCID("ID_FALLBACK"), m_pOptions->GetOptionVal(OPTION_ALLOW_TRANSFERMODEFALLBACK) != 0, failure);

	return !failure;
}

bool COptionsPageConnection::SavePage()
{
	m_pOptions->SetOption(OPTION_USEPASV, GetRCheck(XRCID("ID_PASSIVE")) ? 1 : 0);
	m_pOptions->SetOption(OPTION_ALLOW_TRANSFERMODEFALLBACK, GetCheck(XRCID("ID_FALLBACK")) ? 1 : 0);

	return true;
}

bool COptionsPageConnection::Validate()
{
	return true;
}

void COptionsPageConnection::OnWizard(wxCommandEvent& event)
{
	CNetConfWizard wizard(GetParent(), m_pOptions);
	if (!wizard.Load())
		return;
	if (wizard.Run())
		ReloadSettings();
}
