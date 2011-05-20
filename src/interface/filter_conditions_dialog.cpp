#include <filezilla.h>
#include "filter_conditions_dialog.h"
#include "customheightlistctrl.h"

static wxArrayString stringConditionTypes;
static wxArrayString sizeConditionTypes;
static wxArrayString attributeConditionTypes;
static wxArrayString permissionConditionTypes;
static wxArrayString attributeSetTypes;
static wxArrayString dateConditionTypes;

static wxChoice* CreateChoice(wxWindow* parent, wxPoint pos, const wxArrayString& items, wxSize const& size = wxDefaultSize)
{
#ifdef __WXGTK__
	// Really obscure bug in wxGTK: If creating in a single step,
	// first item in the choice sometimes looks disabled
	// even though it can still be selected and returns to looking
	// normal after hovering mouse over it.
	// This works around it nicely.
	wxChoice *ret( new wxChoice );
	ret->Create(parent, wxID_ANY, pos, size);
	ret->Append(items);
	ret->InvalidateBestSize();
	ret->SetInitialSize();
	return ret;
#else
	return new wxChoice(parent, wxID_ANY, pos, size, items);
#endif
}

CFilterControls::CFilterControls()
{
	pType = 0;
	pCondition = 0;
	pValue = 0;
	pSet = 0;
	pRemove = 0;
}

void CFilterControls::Reset()
{
	delete pType;
	delete pCondition;
	delete pValue;
	delete pSet;
	delete pRemove;

	pType = 0;
	pCondition = 0;
	pValue = 0;
	pSet = 0;
	pRemove = 0;
}

BEGIN_EVENT_TABLE(CFilterConditionsDialog, wxDialogEx)
EVT_BUTTON(wxID_ANY, CFilterConditionsDialog::OnButton)
EVT_CHOICE(wxID_ANY, CFilterConditionsDialog::OnFilterTypeChange)
EVT_LISTBOX(wxID_ANY, CFilterConditionsDialog::OnConditionSelectionChange)
EVT_NAVIGATION_KEY(CFilterConditionsDialog::OnNavigationKeyEvent)
END_EVENT_TABLE()

CFilterConditionsDialog::CFilterConditionsDialog()
{
	m_choiceBoxHeight = 0;
	m_pListCtrl = 0;
	m_has_foreign_type = false;
	m_pAdd = 0;
	m_button_size = wxSize(-1, -1);
}

bool CFilterConditionsDialog::CreateListControl(int conditions /*=common*/)
{
	wxScrolledWindow* wnd = XRCCTRL(*this, "ID_CONDITIONS", wxScrolledWindow);
	if (!wnd)
		return false;

	m_pListCtrl = new wxCustomHeightListCtrl(this, wxID_ANY, wxDefaultPosition, wnd->GetSize(), wxVSCROLL|wxSUNKEN_BORDER|wxTAB_TRAVERSAL);
	if (!m_pListCtrl)
		return false;
	m_pListCtrl->AllowSelection(false);
	ReplaceControl(wnd, m_pListCtrl);
	CalcMinListWidth();

	if (stringConditionTypes.IsEmpty())
	{
		stringConditionTypes.Add(_("contains"));
		stringConditionTypes.Add(_("is equal to"));
		stringConditionTypes.Add(_("begins with"));
		stringConditionTypes.Add(_("ends with"));
		stringConditionTypes.Add(_("matches regex"));
		stringConditionTypes.Add(_("does not contain"));

		sizeConditionTypes.Add(_("greater than"));
		sizeConditionTypes.Add(_("equals"));
		sizeConditionTypes.Add(_("does not equal"));
		sizeConditionTypes.Add(_("less than"));

		attributeSetTypes.Add(_("is set"));
		attributeSetTypes.Add(_("is unset"));

		attributeConditionTypes.Add(_("Archive"));
		attributeConditionTypes.Add(_("Compressed"));
		attributeConditionTypes.Add(_("Encrypted"));
		attributeConditionTypes.Add(_("Hidden"));
		attributeConditionTypes.Add(_("Read-only"));
		attributeConditionTypes.Add(_("System"));

		permissionConditionTypes.Add(_("owner readable"));
		permissionConditionTypes.Add(_("owner writeable"));
		permissionConditionTypes.Add(_("owner executable"));
		permissionConditionTypes.Add(_("group readable"));
		permissionConditionTypes.Add(_("group writeable"));
		permissionConditionTypes.Add(_("group executable"));
		permissionConditionTypes.Add(_("world readable"));
		permissionConditionTypes.Add(_("world writeable"));
		permissionConditionTypes.Add(_("world executable"));

		dateConditionTypes.Add(_("before"));
		dateConditionTypes.Add(_("equals"));
		dateConditionTypes.Add(_("does not equal"));
		dateConditionTypes.Add(_("after"));
	}

	if (conditions & filter_name)
	{
		filterTypes.Add(_("Filename"));
		filter_type_map.push_back(filter_name);
	}
	if (conditions & filter_size)
	{
		filterTypes.Add(_("Filesize"));
		filter_type_map.push_back(filter_size);
	}
	if (conditions & filter_attributes)
	{
		filterTypes.Add(_("Attribute"));
		filter_type_map.push_back(filter_attributes);
	}
	if (conditions & filter_permissions)
	{
		filterTypes.Add(_("Permission"));
		filter_type_map.push_back(filter_permissions);
	}
	if (conditions & filter_path)
	{
		filterTypes.Add(_("Path"));
		filter_type_map.push_back(filter_path);
	}
	if (conditions & filter_date)
	{
		filterTypes.Add(_("Date"));
		filter_type_map.push_back(filter_date);
	}

	SetFilterCtrlState(true);

	m_pListCtrl->Connect(wxEVT_SIZE, wxSizeEventHandler(CFilterConditionsDialog::OnListSize), 0, this);

	m_pListCtrl->MoveAfterInTabOrder(XRCCTRL(*this, "ID_MATCHTYPE", wxChoice));

	return true;
}

void CFilterConditionsDialog::CalcMinListWidth()
{
	wxChoice *pType = new wxChoice(m_pListCtrl, wxID_ANY, wxDefaultPosition, wxDefaultSize, filterTypes);
	int requiredWidth = pType->GetBestSize().GetWidth();
	pType->Destroy();

	wxChoice *pStringCondition = new wxChoice(m_pListCtrl, wxID_ANY, wxDefaultPosition, wxDefaultSize, stringConditionTypes);
	wxChoice *pSizeCondition = new wxChoice(m_pListCtrl, wxID_ANY, wxDefaultPosition, wxDefaultSize, sizeConditionTypes);
	wxChoice *pDateCondition = new wxChoice(m_pListCtrl, wxID_ANY, wxDefaultPosition, wxDefaultSize, dateConditionTypes);

	int w = wxMax(pStringCondition->GetBestSize().GetWidth(), pSizeCondition->GetBestSize().GetWidth());
	w = wxMax(w, pDateCondition->GetBestSize().GetWidth());
	requiredWidth += w;

	pStringCondition->Destroy();
	pSizeCondition->Destroy();
	pDateCondition->Destroy();

	requiredWidth += m_pListCtrl->GetWindowBorderSize().x;
	requiredWidth += 40;
	requiredWidth += 120;
	wxSize minSize = m_pListCtrl->GetMinSize();
	minSize.IncTo(wxSize(requiredWidth, -1));
	m_pListCtrl->SetMinSize(minSize);

	m_lastListSize = m_pListCtrl->GetClientSize();
}

t_filterType CFilterConditionsDialog::GetTypeFromTypeSelection(int selection)
{
	if (selection < 0 || selection > (int)filter_type_map.size())
		selection = 0;

	return filter_type_map[selection];
}

void CFilterConditionsDialog::SetSelectionFromType(wxChoice* pChoice, t_filterType type)
{
	for (unsigned int i = 0; i < filter_type_map.size(); i++)
	{
		if (filter_type_map[i] == type)
		{
			pChoice->SetSelection(i);
			return;
		}
	}

	 pChoice->SetSelection(0);
}

void CFilterConditionsDialog::OnMore()
{
	wxPoint pos = m_pAdd->GetPosition();
	pos.y += m_choiceBoxHeight + 6;
	m_pAdd->SetPosition(pos);

	CFilterCondition cond;
	m_currentFilter.filters.push_back(cond);

	MakeControls(cond);

	CFilterControls& controls = m_filterControls.back();
	m_pAdd->MoveAfterInTabOrder(controls.pSet ? (wxWindow*)controls.pSet : (wxWindow*)controls.pValue);

	m_pListCtrl->SetLineCount(m_filterControls.size() + 1);
	UpdateConditionsClientSize();
}

void CFilterConditionsDialog::OnRemove(int item)
{
	std::set<int> selected;
	selected.insert(item);
	OnRemove(selected);
	if (!m_filterControls.size())
		OnMore();
}

void CFilterConditionsDialog::OnRemove(const std::set<int> &selected)
{
	int delta_y = 0;

	m_pListCtrl->SetLineCount(m_filterControls.size() - selected.size() + 1);

	std::vector<CFilterControls> filterControls = m_filterControls;
	m_filterControls.clear();

	std::vector<CFilterCondition> filters = m_currentFilter.filters;
	m_currentFilter.filters.clear();

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
			pos.y -= delta_y;
			controls.pType->SetPosition(pos);

			pos = controls.pCondition->GetPosition();
			pos.y -= delta_y;
			controls.pCondition->SetPosition(pos);

			if (controls.pValue)
			{
				pos = controls.pValue->GetPosition();
				pos.y -= delta_y;
				controls.pValue->SetPosition(pos);
			}
			if (controls.pSet)
			{
				pos = controls.pSet->GetPosition();
				pos.y -= delta_y;
				controls.pSet->SetPosition(pos);
			}

			pos = controls.pRemove->GetPosition();
			pos.y -= delta_y;
			controls.pRemove->SetPosition(pos);
		}
		else
		{
			controls.Reset();
			delta_y += m_choiceBoxHeight + 6;
		}
	}

	wxPoint pos = m_pAdd->GetPosition();
	pos.y -= delta_y;
	m_pAdd->SetPosition(pos);

	m_pListCtrl->ClearSelection();
	UpdateConditionsClientSize();

	SetFilterCtrlState(false);
}

void CFilterConditionsDialog::OnFilterTypeChange(wxCommandEvent& event)
{
	int item;
	for (item = 0; item < (int)m_filterControls.size(); item++)
	{
		if (!m_filterControls[item].pType || m_filterControls[item].pType->GetId() != event.GetId())
			continue;

		break;
	}
	if (item == (int)m_filterControls.size())
		return;

	CFilterCondition& filter = m_currentFilter.filters[item];

	t_filterType type = GetTypeFromTypeSelection(event.GetSelection());
	if (type == filter.type)
		return;
	filter.type = type;

	if (filter.type == filter_size && filter.condition > 3)
		filter.condition = 0;
	else if (filter.type == filter_date && filter.condition > 3)
		filter.condition = 0;
	delete m_filterControls[item].pCondition;
	m_filterControls[item].pCondition = 0;

	MakeControls(filter, item);
}

void CFilterConditionsDialog::MakeControls(const CFilterCondition& condition, int i /*=-1*/)
{
	wxRect client_size = m_pListCtrl->GetClientSize();

	if (i == -1)
	{
		i = m_filterControls.size();
		CFilterControls controls;
		m_filterControls.push_back(controls);
	}
	CFilterControls& controls = m_filterControls[i];

	// Get correct coordinates
	int posy = i * (m_choiceBoxHeight + 6) + 3;
	int posx = 0, x, y;
	m_pListCtrl->CalcScrolledPosition(posx, posy, &x, &y);
	posy = y;

	wxPoint pos = wxPoint(5, posy);
	if (!controls.pType)
		controls.pType = CreateChoice(m_pListCtrl, pos, filterTypes);
	else
		controls.pType->SetPosition(pos);
	SetSelectionFromType(controls.pType, condition.type);
	wxRect typeRect = controls.pType->GetSize();

	if (!m_choiceBoxHeight)
	{
		wxSize size = controls.pType->GetSize();
		m_choiceBoxHeight = size.GetHeight();
		m_pListCtrl->SetLineHeight(m_choiceBoxHeight + 6);
	}

	pos = wxPoint(10 + typeRect.GetWidth(), posy);
	if (!controls.pCondition)
	{
		switch (condition.type)
		{
		case filter_name:
		case filter_path:
			controls.pCondition = CreateChoice(m_pListCtrl, pos, stringConditionTypes);
			break;
		case filter_size:
			controls.pCondition = CreateChoice(m_pListCtrl, pos, sizeConditionTypes);
			break;
		case filter_attributes:
			controls.pCondition = CreateChoice(m_pListCtrl, pos, attributeConditionTypes);
			break;
		case filter_permissions:
			controls.pCondition = CreateChoice(m_pListCtrl, pos, permissionConditionTypes);
			break;
		case filter_date:
			controls.pCondition = CreateChoice(m_pListCtrl, pos, dateConditionTypes);
			break;
		default:
			wxFAIL_MSG(_T("Unhandled condition"));
			break;
		}
		controls.pCondition->MoveAfterInTabOrder(controls.pType);
	}
	else
		controls.pCondition->SetPosition(pos);

	controls.pCondition->Select(condition.condition);
	wxRect conditionsRect = controls.pCondition->GetSize();

	if (!controls.pRemove)
	{
		controls.pRemove = new wxButton(m_pListCtrl, wxID_ANY, _T("-"), wxPoint(client_size.GetWidth() - 5 - m_button_size.x, posy), m_button_size, wxBU_EXACTFIT);
		if (m_button_size.x <= 0)
		{
			m_button_size.x = wxMax(m_choiceBoxHeight, controls.pRemove->GetSize().x);
			m_button_size.y = m_choiceBoxHeight;
			controls.pRemove->SetSize(m_button_size);
			controls.pRemove->SetPosition(wxPoint(client_size.GetWidth() - 5 - m_button_size.x, posy));
		}
		controls.pRemove->MoveAfterInTabOrder(controls.pCondition);
	}
	else
		controls.pRemove->SetPosition(wxPoint(client_size.GetWidth() - 5 - m_button_size.x, posy));

	posx = 15 + typeRect.GetWidth() + conditionsRect.GetWidth();
	const int maxwidth = client_size.GetWidth() - posx - 10 - m_button_size.x;
	if (condition.type == filter_name || condition.type == filter_size || condition.type == filter_path || condition.type == filter_date)
	{
		delete controls.pSet;
		controls.pSet = 0;

		pos = wxPoint(posx, posy);
		wxSize size(maxwidth, -1);

		if (!controls.pValue)
		{
			controls.pValue = new wxTextCtrl();
			controls.pValue->Create(m_pListCtrl, wxID_ANY, _T(""), pos, size);
		}
		else
		{
			controls.pValue->SetPosition(pos);
			controls.pValue->SetSize(size);
		}
		controls.pValue->SetValue(condition.strValue);

		// Need to explicitely set min size, otherwise initial size becomes min size
		controls.pValue->SetMinSize(wxSize(20, -1));
		controls.pValue->MoveBeforeInTabOrder(controls.pRemove);
	}
	else
	{
		delete controls.pValue;
		controls.pValue = 0;

		pos = wxPoint(posx, posy);
		wxSize size(maxwidth, -1);
		if (!controls.pSet)
			controls.pSet = CreateChoice(m_pListCtrl, pos, attributeSetTypes, size);
		else
		{
			controls.pSet->SetPosition(pos);
			controls.pSet->SetSize(size);
		}
		controls.pSet->Select(condition.strValue != _T("0") ? 0 : 1);

		// Need to explicitely set min size, otherwise initial size becomes min size
		controls.pSet->SetMinSize(wxSize(20, -1));
		controls.pSet->MoveBeforeInTabOrder(controls.pRemove);
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
	wxSize newSize = m_pListCtrl->GetClientSize();

	if (m_lastListSize.GetWidth() == newSize.GetWidth())
		return;

	int deltaX = newSize.GetWidth() - m_lastListSize.GetWidth();
	m_lastListSize = newSize;

	// Resize text fields
	for (unsigned int i = 0; i < m_filterControls.size(); i++)
	{
		CFilterControls& controls = m_filterControls[i];
		if (!controls.pValue)
			continue;

		wxSize size = controls.pValue->GetSize();
		size.SetWidth(size.GetWidth() + deltaX);
		controls.pValue->SetSize(size);

		wxPoint pos = controls.pRemove->GetPosition();
		pos.x += deltaX;
		controls.pRemove->SetPosition(pos);
	}

	// Move add button
	if (m_pAdd)
	{
		wxPoint pos = m_pAdd->GetPosition();
		pos.x += deltaX;
		m_pAdd->SetPosition(pos);
	}
}

void CFilterConditionsDialog::EditFilter(const CFilter& filter)
{
	DestroyControls();

	// Create new controls
	m_currentFilter = filter;

	if (!m_currentFilter.filters.size())
		m_currentFilter.filters.push_back(CFilterCondition());

	for (unsigned int i = 0; i < m_currentFilter.filters.size(); i++)
	{
		const CFilterCondition& cond = m_currentFilter.filters[i];

		MakeControls(cond);
	}

	// Get correct coordinates
	wxSize client_size = m_pListCtrl->GetClientSize();
	wxPoint pos;
	m_pListCtrl->CalcScrolledPosition(client_size.GetWidth() - 5 - m_button_size.x, (m_choiceBoxHeight + 6) * m_filterControls.size() + 3, &pos.x, &pos.y);

	if (!m_pAdd)
		m_pAdd = new wxButton(m_pListCtrl, wxID_ANY, _T("+"), pos, m_button_size);
	else
		m_pAdd->SetPosition(pos);

	m_pListCtrl->SetLineCount(m_filterControls.size() + 1);
	UpdateConditionsClientSize();

	XRCCTRL(*this, "ID_MATCHTYPE", wxChoice)->SetSelection(filter.matchType);

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
		case filter_name:
		case filter_path:
			if (controls.pValue->GetValue() == _T(""))
				continue;
			condition.strValue = controls.pValue->GetValue();
			break;
		case filter_size:
			{
				if (controls.pValue->GetValue() == _T(""))
					continue;
				condition.strValue = controls.pValue->GetValue();
				unsigned long tmp;
				condition.strValue.ToULong(&tmp);
				condition.value = tmp;
			}
			break;
		case filter_attributes:
		case filter_permissions:
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
		case filter_date:
			if (controls.pValue->GetValue() == _T(""))
				continue;
			condition.strValue = controls.pValue->GetValue();
			if (!condition.date.ParseFormat(condition.strValue, _T("%Y-%m-%d")) || !condition.date.IsValid())
				continue;
			break;
		default:
			wxFAIL_MSG(_T("Unhandled condition"));
			break;
		}

		condition.matchCase = filter.matchCase;

		filter.filters.push_back(condition);
	}
	
	switch (XRCCTRL(*this, "ID_MATCHTYPE", wxChoice)->GetSelection())
	{
	case 1:
		filter.matchType = CFilter::any;
		break;
	case 2:
		filter.matchType = CFilter::none;
		break;
	default:
		filter.matchType = CFilter::all;
		break;
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

	XRCCTRL(*this, "ID_MATCHTYPE", wxChoice)->Enable(!disable);
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

		if ((type == filter_name || type == filter_path) && controls.pValue->GetValue() == _T(""))
		{
			if (allow_empty)
				continue;

			m_pListCtrl->SelectLine(i);
			controls.pValue->SetFocus();
			error = _("At least one filter condition is incomplete");
			SetFilterCtrlState(false);
			return false;
		}
		else if (type == filter_size)
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
		else if (type == filter_date)
		{
			const wxString d = controls.pValue->GetValue();
			if (d == _T("") && allow_empty)
				continue;

			wxDateTime date;
			if (!date.ParseFormat(d, _T("%Y-%m-%d")) || !date.IsValid())
			{
				m_pListCtrl->SelectLine(i);
				controls.pValue->SetFocus();
				error = _("Please enter a date of the form YYYY-MM-DD such as for example 2010-07-18.");
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

void CFilterConditionsDialog::OnButton(wxCommandEvent& event)
{
	if (event.GetId() == m_pAdd->GetId())
	{
		OnMore();
		return;
	}

	for (int i = 0; i < (int)m_filterControls.size(); i++)
	{
		if (m_filterControls[i].pRemove->GetId() == event.GetId())
		{
			OnRemove(i);
			return;
		}
	}
	event.Skip();
}

void CFilterConditionsDialog::OnListSize(wxSizeEvent& event)
{
	UpdateConditionsClientSize();
	event.Skip();
}

void CFilterConditionsDialog::OnNavigationKeyEvent(wxNavigationKeyEvent& event)
{
	wxWindow* source = FindFocus();
	if (!source)
	{
		event.Skip();
		return;
	}

	wxWindow* target = 0;

	if (event.GetDirection())
	{
		for (int i = 0; i < (int)m_filterControls.size(); i++)
		{
			if (m_filterControls[i].pType == source)
			{
				target = m_filterControls[i].pCondition;
				break;
			}
			if (m_filterControls[i].pCondition == source)
			{
				target = m_filterControls[i].pValue;
				if (!target)
					m_filterControls[i].pSet;
				break;
			}
			if (m_filterControls[i].pSet == source || m_filterControls[i].pValue == source)
			{
				target = m_filterControls[i].pRemove;
				break;
			}
			if (m_filterControls[i].pRemove == source)
			{
				int j = i + 1;
				if (j == (int)m_filterControls.size())
					target = m_pAdd;
				else
					target = m_filterControls[j].pType;
				break;
			}
		}
	}
	else
	{
		if (source == m_pAdd)
		{
			if (m_filterControls.size())
				target = m_filterControls[m_filterControls.size() - 1].pRemove;
		}
		else
		{

			for (int i = 0; i < (int)m_filterControls.size(); i++)
			{
				if (m_filterControls[i].pType == source)
				{
					if (i > 0)
						target = m_filterControls[i - 1].pRemove;
					break;
				}
				if (m_filterControls[i].pCondition == source)
				{
					target = m_filterControls[i].pType;
					break;
				}
				if (m_filterControls[i].pSet == source || m_filterControls[i].pValue == source)
				{
					target = m_filterControls[i].pCondition;
					break;
				}
				if (m_filterControls[i].pRemove == source)
				{
					target = m_filterControls[i].pValue;
					if (!target)
						m_filterControls[i].pSet;
					break;
				}
			}
		}
	}

	if (target)
		target->SetFocus();
	else
		event.Skip();
}
