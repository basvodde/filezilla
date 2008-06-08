#include "FileZilla.h"

#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_updatecheck.h"
#include "updatewizard.h"

BEGIN_EVENT_TABLE(COptionsPageUpdateCheck, COptionsPage)
EVT_BUTTON(XRCID("ID_RUNUPDATECHECK"), COptionsPageUpdateCheck::OnRunUpdateCheck)
EVT_CHECKBOX(XRCID("ID_CHECKBETA"), COptionsPageUpdateCheck::OnCheckBeta)
END_EVENT_TABLE()

bool COptionsPageUpdateCheck::LoadPage()
{
	bool failure = false;
	SetCheck(XRCID("ID_UPDATECHECK"), m_pOptions->GetOptionVal(OPTION_UPDATECHECK) != 0, failure);
	SetCheck(XRCID("ID_CHECKBETA"), m_pOptions->GetOptionVal(OPTION_UPDATECHECK_CHECKBETA) != 0, failure);
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
		DisplayError(_T("ID_DAYS"), _("Please enter an update interval of at least 7 days"));
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

void COptionsPageUpdateCheck::OnCheckBeta(wxCommandEvent& event)
{
	if (event.IsChecked())
	{
		if (wxMessageBox(_("Do you really want to check for beta versions?\nUnless you want to test new features, keep using stable versions."), _("Update wizard"), wxICON_QUESTION | wxYES_NO, this) != wxYES)
		{
			XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->SetValue(0);
			return;
		}
		COptions::Get()->SetOption(OPTION_UPDATECHECK_CHECKBETA, 1);
	}
	else
		COptions::Get()->SetOption(OPTION_UPDATECHECK_CHECKBETA, 0);
}

#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK
