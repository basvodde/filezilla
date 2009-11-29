#include "FileZilla.h"
#include "../Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_connection.h"
#include "../netconfwizard.h"

BEGIN_EVENT_TABLE(COptionsPageConnection, COptionsPage)
EVT_BUTTON(XRCID("ID_RUNWIZARD"), COptionsPageConnection::OnWizard)
END_EVENT_TABLE()

bool COptionsPageConnection::LoadPage()
{
	bool failure = false;
	SetTextFromOption(XRCID("ID_RETRIES"), OPTION_RECONNECTCOUNT, failure);
	SetTextFromOption(XRCID("ID_RETRYDELAY"), OPTION_RECONNECTDELAY, failure);
	SetTextFromOption(XRCID("ID_TIMEOUT"), OPTION_TIMEOUT, failure);
	return !failure;
}

bool COptionsPageConnection::SavePage()
{
	SetIntOptionFromText(XRCID("ID_RETRIES"), OPTION_RECONNECTCOUNT);
	SetIntOptionFromText(XRCID("ID_RETRYDELAY"), OPTION_RECONNECTDELAY);
	SetOptionFromText(XRCID("ID_TIMEOUT"), OPTION_TIMEOUT);
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

	long timeout;
	wxTextCtrl *pTimeout = XRCCTRL(*this, "ID_TIMEOUT", wxTextCtrl);
	if (!pTimeout->GetValue().ToLong(&timeout) || ((timeout < 5 || timeout > 9999) && timeout != 0))
		return DisplayError(pTimeout, _("Please enter a timeout between 5 and 9999 seconds or 0 to disable timeouts."));

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
