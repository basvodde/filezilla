#include "FileZilla.h"
#include "filter_conditions_dialog.h"
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

BEGIN_EVENT_TABLE(CFilterConditionsDialog, wxDialogEx)
EVT_BUTTON(XRCID("ID_MORE"), CFilterConditionsDialog::OnMore)
EVT_BUTTON(XRCID("ID_REMOVE"), CFilterConditionsDialog::OnRemove)
EVT_CHOICE(wxID_ANY, CFilterConditionsDialog::OnFilterTypeChange)
EVT_LISTBOX(wxID_ANY, CFilterConditionsDialog::OnConditionSelectionChange)
END_EVENT_TABLE()

CFilterConditionsDialog::CFilterConditionsDialog()
{
	m_choiceBoxHeight = 0;
	m_pListCtrl = 0;
	m_has_foreign_type = false;
}

bool CFilterConditionsDialog::CreateListControl(bool has_foreign_type)
{
	wxScrolledWindow* wnd = XRCCTRL(*this, "ID_CONDITIONS", wxScrolledWindow);
	if (!wnd)
		return false;

	m_has_foreign_type = has_foreign_type;

	m_pListCtrl = new wxCustomHeightListCtrl(this, wxID_ANY, wxDefaultPosition, wnd->GetSize(), wxVSCROLL|wxSUNKEN_BORDER);
	if (!m_pListCtrl)
		return false;
	ReplaceControl(wnd, m_pListCtrl);
	CalcMinListWidth();

	if (filterTypes.IsEmpty())
	{
		filterTypes.Add(_("Filename"));
		filterTypes.Add(_("Filesize"));
#ifndef __WXMSW__
		if (m_has_foreign_type)
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
		if (m_has_foreign_type)
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
		filterTypes.Add(_("Path"));

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

	SetFilterCtrlState(true);

	return true;
}

void CFilterConditionsDialog::CalcMinListWidth()
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
}

t_filterType CFilterConditionsDialog::GetTypeFromTypeSelection(int selection)
{
	if (!m_has_foreign_type)
	{
#ifdef __WXMSW__
		if (selection >= permissions)
			selection++;
#else
		if (selection >= attributes)
			selection++;
#endif
	}

	return (enum t_filterType)selection;
}

void CFilterConditionsDialog::SetSelectionFromType(wxChoice* pChoice, t_filterType type)
{
	int selection = type;

	if (!m_has_foreign_type)
	{
#ifdef __WXMSW__
		if (selection >= permissions)
			selection--;
#else
		if (selection >= attributes)
			selection--;
#endif
	}

	pChoice->SetSelection(selection);
}

void CFilterConditionsDialog::OnMore(wxCommandEvent& event)
{
	CFilterCondition cond;
	m_currentFilter.filters.push_back(cond);

	MakeControls(cond);
	UpdateConditionsClientSize();
}

void CFilterConditionsDialog::OnRemove(wxCommandEvent& event)
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
	UpdateConditionsClientSize();

	SetFilterCtrlState(false);
}

void CFilterConditionsDialog::OnFilterTypeChange(wxCommandEvent& event)
{
	int item = event.GetId();
	if (item < 0 || item >= (int)m_filterControls.size())
		return;

	CFilterCondition& filter = m_currentFilter.filters[item];

	t_filterType type = GetTypeFromTypeSelection(event.GetSelection());
	if (type == filter.type)
		return;
	filter.type = type;

	if (filter.type == size && filter.condition > 3)
		filter.condition = 0;

	MakeControls(filter, item);
}

void CFilterConditionsDialog::MakeControls(const CFilterCondition& condition, int i /*=-1*/)
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
	SetSelectionFromType(controls.pType, condition.type);
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
	case path:
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
	default:
		wxFAIL_MSG(_T("Unhandled condition"));
		break;
	}
	controls.pCondition->Select(condition.condition);
	wxRect conditionsRect = controls.pCondition->GetSize();

	posx = 30 + typeRect.GetWidth() + conditionsRect.GetWidth();
	if (condition.type == name || condition.type == (enum t_filterType)::size || condition.type == path)
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

void CFilterConditionsDialog::DestroyControls()
{
	for (unsigned int i = 0; i < m_filterControls.size(); i++)
	{
		CFilterControls& controls = m_filterControls[i];
		controls.Reset();
	}
	m_filterControls.clear();
}

void CFilterConditionsDialog::UpdateConditionsClientSize()
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

void CFilterConditionsDialog::EditFilter(const CFilter& filter)
{
	DestroyControls();

	// Create new controls
	m_currentFilter = filter;
	for (unsigned int i = 0; i < filter.filters.size(); i++)
	{
		const CFilterCondition& cond = filter.filters[i];

		MakeControls(cond);
	}

	UpdateConditionsClientSize();

	SetFilterCtrlState(false);
}

CFilter CFilterConditionsDialog::GetFilter()
{
	wxASSERT(m_filterControls.size() == m_currentFilter.filters.size());

	CFilter filter;
	for (unsigned int i = 0; i < m_currentFilter.filters.size(); i++)
	{
		const CFilterControls& controls = m_filterControls[i];
		CFilterCondition condition = m_currentFilter.filters[i];

		condition.type = GetTypeFromTypeSelection(controls.pType->GetSelection());
		condition.condition = controls.pCondition->GetSelection();

		switch (condition.type)
		{
		case name:
		case path:
			if (controls.pValue->GetValue() == _T(""))
				continue;
			condition.strValue = controls.pValue->GetValue();
			break;
		case size:
			{
				if (controls.pValue->GetValue() == _T(""))
					continue;
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
		default:
			wxFAIL_MSG(_T("Unhandled condition"));
			break;
		}

		condition.matchCase = filter.matchCase;

		filter.filters.push_back(condition);
	}
	
	return filter;
}

void CFilterConditionsDialog::ClearFilter(bool disable)
{
	DestroyControls();
	m_pListCtrl->SetLineCount(0);

	SetFilterCtrlState(disable);
}

void CFilterConditionsDialog::SetFilterCtrlState(bool disable)
{
	m_pListCtrl->Enable(!disable);
	XRCCTRL(*this, "ID_MORE", wxButton)->Enable(!disable);
	XRCCTRL(*this, "ID_REMOVE", wxButton)->Enable(!disable && !m_pListCtrl->GetSelection().empty());
}

bool CFilterConditionsDialog::ValidateFilter(wxString& error, bool allow_empty /*=false*/)
{
	const unsigned int size = m_currentFilter.filters.size();
	if (!size)
	{
		if (allow_empty)
			return true;

		error = _("Each filter needs at least one condition.");
		return false;
	}

	wxASSERT(m_filterControls.size() == m_currentFilter.filters.size());

	for (unsigned int i = 0; i < size; i++)
	{
		const CFilterControls& controls = m_filterControls[i];
		enum t_filterType type = GetTypeFromTypeSelection(controls.pType->GetSelection());

		if ((type == name || type == path) && controls.pValue->GetValue() == _T(""))
		{
			if (allow_empty)
				continue;

			m_pListCtrl->SelectLine(i);
			controls.pValue->SetFocus();
			error = _("At least one filter condition is incomplete");
			SetFilterCtrlState(false);
			return false;
		}
		else if (type == (enum t_filterType)::size)
		{
			const wxString v = controls.pValue->GetValue();
			if (v == _T("") && allow_empty)
				continue;

			long number;
			if (!v.ToLong(&number) || number < 0)
			{
				m_pListCtrl->SelectLine(i);
				controls.pValue->SetFocus();
				error = _("Invalid size in condition");
				SetFilterCtrlState(false);
				return false;
			}
		}
	}

	return true;
}

void CFilterConditionsDialog::OnConditionSelectionChange(wxCommandEvent& event)
{
	if (event.GetId() != m_pListCtrl->GetId())
		return;

	SetFilterCtrlState(false);
}
