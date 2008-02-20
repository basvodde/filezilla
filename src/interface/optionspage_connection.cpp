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
	SetTextFromOption(XRCID("ID_RETRIES"), OPTION_RECONNECTCOUNT, failure);
	SetTextFromOption(XRCID("ID_RETRYDELAY"), OPTION_RECONNECTDELAY, failure);
	return !failure;
}

bool COptionsPageConnection::SavePage()
{
	long tmp;
	GetText(XRCID("ID_RETRIES")).ToLong(&tmp); m_pOptions->SetOption(OPTION_RECONNECTCOUNT, tmp);
	GetText(XRCID("ID_RETRYDELAY")).ToLong(&tmp); m_pOptions->SetOption(OPTION_RECONNECTDELAY, tmp);
	return true;
}

bool COptionsPageConnection::Validate()
{
	wxTextCtrl* pRetries = XRCCTRL(*this, "ID_RETRIES", wxTextCtrl);

	long retries;
	if (!pRetries->GetValue().ToLong(&retries) || retries < 0 || retries > 99)
		return DisplayError(pRetries, _("Number of retries has to be between 0 and 99."));

	wxTextCtrl* pDelay = XRCCTRL(*this, "ID_RETRYDELAY", wxTextCtrl);

	long delay;
	if (!pDelay->GetValue().ToLong(&delay) || delay < 0 || delay > 999)
		return DisplayError(pDelay, _("Delay between failed connection attempts has to be between 1 and 999 seconds."));

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
