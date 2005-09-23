#include "FileZilla.h"
#include "filter.h"
#include "filteredit.h"

BEGIN_EVENT_TABLE(CFilterDialog, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CFilterDialog::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CFilterDialog::OnCancel)
EVT_BUTTON(XRCID("ID_EDIT"), CFilterDialog::OnEdit)
END_EVENT_TABLE();

CFilterCondition::CFilterCondition()
{
	type = 0;
	condition = 0;
	value = 0;
}

// CFilterDialog implementation
bool CFilterDialog::Create(wxWindow* parent)
{
	if (!Load(parent, _T("ID_FILTER")))
		return false;

	return true;
}

void CFilterDialog::OnOK(wxCommandEvent& event)
{
	EndModal(wxID_OK);
}

void CFilterDialog::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

void CFilterDialog::OnEdit(wxCommandEvent& event)
{
	CFilterEditDialog dlg;
	if (!dlg.Create(this))
		return;
	dlg.ShowModal();
}
