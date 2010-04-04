#include <filezilla.h>
#include "../Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_interface.h"
#include "../Mainfrm.h"
#include "../power_management.h"

BEGIN_EVENT_TABLE(COptionsPageInterface, COptionsPage)
EVT_CHECKBOX(XRCID("ID_FILEPANESWAP"), COptionsPageInterface::OnLayoutChange)
EVT_CHOICE(XRCID("ID_FILEPANELAYOUT"), COptionsPageInterface::OnLayoutChange)
EVT_CHOICE(XRCID("ID_MESSAGELOGPOS"), COptionsPageInterface::OnLayoutChange)
END_EVENT_TABLE()

bool COptionsPageInterface::LoadPage()
{
	bool failure = false;

	SetCheckFromOption(XRCID("ID_FILEPANESWAP"), OPTION_FILEPANE_SWAP, failure);
	SetChoice(XRCID("ID_FILEPANELAYOUT"), m_pOptions->GetOptionVal(OPTION_FILEPANE_LAYOUT), failure);

	SetChoice(XRCID("ID_MESSAGELOGPOS"), m_pOptions->GetOptionVal(OPTION_MESSAGELOG_POSITION), failure);

#ifndef __WXMAC__
	SetCheckFromOption(XRCID("ID_MINIMIZE_TRAY"), OPTION_MINIMIZE_TRAY, failure);
#endif

	SetCheckFromOption(XRCID("ID_PREVENT_IDLESLEEP"), OPTION_PREVENT_IDLESLEEP, failure);
	
	SetCheckFromOption(XRCID("ID_SPEED_DISPLAY"), OPTION_SPEED_DISPLAY, failure);

	if (!CPowerManagement::IsSupported())
		XRCCTRL(*this, "ID_PREVENT_IDLESLEEP", wxCheckBox)->Hide();

	return !failure;
}

bool COptionsPageInterface::SavePage()
{
	SetOptionFromCheck(XRCID("ID_FILEPANESWAP"), OPTION_FILEPANE_SWAP);
	m_pOptions->SetOption(OPTION_FILEPANE_LAYOUT, GetChoice(XRCID("ID_FILEPANELAYOUT")));

	m_pOptions->SetOption(OPTION_MESSAGELOG_POSITION, GetChoice(XRCID("ID_MESSAGELOGPOS")));

#ifndef __WXMAC__
	SetOptionFromCheck(XRCID("ID_MINIMIZE_TRAY"), OPTION_MINIMIZE_TRAY);
#endif

	SetOptionFromCheck(XRCID("ID_PREVENT_IDLESLEEP"), OPTION_PREVENT_IDLESLEEP);
	
	SetOptionFromCheck(XRCID("ID_SPEED_DISPLAY"), OPTION_SPEED_DISPLAY);

	return true;
}

bool COptionsPageInterface::Validate()
{
	return true;
}

void COptionsPageInterface::OnLayoutChange(wxCommandEvent& event)
{
	int layout = GetChoice(XRCID("ID_FILEPANELAYOUT"));
	int swap = GetCheck(XRCID("ID_FILEPANESWAP")) ? 1 : 0;

	m_pOwner->m_pMainFrame->UpdateLayout(layout, swap, GetChoice(XRCID("ID_MESSAGELOGPOS")));
}
