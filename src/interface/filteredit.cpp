#include "FileZilla.h"
#include "filteredit.h"
#include "customheightlistctrl.h"

static wxArrayString filterTypes;
static wxArrayString stringConditionTypes;
static wxArrayString sizeConditionTypes;
static wxArrayString attributeConditionTypes;
static wxArrayString permissionConditionTypes;
static wxArrayString attributeSetTypes;

CFilterControls::CFilterControls()
{
	pType = 0;
	pCondition = 0;
	pValue = 0;
	pSet = 0;
}

void CFilterControls::Reset()
{
	delete pType;
	delete pCondition;
	delete pValue;
	delete pSet;

	pType = 0;
	pCondition = 0;
	pValue = 0;
	pSet = 0;
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
	for (unsigned int i = 0; i < m_filters.size(); i++)
	{
		if (!m_filters[i].HasConditionOfType(permissions) && !m_filters[i].HasConditionOfType(permissions))
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
#ifndef __WXMSW__
	bool m_hasAttributes = false;
	for (std::vector<CFilter>::const_iterator iter = filters.begin(); iter != filters.end(); iter++)
	{
		const CFilter& filter = *iter;
		if (!filter.HasConditionOfType(attributes))
			continue;

		m_hasAttributes = true;
		break;
	}
#else
	bool m_hasPermissions = false;
	for (std::vector<CFilter>::const_iterator iter = filters.begin(); iter != filters.end(); iter++)
	{
		const CFilter& filter = *iter;
		if (!filter.HasConditionOfType(permissions))
			continue;

		m_hasPermissions = true;
		break;
	}
#endif

	if (filterTypes.IsEmpty())
	{
		filterTypes.Add(_("Filename"));
		filterTypes.Add(_("Filesize"));
#ifndef __WXMSW__
		if (m_hasAttributes)
#endif
		{
			filterTypes.Add(_("Attribute"));
			attributeConditionTypes.Add(_("Archive"));
			attributeConditionTypes.Add(_("Compressed"));
			attributeConditionTypes.Add(_("Encrypted"));
			attributeConditionTypes.Add(_("Hidden"));
			attributeConditionTypes.Add(_("Read-only"));
			attributeConditionTypes.Add(_("System"));
		}

#ifdef __WXMSW__
		if (m_hasPermissions)
#endif
		{
			filterTypes.Add(_("Permission"));
			permissionConditionTypes.Add(_("owner readable"));
			permissionConditionTypes.Add(_("owner writeable"));
			permissionConditionTypes.Add(_("owner executable"));
			permissionConditionTypes.Add(_("group readable"));
			permissionConditionTypes.Add(_("group writeable"));
			permissionConditionTypes.Add(_("group executable"));
			permissionConditionTypes.Add(_("world readable"));
			permissionConditionTypes.Add(_("world writeable"));
			permissionConditionTypes.Add(_("world executable"));
		}

		attributeSetTypes.Add(_("is set"));
		attributeSetTypes.Add(_("is unset"));

		stringConditionTypes.Add(_("contains"));
		stringConditionTypes.Add(_("is equal to"));
		stringConditionTypes.Add(_("begins with"));
		stringConditionTypes.Add(_("ends with"));
		stringConditionTypes.Add(_("matches regex"));

		sizeConditionTypes.Add(_("greater than"));
		sizeConditionTypes.Add(_("equals"));
		sizeConditionTypes.Add(_("less than"));
	}

	if (!Load(parent, _T("ID_EDITFILTER")))
		return false;

	wxScrolledWindow* wnd = XRCCTRL(*this, "ID_CONDITIONS", wxScrolledWindow);
	wxSizerItem* pSizerItem = GetSizer()->GetItem(wnd, true);
	m_pListCtrl = new wxCustomHeightListCtrl(this, wxID_ANY, wxDefaultPosition, wnd->GetSize(), wxVSCROLL|wxSUNKEN_BORDER);
	if (!m_pListCtrl)
		return false;
	pSizerItem->SetWindow(m_pListCtrl);
	wnd->Destroy();
	CalcMinListWidth();

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

			if (controls.pValue)
			{
				pos = controls.pValue->GetPosition();
				pos.y -= deleted * (m_choiceBoxHeight + 6);
				controls.pValue->SetPosition(pos);
			}
			if (controls.pSet)
			{
				pos = controls.pSet->GetPosition();
				pos.y -= deleted * (m_choiceBoxHeight + 6);
				controls.pSet->SetPosition(pos);
			}

			controls.pType->SetId(i - deleted);
		}
		else
		{
			controls.Reset();
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

	t_filterType type = GetTypeFromTypeSelection(event.GetSelection());
	if (type == filter.type)
		return;
	filter.type = type;

	MakeControls(filter, item);
}

void CFilterEditDialog::MakeControls(const CFilterCondition& condition, unsigned int i /*=-1*/)
{
	wxRect size = m_pListCtrl->GetClientSize();

	if (i == -1)
	{
		i = m_filterControls.size();
		CFilterControls controls;
		m_filterControls.push_back(controls);
	}
	CFilterControls& controls = m_filterControls[i];
	controls.Reset();

	// Get correct coordinates
	int posy = i * (m_choiceBoxHeight + 6) + 3;
	int posx = 0, x, y;
	m_pListCtrl->CalcScrolledPosition(posx, posy, &x, &y);
	posy = y;

	controls.pType = new wxChoice();
	controls.pType->Create(m_pListCtrl, i, wxPoint(10, posy), wxDefaultSize, filterTypes);
	controls.pType->Select(condition.type);
	wxRect typeRect = controls.pType->GetSize();

	if (!m_choiceBoxHeight)
	{
		wxSize size = controls.pType->GetSize();
		m_choiceBoxHeight = size.GetHeight();
		m_pListCtrl->SetLineHeight(m_choiceBoxHeight + 6);
	}

	controls.pCondition = new wxChoice();
	switch (condition.type)
	{
	case name:
		controls.pCondition->Create(m_pListCtrl, wxID_ANY, wxPoint(20 + typeRect.GetWidth(), posy), wxDefaultSize, stringConditionTypes);
		break;
	case (enum t_filterType)::size:
		controls.pCondition->Create(m_pListCtrl, wxID_ANY, wxPoint(20 + typeRect.GetWidth(), posy), wxDefaultSize, sizeConditionTypes);
		break;
	case attributes:
		controls.pCondition->Create(m_pListCtrl, wxID_ANY, wxPoint(20 + typeRect.GetWidth(), posy), wxDefaultSize, attributeConditionTypes);
		break;
	case permissions:
		controls.pCondition->Create(m_pListCtrl, wxID_ANY, wxPoint(20 + typeRect.GetWidth(), posy), wxDefaultSize, permissionConditionTypes);
		break;
	}
	controls.pCondition->Select(condition.condition);
	wxRect conditionsRect = controls.pCondition->GetSize();

	posx = 30 + typeRect.GetWidth() + conditionsRect.GetWidth();
	if (condition.type == name || condition.type == (enum t_filterType)::size)
	{		
		controls.pValue = new wxTextCtrl();
		controls.pValue->Create(m_pListCtrl, wxID_ANY, _T(""), wxPoint(posx, posy), wxSize(size.GetWidth() - posx - 10, -1));
		controls.pValue->SetValue(condition.strValue);
	}
	else
	{
		controls.pSet = new wxChoice();
		controls.pSet->Create(m_pListCtrl, wxID_ANY, wxPoint(posx, posy), wxSize(size.GetWidth() - posx - 10, -1), attributeSetTypes);
		controls.pSet->Select(condition.strValue != _T("0") ? 0 : 1);
	}
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

	wxASSERT(m_filterControls.size() == m_currentFilter.filters.size());

	filter.filters.clear();
	for (unsigned int i = 0; i < m_currentFilter.filters.size(); i++)
	{
		const CFilterControls& controls = m_filterControls[i];
		CFilterCondition condition = m_currentFilter.filters[i];

		condition.type = GetTypeFromTypeSelection(controls.pType->GetSelection());
		condition.condition = controls.pCondition->GetSelection();

		switch (condition.type)
		{
		case name:
			condition.strValue = controls.pValue->GetValue();
			break;
		case size:
			{
				condition.strValue = controls.pValue->GetValue();
				unsigned long tmp;
				condition.strValue.ToULong(&tmp);
				condition.value = tmp;
			}
			break;
		case attributes:
		case permissions:
			if (controls.pSet->GetSelection())
			{
				condition.strValue = _T("0");
				condition.value = 0;
			}
			else
			{
				condition.strValue = _T("1");
				condition.value = 1;
			}
			break;
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
		int oldSelection = m_currentSelection;
		m_pFilterListCtrl->Delete(oldSelection);
		m_pFilterListCtrl->Insert(filter.name, oldSelection);
		m_pFilterListCtrl->Select(oldSelection);
		wxCommandEvent evt;
		OnFilterSelect(evt);
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

	SetCtrlState(false);
}

void CFilterEditDialog::OnRename(wxCommandEvent& event)
{
	const wxString& oldName = XRCCTRL(*this, "ID_NAME", wxTextCtrl)->GetValue();
	wxTextEntryDialog dlg(this, _("Please enter a new name for the filter."), _("Enter filter name"), oldName);
	if (dlg.ShowModal() != wxID_OK)
		return;

	const wxString& newName = dlg.GetValue();

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

	m_currentFilter.name = newName;
	XRCCTRL(*this, "ID_NAME", wxTextCtrl)->SetValue(newName);
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
	const wxString& name = m_currentFilter.name;
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
	XRCCTRL(*this, "ID_MATCHNONE", wxRadioButton)->Enable(enabled);
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
		controls.Reset();
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
		wxMessageBox(_("Each filter needs at least one condition."), _("Filter validation failed"), wxICON_ERROR, this);
		return false;
	}

	wxASSERT(m_filterControls.size() == m_currentFilter.filters.size());

	for (unsigned int i = 0; i < size; i++)
	{
		const CFilterControls& controls = m_filterControls[i];
		enum t_filterType type = GetTypeFromTypeSelection(controls.pType->GetSelection());
		if (type == name && controls.pValue->GetValue() == _T(""))
		{
			m_pFilterListCtrl->SetSelection(m_currentSelection);
			m_pListCtrl->SelectLine(i);
			controls.pValue->SetFocus();
			wxMessageBox(_("At least one filter condition is incomplete"), _("Filter validation failed"), wxICON_ERROR, this);
			return false;
		}
		else if (type == size)
		{
			long number;
			if (!controls.pValue->GetValue().ToLong(&number) || number < 0)
			{
				m_pFilterListCtrl->SetSelection(m_currentSelection);
				m_pListCtrl->SelectLine(i);
				controls.pValue->SetFocus();
				wxMessageBox(_("Invalid size in condition"), _("Filter validation failed"), wxICON_ERROR, this);
				return false;
			}
		}
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

void CFilterEditDialog::CalcMinListWidth()
{
	wxChoice *pType = new wxChoice(m_pListCtrl, wxID_ANY, wxDefaultPosition, wxDefaultSize, filterTypes);
	int requiredWidth = pType->GetBestSize().GetWidth();
	pType->Destroy();

	wxChoice *pStringCondition = new wxChoice(m_pListCtrl, wxID_ANY, wxDefaultPosition, wxDefaultSize, stringConditionTypes);
	wxChoice *pSizeCondition = new wxChoice(m_pListCtrl, wxID_ANY, wxDefaultPosition, wxDefaultSize, sizeConditionTypes);
	requiredWidth += wxMax(pStringCondition->GetBestSize().GetWidth(), pSizeCondition->GetBestSize().GetWidth());
	pStringCondition->Destroy();
	pSizeCondition->Destroy();

	requiredWidth += m_pListCtrl->GetWindowBorderSize().x;
	requiredWidth += 40;
	requiredWidth += 100;

	wxSize minSize = m_pListCtrl->GetMinSize();
	minSize.IncTo(wxSize(requiredWidth, -1));
	m_pListCtrl->SetMinSize(minSize);
	Layout();
	Fit();
}

t_filterType CFilterEditDialog::GetTypeFromTypeSelection(int selection)
{
#ifdef __WXMSW__
	if (!m_hasPermissions && selection >= permissions)
		selection++;
#else
	if (!m_hasAttributes && selection >= attributes)
		selection++;
#endif
	return (enum t_filterType)selection;
}
