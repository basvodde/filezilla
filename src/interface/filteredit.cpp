#include <filezilla.h>
#include "filteredit.h"
#include "customheightlistctrl.h"
#include "window_state_manager.h"
#include "Options.h"

BEGIN_EVENT_TABLE(CFilterEditDialog, CFilterConditionsDialog)
EVT_BUTTON(XRCID("wxID_OK"), CFilterEditDialog::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CFilterEditDialog::OnCancel)
EVT_BUTTON(XRCID("ID_NEW"), CFilterEditDialog::OnNew)
EVT_BUTTON(XRCID("ID_DELETE"), CFilterEditDialog::OnDelete)
EVT_BUTTON(XRCID("ID_RENAME"), CFilterEditDialog::OnRename)
EVT_BUTTON(XRCID("ID_COPY"), CFilterEditDialog::OnCopy)
EVT_LISTBOX(XRCID("ID_FILTERS"), CFilterEditDialog::OnFilterSelect)
END_EVENT_TABLE();

CFilterEditDialog::CFilterEditDialog()
{
	m_pWindowStateManager = 0;
}

CFilterEditDialog::~CFilterEditDialog()
{
	if (m_pWindowStateManager)
	{
		m_pWindowStateManager->Remember(OPTION_FILTEREDIT_SIZE);
		delete m_pWindowStateManager;
	}
}

void CFilterEditDialog::OnOK(wxCommandEvent& event)
{
	if (!Validate())
		return;

	if (m_currentSelection != -1)
	{
		wxASSERT((unsigned int)m_currentSelection < m_filters.size());
		SaveFilter(m_filters[m_currentSelection]);
	}
	for (unsigned int i = 0; i < m_filters.size(); i++)
	{
		if (!m_filters[i].HasConditionOfType(filter_permissions) && !m_filters[i].HasConditionOfType(filter_attributes))
			continue;

		for (unsigned int j = 0; j < m_filterSets.size(); j++)
			m_filterSets[j].remote[i] = false;
	}

	EndModal(wxID_OK);
}

void CFilterEditDialog::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

bool CFilterEditDialog::Create(wxWindow* parent, const std::vector<CFilter>& filters, const std::vector<CFilterSet>& filterSets)
{
	bool has_foreign_type = false;
	for (std::vector<CFilter>::const_iterator iter = filters.begin(); iter != filters.end(); iter++)
	{
		const CFilter& filter = *iter;
		if (!filter.HasConditionOfType(filter_foreign))
			continue;

		has_foreign_type = true;
		break;
	}

	if (!Load(parent, _T("ID_EDITFILTER")))
		return false;

	int conditions = filter_name | filter_size | filter_path | filter_meta;
	if (has_foreign_type)
		conditions |= filter_foreign;
	if (!CreateListControl(conditions))
		return false;
	
	m_pFilterListCtrl = XRCCTRL(*this, "ID_FILTERS", wxListBox);
	if (!m_pFilterListCtrl)
		return false;
	m_currentSelection = -1;

	m_filters = filters;
	m_filterSets = filterSets;
	for (std::vector<CFilter>::const_iterator iter = filters.begin(); iter != filters.end(); iter++)
		m_pFilterListCtrl->Append(iter->name);

	m_pWindowStateManager = new CWindowStateManager(this);
	m_pWindowStateManager->Restore(OPTION_FILTEREDIT_SIZE, wxSize(750, 500));

	Layout();

	SetCtrlState(false);

	return true;
}

void CFilterEditDialog::SaveFilter(CFilter& filter)
{
	filter = GetFilter();

	filter.matchCase = XRCCTRL(*this, "ID_CASE", wxCheckBox)->GetValue();

	filter.filterFiles = XRCCTRL(*this, "ID_FILES", wxCheckBox)->GetValue();
	filter.filterDirs = XRCCTRL(*this, "ID_DIRS", wxCheckBox)->GetValue();

	filter.name = XRCCTRL(*this, "ID_NAME", wxTextCtrl)->GetValue();
	if (filter.name != m_pFilterListCtrl->GetString(m_currentSelection))
	{
		int oldSelection = m_currentSelection;
		m_pFilterListCtrl->Delete(oldSelection);
		m_pFilterListCtrl->Insert(filter.name, oldSelection);
		m_pFilterListCtrl->SetSelection(oldSelection);
	}
}

void CFilterEditDialog::OnNew(wxCommandEvent& event)
{
	if (m_currentSelection != -1)
	{
		if (!Validate())
			return;
		SaveFilter(m_filters[m_currentSelection]);
	}

	int index = 1;
	wxString name = _("New filter");
	wxString newName = name;
	while (m_pFilterListCtrl->FindString(newName) != wxNOT_FOUND)
		newName = wxString::Format(_T("%s (%d)"), name.c_str(), ++index);

	wxTextEntryDialog dlg(this, _("Please enter a name for the new filter."), _("Enter filter name"), newName);
	if (dlg.ShowModal() != wxID_OK)
		return;
	newName = dlg.GetValue();

	if (newName == _T(""))
	{
		wxMessageBox(_("No filter name given"), _("Cannot create new filter"), wxICON_INFORMATION);
		return;
	}

	if (m_pFilterListCtrl->FindString(newName) != wxNOT_FOUND)
	{
		wxMessageBox(_("The entered filter name already exists."), _("Duplicate filter name"), wxICON_ERROR, this);
		return;
	}

	CFilter filter;
	filter.name = newName;

	m_filters.push_back(filter);

	for (std::vector<CFilterSet>::iterator iter = m_filterSets.begin(); iter != m_filterSets.end(); iter++)
	{
		CFilterSet& set = *iter;
		set.local.push_back(false);
		set.remote.push_back(false);
	}

	int item = m_pFilterListCtrl->Append(newName);
	m_pFilterListCtrl->Select(item);
	wxCommandEvent evt;
	OnFilterSelect(evt);
}

void CFilterEditDialog::OnDelete(wxCommandEvent& event)
{
	int item = m_pFilterListCtrl->GetSelection();
	if (item == -1)
		return;

	m_currentSelection = -1;
	m_pFilterListCtrl->Delete(item);
	m_filters.erase(m_filters.begin() + item);

	// Remote filter from all filter sets
	for (std::vector<CFilterSet>::iterator iter = m_filterSets.begin(); iter != m_filterSets.end(); iter++)
	{
		CFilterSet& set = *iter;
		set.local.erase(set.local.begin() + item);
		set.remote.erase(set.remote.begin() + item);
	}

	ClearFilter(true);
	SetCtrlState(false);
}

void CFilterEditDialog::OnRename(wxCommandEvent& event)
{
	const wxString& oldName = XRCCTRL(*this, "ID_NAME", wxTextCtrl)->GetValue();
	wxTextEntryDialog *pDlg = new wxTextEntryDialog(this, _("Please enter a new name for the filter."), _("Enter filter name"), oldName);
	if (pDlg->ShowModal() != wxID_OK)
	{
		delete pDlg;
		return;
	}

	const wxString& newName = pDlg->GetValue();
	delete pDlg;

	if (newName == _T(""))
	{
		wxMessageBox(_("Empty filter names are not allowed."), _("Empty name"), wxICON_ERROR, this);
		return;
	}

	if (newName == oldName)
		return;

	if (m_pFilterListCtrl->FindString(newName) != wxNOT_FOUND)
	{
		wxMessageBox(_("The entered filter name already exists."), _("Duplicate filter name"), wxICON_ERROR, this);
		return;
	}

	m_pFilterListCtrl->Delete(m_currentSelection);
	m_pFilterListCtrl->Insert(newName, m_currentSelection);
	m_pFilterListCtrl->Select(m_currentSelection);
}

void CFilterEditDialog::OnCopy(wxCommandEvent& event)
{
	if (m_currentSelection == -1)
		return;

	if (!Validate())
		return;
	SaveFilter(m_filters[m_currentSelection]);

	CFilter filter = m_filters[m_currentSelection];

	int index = 1;
	const wxString& name = filter.name;
	wxString newName = name;
	while (m_pFilterListCtrl->FindString(newName) != wxNOT_FOUND)
		newName = wxString::Format(_T("%s (%d)"), name.c_str(), ++index);

	wxTextEntryDialog dlg(this, _("Please enter a new name for the copied filter."), _("Enter filter name"), newName);
	if (dlg.ShowModal() != wxID_OK)
		return;

	newName = dlg.GetValue();
	if (newName == _T(""))
	{
		wxMessageBox(_("Empty filter names are not allowed."), _("Empty name"), wxICON_ERROR, this);
		return;
	}

	if (m_pFilterListCtrl->FindString(newName) != wxNOT_FOUND)
	{
		wxMessageBox(_("The entered filter name already exists."), _("Duplicate filter name"), wxICON_ERROR, this);
		return;
	}

	filter.name = newName;

	m_filters.push_back(filter);

	for (std::vector<CFilterSet>::iterator iter = m_filterSets.begin(); iter != m_filterSets.end(); iter++)
	{
		CFilterSet& set = *iter;
		set.local.push_back(false);
		set.remote.push_back(false);
	}

	int item = m_pFilterListCtrl->Append(newName);
	m_pFilterListCtrl->Select(item);
	wxCommandEvent evt;
	OnFilterSelect(evt);
}

void CFilterEditDialog::OnFilterSelect(wxCommandEvent& event)
{
	int item = m_pFilterListCtrl->GetSelection();
	if (item == -1)
	{
		m_currentSelection = -1;
		SetCtrlState(false);
		return;
	}
	else
		SetCtrlState(true);

	if (item == m_currentSelection)
		return;

	if (m_currentSelection != -1)
	{
		wxASSERT((unsigned int)m_currentSelection < m_filters.size());

		if (!Validate())
			return;

		SaveFilter(m_filters[m_currentSelection]);
	}

	m_currentSelection = item;
	m_pFilterListCtrl->SetSelection(item); // In case SaveFilter has renamed an item
	CFilter filter = m_filters[item];
	EditFilter(filter);

	XRCCTRL(*this, "ID_CASE", wxCheckBox)->SetValue(filter.matchCase);

	XRCCTRL(*this, "ID_FILES", wxCheckBox)->SetValue(filter.filterFiles);
	XRCCTRL(*this, "ID_DIRS", wxCheckBox)->SetValue(filter.filterDirs);

	XRCCTRL(*this, "ID_NAME", wxTextCtrl)->SetValue(filter.name);
}

void CFilterEditDialog::SetCtrlState(bool enabled)
{
	XRCCTRL(*this, "ID_FILES", wxCheckBox)->Enable(enabled);
	XRCCTRL(*this, "ID_DIRS", wxCheckBox)->Enable(enabled);
}

const std::vector<CFilter>& CFilterEditDialog::GetFilters() const
{
	return m_filters;
}

const std::vector<CFilterSet>& CFilterEditDialog::GetFilterSets() const
{
	return m_filterSets;
}

bool CFilterEditDialog::Validate()
{
	if (m_currentSelection == -1)
		return true;

	wxString error;
	if (!ValidateFilter(error))
	{
		m_pFilterListCtrl->SetSelection(m_currentSelection);
		wxMessageBox(error, _("Filter validation failed"), wxICON_ERROR, this);
		return false;
	}

	wxString name = XRCCTRL(*this, "ID_NAME", wxTextCtrl)->GetValue();
	if (name == _T(""))
	{
		m_pFilterListCtrl->SetSelection(m_currentSelection);
		XRCCTRL(*this, "ID_NAME", wxTextCtrl)->SetFocus();
		wxMessageBox(_("Need to enter filter name"), _("Filter validation failed"), wxICON_ERROR, this);
		return false;
	}

	int pos = m_pFilterListCtrl->FindString(name);
	if (pos != wxNOT_FOUND && pos != m_currentSelection)
	{
		m_pFilterListCtrl->SetSelection(m_currentSelection);
		XRCCTRL(*this, "ID_NAME", wxTextCtrl)->SetFocus();
		wxMessageBox(_("Filter name already exists"), _("Filter validation failed"), wxICON_ERROR, this);
		return false;
	}

	return true;
}
