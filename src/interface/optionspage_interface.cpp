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
	SetChoice(XRCID("ID_FILEPANELAYOUT"), m_pOptions->GetOptionVal(OPTION_FILEPANE_LAYOUT), failure);
	SetChoice(XRCID("ID_SORTMODE"), m_pOptions->GetOptionVal(OPTION_FILELIST_DIRSORT), failure);

	return !failure;
}

bool COptionsPageInterface::SavePage()
{
	m_pOptions->SetOption(OPTION_FILEPANE_SWAP, GetCheck(XRCID("ID_FILEPANESWAP")) ? 1 : 0);
	m_pOptions->SetOption(OPTION_FILEPANE_LAYOUT, GetChoice(XRCID("ID_FILEPANELAYOUT")));
	m_pOptions->SetOption(OPTION_FILELIST_DIRSORT, GetChoice(XRCID("ID_SORTMODE")));

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

	m_pOwner->m_pMainFrame->UpdateLayout(layout, swap);
}
