#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_updatecheck.h"
#include "updatewizard.h"

BEGIN_EVENT_TABLE(COptionsPageUpdateCheck, COptionsPage)
EVT_BUTTON(XRCID("ID_RUNUPDATECHECK"), COptionsPageUpdateCheck::OnRunUpdateCheck)
END_EVENT_TABLE()

bool COptionsPageUpdateCheck::LoadPage()
{
	bool failure = false;
	SetCheck(XRCID("ID_UPDATECHECK"), m_pOptions->GetOptionVal(OPTION_UPDATECHECK) != 0, failure);
	SetTextFromOption(XRCID("ID_DAYS"), OPTION_UPDATECHECK_INTERVAL, failure);

	return !failure;
}

bool COptionsPageUpdateCheck::SavePage()
{
	m_pOptions->SetOption(OPTION_UPDATECHECK, GetCheck(XRCID("ID_UPDATECHECK")) ? 1 : 0);
	SetOptionFromText(XRCID("ID_DAYS"), OPTION_UPDATECHECK_INTERVAL);

	return true;
}

bool COptionsPageUpdateCheck::Validate()
{
	unsigned long days;
	if (!GetText(XRCID("ID_DAYS")).ToULong(&days) || days < 7)
	{
		XRCCTRL(*this, "ID_DAYS", wxTextCtrl)->SetFocus();
		wxMessageBox(_("Please enter an update interval of at least 7 days"), validationFailed, wxICON_EXCLAMATION, this);
		return false;
	}

	return true;
}

void COptionsPageUpdateCheck::OnRunUpdateCheck(wxCommandEvent &event)
{
	CUpdateWizard dlg(this);
	if (!dlg.Load())
		return;

	dlg.Run();
}
