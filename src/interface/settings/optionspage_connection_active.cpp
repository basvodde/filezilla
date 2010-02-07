#include <filezilla.h>
#include "../Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_connection_active.h"

BEGIN_EVENT_TABLE(COptionsPageConnectionActive, COptionsPage)
EVT_CHECKBOX(XRCID("ID_LIMITPORTS"), COptionsPageConnectionActive::OnRadioOrCheckEvent)
EVT_RADIOBUTTON(XRCID("ID_ACTIVEMODE1"), COptionsPageConnectionActive::OnRadioOrCheckEvent)
EVT_RADIOBUTTON(XRCID("ID_ACTIVEMODE2"), COptionsPageConnectionActive::OnRadioOrCheckEvent)
EVT_RADIOBUTTON(XRCID("ID_ACTIVEMODE3"), COptionsPageConnectionActive::OnRadioOrCheckEvent)
END_EVENT_TABLE();

bool COptionsPageConnectionActive::LoadPage()
{
	bool failure = false;
	SetCheckFromOption(XRCID("ID_LIMITPORTS"), OPTION_LIMITPORTS, failure);
	SetTextFromOption(XRCID("ID_LOWESTPORT"), OPTION_LIMITPORTS_LOW, failure);
	SetTextFromOption(XRCID("ID_HIGHESTPORT"), OPTION_LIMITPORTS_HIGH, failure);
	
	SetRCheck(XRCID("ID_ACTIVEMODE1"), m_pOptions->GetOptionVal(OPTION_EXTERNALIPMODE) == 0, failure);
	SetRCheck(XRCID("ID_ACTIVEMODE2"), m_pOptions->GetOptionVal(OPTION_EXTERNALIPMODE) == 1, failure);
	SetRCheck(XRCID("ID_ACTIVEMODE3"), m_pOptions->GetOptionVal(OPTION_EXTERNALIPMODE) == 2, failure);

	SetTextFromOption(XRCID("ID_ACTIVEIP"), OPTION_EXTERNALIP, failure);
	SetTextFromOption(XRCID("ID_ACTIVERESOLVER"), OPTION_EXTERNALIPRESOLVER, failure);
	SetCheckFromOption(XRCID("ID_NOEXTERNALONLOCAL"), OPTION_NOEXTERNALONLOCAL, failure);

	if (!failure)
		SetCtrlState();

	return !failure;
}

bool COptionsPageConnectionActive::SavePage()
{
	SetOptionFromCheck(XRCID("ID_LIMITPORTS"), OPTION_LIMITPORTS);
	
	SetIntOptionFromText(XRCID("ID_LOWESTPORT"), OPTION_LIMITPORTS_LOW);
	SetIntOptionFromText(XRCID("ID_HIGHESTPORT"), OPTION_LIMITPORTS_HIGH);
	
	int mode;
	if (GetRCheck(XRCID("ID_ACTIVEMODE1")))
		mode = 0;
	else
		mode = GetRCheck(XRCID("ID_ACTIVEMODE2")) ? 1 : 2;
	m_pOptions->SetOption(OPTION_EXTERNALIPMODE, mode);

	if (mode == 1)
		SetOptionFromText(XRCID("ID_ACTIVEIP"), OPTION_EXTERNALIP);
	else if (mode == 2)
		SetOptionFromText(XRCID("ID_ACTIVERESOLVER"), OPTION_EXTERNALIPRESOLVER);
	SetOptionFromCheck(XRCID("ID_NOEXTERNALONLOCAL"), OPTION_NOEXTERNALONLOCAL);

	return true;
}

bool COptionsPageConnectionActive::Validate()
{
	// Validate port limiting settings
	if (GetCheck(XRCID("ID_LIMITPORTS")))
	{
		wxTextCtrl* pLow = XRCCTRL(*this, "ID_LOWESTPORT", wxTextCtrl);

		long low;
		if (!pLow->GetValue().ToLong(&low) || low < 1024 || low > 65535)
			return DisplayError(pLow, _("Lowest available port has to be a number between 1024 and 65535."));

		wxTextCtrl* pHigh = XRCCTRL(*this, "ID_LOWESTPORT", wxTextCtrl);

		long high;
		if (!pHigh->GetValue().ToLong(&high) || high < 1024 || high > 65535)
			return DisplayError(pHigh, _("Highest available port has to be a number between 1024 and 65535."));

		if (low > high)
			return DisplayError(pLow, _("The lowest available port has to be less or equal than the highest available port."));
	}

	int mode;
	if (GetRCheck(XRCID("ID_ACTIVEMODE1")))
		mode = 0;
	else
		mode = GetRCheck(XRCID("ID_ACTIVEMODE2")) ? 1 : 2;

	if (mode == 1)
	{
		wxTextCtrl* pActiveIP = XRCCTRL(*this, "ID_ACTIVEIP", wxTextCtrl);
		if (!IsIpAddress(pActiveIP->GetValue()))
			return DisplayError(pActiveIP, _("You have to enter a valid IP address."));
	}
	
	return true;
}

void COptionsPageConnectionActive::SetCtrlState()
{
	FindWindow(XRCID("ID_LOWESTPORT"))->Enable(GetCheck(XRCID("ID_LIMITPORTS")));
	FindWindow(XRCID("ID_HIGHESTPORT"))->Enable(GetCheck(XRCID("ID_LIMITPORTS")));

	int mode;
	if (GetRCheck(XRCID("ID_ACTIVEMODE1")))
		mode = 0;
	else
		mode = GetRCheck(XRCID("ID_ACTIVEMODE2")) ? 1 : 2;
	FindWindow(XRCID("ID_ACTIVEIP"))->Enable(mode == 1);
	FindWindow(XRCID("ID_ACTIVERESOLVER"))->Enable(mode == 2);

	FindWindow(XRCID("ID_NOEXTERNALONLOCAL"))->Enable(mode != 0);	
}

void COptionsPageConnectionActive::OnRadioOrCheckEvent(wxCommandEvent& event)
{
	SetCtrlState();
}
