#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_connection_active.h"

BEGIN_EVENT_TABLE(COptionsPageConnectionActive, COptionsPage)
EVT_CHECKBOX(XRCID("ID_LIMITPORTS"), COptionsPageConnectionActive::OnToggleLimitPorts)
END_EVENT_TABLE();

bool COptionsPageConnectionActive::LoadPage()
{
	bool failure = false;
	SetCheck(XRCID("ID_LIMITPORTS"), m_pOptions->GetOptionVal(OPTION_LIMITPORTS) != 0, failure);
	SetText(XRCID("ID_LOWESTPORT"), wxString::Format(_T("%d"), m_pOptions->GetOptionVal(OPTION_LIMITPORTS_LOW)), failure);
	SetText(XRCID("ID_HIGHESTPORT"), wxString::Format(_T("%d"), m_pOptions->GetOptionVal(OPTION_LIMITPORTS_HIGH)), failure);
	
	SetRCheck(XRCID("ID_ACTIVEMODE1"), m_pOptions->GetOptionVal(OPTION_EXTERNALIPMODE) == 0, failure);
	SetRCheck(XRCID("ID_ACTIVEMODE2"), m_pOptions->GetOptionVal(OPTION_EXTERNALIPMODE) == 1, failure);
	SetRCheck(XRCID("ID_ACTIVEMODE3"), m_pOptions->GetOptionVal(OPTION_EXTERNALIPMODE) == 2, failure);

	SetText(XRCID("ID_ACTIVEIP"), m_pOptions->GetOption(OPTION_EXTERNALIP), failure);
	SetText(XRCID("ID_ACTIVERESOLVER"), m_pOptions->GetOption(OPTION_EXTERNALIPRESOLVER), failure);
	SetCheck(XRCID("ID_NOEXTERNALONLOCAL"), m_pOptions->GetOptionVal(OPTION_NOEXTERNALONLOCAL) != 0, failure);

	if (!failure)
		SetCtrlState();

	return !failure;
}

bool COptionsPageConnectionActive::SavePage()
{
	m_pOptions->SetOption(OPTION_LIMITPORTS, GetCheck(XRCID("ID_LIMITPORTS")) ? 1 : 0);

	long tmp;
	GetText(XRCID("ID_LOWESTPORT")).ToLong(&tmp); m_pOptions->SetOption(OPTION_LIMITPORTS_LOW, tmp);
	GetText(XRCID("ID_HIGHESTPORT")).ToLong(&tmp); m_pOptions->SetOption(OPTION_LIMITPORTS_HIGH, tmp);
	
	if (XRCCTRL(*this, "ID_ACTIVEMODE1", wxRadioButton)->GetValue())
		m_pOptions->SetOption(OPTION_EXTERNALIPMODE, 0);
	else
		m_pOptions->SetOption(OPTION_EXTERNALIPMODE, XRCCTRL(*this, "ID_ACTIVEMODE2", wxRadioButton)->GetValue() ? 1 : 2);

	m_pOptions->SetOption(OPTION_EXTERNALIP, XRCCTRL(*this, "ID_ACTIVEIP", wxTextCtrl)->GetValue());
	m_pOptions->SetOption(OPTION_EXTERNALIPRESOLVER, XRCCTRL(*this, "ID_ACTIVERESOLVER", wxTextCtrl)->GetValue());
	m_pOptions->SetOption(OPTION_NOEXTERNALONLOCAL, XRCCTRL(*this, "ID_NOEXTERNALONLOCAL", wxCheckBox)->GetValue());

	return true;
}

bool COptionsPageConnectionActive::Validate()
{
	// Validate port limiting settings
	if (GetCheck(XRCID("ID_LIMITPORTS")))
	{
		wxTextCtrl* pLow = XRCCTRL(*this, "ID_LOWESTPORT", wxTextCtrl);
		wxASSERT(pLow);

		long low;
		if (!pLow->GetValue().ToLong(&low) || low < 1024 || low > 65535)
		{
			pLow->SetFocus();
			wxMessageBox(_("Lowest available port has to be a number between 1024 and 65535."), validationFailed, wxICON_EXCLAMATION, this);
			return false;
		}

		wxTextCtrl* pHigh = XRCCTRL(*this, "ID_LOWESTPORT", wxTextCtrl);
		wxASSERT(pHigh);

		long high;
		if (!pHigh->GetValue().ToLong(&high) || high < 1024 || high > 65535)
		{
			pHigh->SetFocus();
			wxMessageBox(_("Highest available port has to be a number between 1024 and 65535."), validationFailed, wxICON_EXCLAMATION, this);
			return false;
		}

		if (low > high)
		{
			pLow->SetFocus();
			wxMessageBox(_("The lowest available port has to be less or equal than the highest available port."), validationFailed, wxICON_EXCLAMATION, this);
			return false;
		}
	}
	return true;
}

void COptionsPageConnectionActive::SetCtrlState()
{
	FindWindow(XRCID("ID_LOWESTPORT"))->Enable(GetCheck(XRCID("ID_LIMITPORTS")));
	FindWindow(XRCID("ID_HIGHESTPORT"))->Enable(GetCheck(XRCID("ID_LIMITPORTS")));
}

void COptionsPageConnectionActive::OnToggleLimitPorts(wxCommandEvent& event)
{
	SetCtrlState();
}
