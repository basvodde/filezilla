#include "FileZilla.h"
#include "filteredit.h"
#include "customheightlistctrl.h"

const wxString filterTypes[] = { _("Filename"), _("Filesize") };
const wxString stringConditionTypes[] = { _("contains"), _("is equal to"), _("begins with"), _("ends with"), _("matches regex") };
const wxString sizeConditionTypes[] = { _("greater than"), _("equals"), _("less than") };

CFilterControls::CFilterControls()
{
	pType = 0;
	pCondition = 0;
	pValue = 0;
}

BEGIN_EVENT_TABLE(CFilterEditDialog, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CFilterEditDialog::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CFilterEditDialog::OnCancel)
EVT_BUTTON(XRCID("ID_MORE"), CFilterEditDialog::OnMore)
EVT_BUTTON(XRCID("ID_REMOVE"), CFilterEditDialog::OnRemove)
EVT_CHOICE(wxID_ANY, CFilterEditDialog::OnFilterTypeChange)
EVT_BUTTON(XRCID("ID_NEW"), CFilterEditDialog::OnNew)
EVT_BUTTON(XRCID("ID_DELETE"), CFilterEditDialog::OnDelete)
EVT_BUTTON(XRCID("ID_RENAME"), CFilterEditDialog::OnRename)
EVT_BUTTON(XRCID("ID_COPY"), CFilterEditDialog::OnCopy)
EVT_LISTBOX(XRCID("ID_FILTERS"), CFilterEditDialog::OnFilterSelect)
END_EVENT_TABLE();

void CFilterEditDialog::OnOK(wxCommandEvent& event)
{
	if (!Validate())
		return;

	if (m_currentSelection != -1)
	{
		wxASSERT((unsigned int)m_currentSelection < m_filters.size());
		SaveFilter(m_filters[m_currentSelection]);
	}

	EndModal(wxID_OK);
}

void CFilterEditDialog::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

bool CFilterEditDialog::Create(wxWindow* parent, const std::vector<CFilter>& filters, const std::vector<CFilterSet>& filterSets)
{
	if (!Load(parent, _T("ID_EDITFILTER")))
		return false;

	wxScrolledWindow* wnd = XRCCTRL(*this, "ID_CONDITIONS", wxScrolledWindow);
	m_pListCtrl = new wxCustomHeightListCtrl(wnd, wxID_ANY, wxDefaultPosition, wnd->GetSize(), wxVSCROLL|wxSUNKEN_BORDER);
	if (!m_pListCtrl)
		return false;

	m_choiceBoxHeight = 0;

	m_pFilterListCtrl = XRCCTRL(*this, "ID_FILTERS", wxListBox);
	if (!m_pFilterListCtrl)
		return false;
	m_currentSelection = -1;

	m_filters = filters;
	m_filterSets = filterSets;
	for (std::vector<CFilter>::const_iterator iter = filters.begin(); iter != filters.end(); iter++)
		m_pFilterListCtrl->Append(iter->name);

	SetCtrlState(false);

	return true;
}

void CFilterEditDialog::OnMore(wxCommandEvent& event)
{
	CFilterCondition cond;
	m_currentFilter.filters.push_back(cond);

	MakeControls(cond);
	UpdateCount();
}

void CFilterEditDialog::OnRemove(wxCommandEvent& event)
{
	std::set<int> selected = m_pListCtrl->GetSelection();

	std::vector<CFilterControls> filterControls = m_filterControls;
	m_filterControls.clear();

	std::vector<CFilterCondition> filters = m_currentFilter.filters;
	m_currentFilter.filters.clear();
	
	int deleted = 0;
	for (unsigned int i = 0; i < filterControls.size(); i++)
	{
		CFilterControls& controls = filterControls[i];
		if (selected.find(i) == selected.end())
		{
			m_filterControls.push_back(controls);
			m_currentFilter.filters.push_back(filters[i]);

			// Reposition controls
			wxPoint pos;
			
			pos = controls.pType->GetPosition();
			pos.y -= deleted * (m_choiceBoxHeight + 6);
			controls.pType->SetPosition(pos);

			pos = controls.pCondition->GetPosition();
			pos.y -= deleted * (m_choiceBoxHeight + 6);
			controls.pCondition->SetPosition(pos);

			pos = controls.pValue->GetPosition();
			pos.y -= deleted * (m_choiceBoxHeight + 6);
			controls.pValue->SetPosition(pos);

			controls.pType->SetId(i - deleted);
		}
		else
		{
			delete controls.pType;
			delete controls.pCondition;
			delete controls.pValue;
			deleted++;
		}
	}

	m_pListCtrl->ClearSelection();
	UpdateCount();
}

void CFilterEditDialog::ShowFilter(const CFilter& filter)
{
	DestroyControls();

	// Create new controls
	m_currentFilter = filter;
	for (unsigned int i = 0; i < filter.filters.size(); i++)
	{
		const CFilterCondition& cond = filter.filters[i];

		MakeControls(cond);
	}

	XRCCTRL(*this, "ID_MATCHALL", wxRadioButton)->SetValue(filter.matchType == CFilter::all);
	XRCCTRL(*this, "ID_MATCHANY", wxRadioButton)->SetValue(filter.matchType == CFilter::any);
	XRCCTRL(*this, "ID_MATCHNONE", wxRadioButton)->SetValue(filter.matchType == CFilter::none);

	XRCCTRL(*this, "ID_CASE", wxCheckBox)->SetValue(filter.matchCase);

	XRCCTRL(*this, "ID_FILES", wxCheckBox)->SetValue(filter.filterFiles);
	XRCCTRL(*this, "ID_DIRS", wxCheckBox)->SetValue(filter.filterDirs);

	XRCCTRL(*this, "ID_NAME", wxTextCtrl)->SetValue(filter.name);

	UpdateCount();
}

void CFilterEditDialog::OnFilterTypeChange(wxCommandEvent& event)
{
	int item = event.GetId();
	if (item < 0 || item >= (int)m_filterControls.size())
		return;

	CFilterCondition& filter = m_currentFilter.filters[item];

	int type = event.GetSelection();
	if (type == filter.type)
		return;

	CFilterControls& controls = m_filterControls[item];
	controls.pCondition->Clear();
	if (type == 1)
	{
		controls.pCondition->Append(wxArrayString(sizeof(sizeConditionTypes) / sizeof(wxString), sizeConditionTypes));
		controls.pCondition->Select(1);
	}
	else
	{
		controls.pCondition->Append(wxArrayString(sizeof(stringConditionTypes) / sizeof(wxString), stringConditionTypes));
		controls.pCondition->Select(0);
	}

	controls.pValue->SetValue(_T(""));

	filter.type = type;
}

void CFilterEditDialog::MakeControls(const CFilterCondition& condition)
{
	wxRect size = m_pListCtrl->GetClientSize();

	// Get correct coordinates
	int posy = m_filterControls.size() * (m_choiceBoxHeight + 6) + 3;
	int posx = 0, x, y;
	m_pListCtrl->CalcScrolledPosition(posx, posy, &x, &y);
	posy = y;

	CFilterControls controls;
	controls.pType = new wxChoice();
	controls.pType->Create(m_pListCtrl, m_filterControls.size(), wxPoint(10, posy), wxDefaultSize, sizeof(filterTypes) / sizeof(wxString), filterTypes);
	controls.pType->Select(condition.type);
	wxRect typeRect = controls.pType->GetSize();

	if (!m_choiceBoxHeight)
	{
		wxSize size = controls.pType->GetSize();
		m_choiceBoxHeight = size.GetHeight();
		m_pListCtrl->SetLineHeight(m_choiceBoxHeight + 6);
	}

	controls.pCondition = new wxChoice();
	if (condition.type != 1)
		controls.pCondition->Create(m_pListCtrl, wxID_ANY, wxPoint(20 + typeRect.GetWidth(), posy), wxDefaultSize, sizeof(stringConditionTypes) / sizeof(wxString), stringConditionTypes);
	else
		controls.pCondition->Create(m_pListCtrl, wxID_ANY, wxPoint(20 + typeRect.GetWidth(), posy), wxDefaultSize, sizeof(sizeConditionTypes) / sizeof(wxString), sizeConditionTypes);
	controls.pCondition->Select(condition.condition);
	wxRect conditionsRect = controls.pCondition->GetSize();

	int textWidth = 30 + typeRect.GetWidth() + conditionsRect.GetWidth();
	controls.pValue = new wxTextCtrl();
	controls.pValue->Create(m_pListCtrl, wxID_ANY, _T(""), wxPoint(textWidth, posy), wxSize(size.GetWidth() - textWidth - 10, -1));	
	controls.pValue->SetValue(condition.strValue);

	m_filterControls.push_back(controls);
}

void CFilterEditDialog::UpdateCount()
{
	wxSize oldSize = m_pListCtrl->GetClientSize();
	m_pListCtrl->SetLineCount(m_filterControls.size());
	wxSize newSize = m_pListCtrl->GetClientSize();

	if (oldSize.GetWidth() != newSize.GetWidth())
	{
		int deltaX = newSize.GetWidth() - oldSize.GetWidth();

		// Resize text fields
		for (unsigned int i = 0; i < m_filterControls.size(); i++)
		{
			CFilterControls& controls = m_filterControls[i];
			if (!controls.pValue)
				continue;

			wxSize size = controls.pValue->GetSize();
			size.SetWidth(size.GetWidth() + deltaX);
			controls.pValue->SetSize(size);
		}
	}
}

void CFilterEditDialog::SaveFilter(CFilter& filter)
{
	if (XRCCTRL(*this, "ID_MATCHANY", wxRadioButton)->GetValue())
		filter.matchType = CFilter::any;
	else if (XRCCTRL(*this, "ID_MATCHNONE", wxRadioButton)->GetValue())
		filter.matchType = CFilter::none;
	else
		filter.matchType = CFilter::all;

	filter.matchCase = XRCCTRL(*this, "ID_CASE", wxCheckBox)->GetValue();

	filter.filters.clear();
	for (unsigned int i = 0; i < m_currentFilter.filters.size(); i++)
	{
		const CFilterControls& controls = m_filterControls[i];
		CFilterCondition condition = m_currentFilter.filters[i];
		
		condition.type = controls.pType->GetSelection();
		condition.condition = controls.pCondition->GetSelection();
		condition.strValue = controls.pValue->GetValue();

		// TODO: 64bit filesize
		if (condition.type == 1)
		{
			unsigned long tmp;
			condition.strValue.ToULong(&tmp);
			condition.value = tmp;
		}

		condition.matchCase = filter.matchCase;

		filter.filters.push_back(condition);
	}

	filter.filterFiles = XRCCTRL(*this, "ID_FILES", wxCheckBox)->GetValue();
	filter.filterDirs = XRCCTRL(*this, "ID_DIRS", wxCheckBox)->GetValue();

	wxString oldName = filter.name;
	filter.name = XRCCTRL(*this, "ID_NAME", wxTextCtrl)->GetValue();
	if (oldName != filter.name)
	{
		m_pFilterListCtrl->Delete(m_currentSelection);
		m_pFilterListCtrl->Insert(filter.name, m_currentSelection);
	}
}

void CFilterEditDialog::OnNew(wxCommandEvent& event)
{
	CFilter filter;

	int index = 1;
	wxString name = _("New filter");
	wxString newName = name;
	while (m_pFilterListCtrl->FindString(newName) != wxNOT_FOUND)
		newName = wxString::Format(_T("%s (%d)"), name.c_str(), ++index);

	filter.name = newName;

	m_filters.push_back(filter);

	m_pFilterListCtrl->Append(newName);

	for (std::vector<CFilterSet>::iterator iter = m_filterSets.begin(); iter != m_filterSets.end(); iter++)
	{
		CFilterSet& set = *iter;
		set.local.push_back(false);
		set.remote.push_back(false);
	}
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
	
	SetCtrlState(false);
}

void CFilterEditDialog::OnRename(wxCommandEvent& event)
{
}

void CFilterEditDialog::OnCopy(wxCommandEvent& event)
{
}

void CFilterEditDialog::OnFilterSelect(wxCommandEvent& event)
{
	int item = m_pFilterListCtrl->GetSelection();
	if (item == -1)
	{
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
	CFilter filter = m_filters[item];
	ShowFilter(filter);
}

void CFilterEditDialog::SetCtrlState(bool enabled)
{
	if (!enabled)
	{
		DestroyControls();
		m_pListCtrl->SetLineCount(0);
	}

	m_pListCtrl->Enable(enabled);
	XRCCTRL(*this, "ID_MATCHALL", wxRadioButton)->Enable(enabled);
	XRCCTRL(*this, "ID_MATCHANY", wxRadioButton)->Enable(enabled);
	XRCCTRL(*this, "ID_MORE", wxButton)->Enable(enabled);
	XRCCTRL(*this, "ID_REMOVE", wxButton)->Enable(enabled);
	XRCCTRL(*this, "ID_FILES", wxCheckBox)->Enable(enabled);
	XRCCTRL(*this, "ID_DIRS", wxCheckBox)->Enable(enabled);
}

void CFilterEditDialog::DestroyControls()
{
	for (unsigned int i = 0; i < m_filterControls.size(); i++)
	{
		CFilterControls& controls = m_filterControls[i];
		delete controls.pType;
		delete controls.pCondition;
		delete controls.pValue;
	}
	m_filterControls.clear();
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

	const unsigned int size = m_currentFilter.filters.size();
	if (!size)
	{
		m_pFilterListCtrl->SetSelection(m_currentSelection);
		wxMessageBox(_("Each filter needs at least one condition"));
		return false;
	}
	for (unsigned int i = 0; i < size; i++)
	{
		const CFilterControls& controls = m_filterControls[i];
		if (controls.pValue->GetValue() == _T(""))
		{
			m_pFilterListCtrl->SetSelection(m_currentSelection);
			m_pListCtrl->SelectLine(i);
			controls.pValue->SetFocus();
			wxMessageBox(_("At least one filter condition is incomplete"));
			return false;
		}
		if (controls.pType->GetSelection() == 1)
		{
			long number;
			if (!controls.pValue->GetValue().ToLong(&number) || number < 0)
			{
				m_pFilterListCtrl->SetSelection(m_currentSelection);
				m_pListCtrl->SelectLine(i);
				controls.pValue->SetFocus();
				wxMessageBox(_("Invalid size in condition"));
				return false;
			}				
		}
	}

	wxString name = XRCCTRL(*this, "ID_NAME", wxTextCtrl)->GetValue();
	if (name == _T(""))
	{
		m_pFilterListCtrl->SetSelection(m_currentSelection);
		XRCCTRL(*this, "ID_NAME", wxTextCtrl)->SetFocus();
		wxMessageBox(_("Need to enter filter name"));
		return false;
	}

	int pos = m_pFilterListCtrl->FindString(name);
	if (pos != wxNOT_FOUND && pos != m_currentSelection)
	{
		m_pFilterListCtrl->SetSelection(m_currentSelection);
		XRCCTRL(*this, "ID_NAME", wxTextCtrl)->SetFocus();
		wxMessageBox(_("Filter name already exists"));
		return false;
	}

	return true;
}
