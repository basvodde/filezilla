#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_passive.h"

BEGIN_EVENT_TABLE(COptionsPagePassive, COptionsPage)
EVT_CHECKBOX(XRCID("ID_LIMITPORTS"), COptionsPagePassive::OnToggleLimitPorts)
END_EVENT_TABLE();

bool COptionsPagePassive::LoadPage()
{
	bool failure = false;
	SetCheck("ID_PASSIVE", m_pOptions->GetOptionVal(OPTION_USEPASV) != 0, failure);
	SetCheck("ID_LIMITPORTS", m_pOptions->GetOptionVal(OPTION_LIMITPORTS) != 0, failure);
	SetText("ID_LOWESTPORT", wxString::Format(_T("%d"), m_pOptions->GetOptionVal(OPTION_LIMITPORTS_LOW)), failure);
	SetText("ID_HIGHESTPORT", wxString::Format(_T("%d"), m_pOptions->GetOptionVal(OPTION_LIMITPORTS_HIGH)), failure);
	SetText("ID_EXTERNALADDRESS", m_pOptions->GetOption(OPTION_EXTERNALIP), failure);

	if (!failure)
		SetCtrlState();

	return !failure;
}

bool COptionsPagePassive::SavePage()
{
	m_pOptions->SetOption(OPTION_USEPASV, GetCheck("ID_PASSIVE") ? 1 : 0);
	m_pOptions->SetOption(OPTION_LIMITPORTS, GetCheck("ID_LIMITPORTS") ? 1 : 0);

	long tmp;
	GetText("ID_LOWESTPORT").ToLong(&tmp); m_pOptions->SetOption(OPTION_LIMITPORTS_LOW, tmp);
	GetText("ID_HIGHESTPORT").ToLong(&tmp); m_pOptions->SetOption(OPTION_LIMITPORTS_HIGH, tmp);
	
	m_pOptions->SetOption(OPTION_EXTERNALIP, GetText("ID_EXTERNALADDRESS"));

	return true;
}

bool COptionsPagePassive::Validate()
{
	// Validate port limiting settings
	if (GetCheck("ID_LIMITPORTS"))
	{
		wxTextCtrl* pLow = XRCCTRL(*this, "ID_LOWESTPORT", wxTextCtrl);
		wxASSERT(pLow);

		long low;
		if (!pLow->GetValue().ToLong(&low) || low < 1 || low > 65535)
		{
			pLow->SetFocus();
			wxMessageBox(_("Lowest available port has to be a number between 1 and 65535."), validationFailed, wxICON_EXCLAMATION);
			return false;
		}

		wxTextCtrl* pHigh = XRCCTRL(*this, "ID_LOWESTPORT", wxTextCtrl);
		wxASSERT(pHigh);

		long high;
		if (!pHigh->GetValue().ToLong(&high) || high < 1 || high > 65535)
		{
			pHigh->SetFocus();
			wxMessageBox(_("Highest available port has to be a number between 1 and 65535."), validationFailed, wxICON_EXCLAMATION);
			return false;
		}

		if (low > high)
		{
			pLow->SetFocus();
			wxMessageBox(_("The lowest available port has to be less or equal than the highest available port."), validationFailed, wxICON_EXCLAMATION);
			return false;
		}
	}
	return true;
}

void COptionsPagePassive::SetCtrlState()
{
	FindWindow(XRCID("ID_LOWESTPORT"))->Enable(GetCheck("ID_LIMITPORTS"));
	FindWindow(XRCID("ID_HIGHESTPORT"))->Enable(GetCheck("ID_LIMITPORTS"));
}

void COptionsPagePassive::OnToggleLimitPorts(wxCommandEvent& event)
{
	SetCtrlState();
}