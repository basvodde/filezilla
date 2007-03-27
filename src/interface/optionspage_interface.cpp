#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_interface.h"
#include "Mainfrm.h"

BEGIN_EVENT_TABLE(COptionsPageInterface, COptionsPage)
EVT_CHECKBOX(XRCID("ID_FILEPANESWAP"), COptionsPageInterface::OnLayoutChange)
EVT_CHOICE(XRCID("ID_FILEPANELAYOUT"), COptionsPageInterface::OnLayoutChange)
END_EVENT_TABLE()

bool COptionsPageInterface::LoadPage()
{
	bool failure = false;

	SetCheck(XRCID("ID_FILEPANESWAP"), m_pOptions->GetOptionVal(OPTION_FILEPANE_SWAP) != 0, failure);

	wxChoice* pChoice = XRCCTRL(*this, "ID_FILEPANELAYOUT", wxChoice);
	if (!pChoice)
		return false;
	int selection = m_pOptions->GetOptionVal(OPTION_FILEPANE_LAYOUT);
	pChoice->SetSelection(selection);

	return !failure;
}

bool COptionsPageInterface::SavePage()
{
	m_pOptions->SetOption(OPTION_FILEPANE_SWAP, GetCheck(XRCID("ID_FILEPANESWAP")) ? 1 : 0);

	wxChoice* pChoice = XRCCTRL(*this, "ID_FILEPANELAYOUT", wxChoice);
	if (!pChoice)
		return false;
	m_pOptions->SetOption(OPTION_FILEPANE_LAYOUT, pChoice->GetSelection());

	return true;
}

bool COptionsPageInterface::Validate()
{
	return true;
}

void COptionsPageInterface::OnLayoutChange(wxCommandEvent& event)
{
	wxChoice* pChoice = XRCCTRL(*this, "ID_FILEPANELAYOUT", wxChoice);
	if (!pChoice)
		return;
	
	int layout = pChoice->GetSelection();
	int swap = GetCheck(XRCID("ID_FILEPANESWAP")) ? 1 : 0;

	m_pOwner->m_pMainFrame->UpdateLayout(layout, swap);
}
