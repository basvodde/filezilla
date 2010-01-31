#include "FileZilla.h"
#include "speedlimits_dialog.h"
#include "Options.h"

BEGIN_EVENT_TABLE(CSpeedLimitsDialog, wxDialogEx)
EVT_BUTTON(wxID_OK, CSpeedLimitsDialog::OnOK)
EVT_CHECKBOX(XRCID("ID_ENABLE_SPEEDLIMITS"), CSpeedLimitsDialog::OnToggleEnable)
END_EVENT_TABLE()

void CSpeedLimitsDialog::Run(wxWindow* parent)
{
	if (!Load(parent, _T("ID_SPEEDLIMITS")))
		return;

	bool enable = COptions::Get()->GetOptionVal(OPTION_SPEEDLIMIT_ENABLE) != 0;

	XRCCTRL(*this, "ID_ENABLE_SPEEDLIMITS", wxCheckBox)->SetValue(enable);

	wxTextCtrl* pCtrl = XRCCTRL(*this, "ID_DOWNLOADLIMIT", wxTextCtrl);
	pCtrl->Enable(enable);
	pCtrl->SetMaxLength(9);
	pCtrl->ChangeValue(COptions::Get()->GetOption(OPTION_SPEEDLIMIT_INBOUND));

	pCtrl = XRCCTRL(*this, "ID_UPLOADLIMIT", wxTextCtrl);
	pCtrl->Enable(enable);
	pCtrl->SetMaxLength(9);
	pCtrl->ChangeValue(COptions::Get()->GetOption(OPTION_SPEEDLIMIT_OUTBOUND));

	ShowModal();
}

void CSpeedLimitsDialog::OnOK(wxCommandEvent& event)
{
	long download, upload;
	wxTextCtrl* pCtrl = XRCCTRL(*this, "ID_DOWNLOADLIMIT", wxTextCtrl);
	if (!pCtrl->GetValue().ToLong(&download) || (download < 0))
	{
		wxMessageBox(_("Please enter a download speedlimit greater or equal to 0 KB/s."), _("Speed Limits"), wxOK, this);
		return;
	}

	pCtrl = XRCCTRL(*this, "ID_UPLOADLIMIT", wxTextCtrl);
	if (!pCtrl->GetValue().ToLong(&upload) || (upload < 0))
	{
		wxMessageBox(_("Please enter a upload speedlimit greater or equal to 0 KB/s."), _("Speed Limits"), wxOK, this);
		return;
	}

	COptions::Get()->SetOption(OPTION_SPEEDLIMIT_INBOUND, download);
	COptions::Get()->SetOption(OPTION_SPEEDLIMIT_OUTBOUND, upload);

	COptions::Get()->SetOption(OPTION_SPEEDLIMIT_ENABLE, XRCCTRL(*this, "ID_ENABLE_SPEEDLIMITS", wxCheckBox)->GetValue() ? 1 : 0);

	EndDialog(wxID_OK);
}

void CSpeedLimitsDialog::OnToggleEnable(wxCommandEvent& event)
{
	XRCCTRL(*this, "ID_DOWNLOADLIMIT", wxTextCtrl)->Enable(event.IsChecked());
	XRCCTRL(*this, "ID_UPLOADLIMIT", wxTextCtrl)->Enable(event.IsChecked());
}
