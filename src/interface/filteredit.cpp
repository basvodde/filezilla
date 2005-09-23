#include "FileZilla.h"
#include "filteredit.h"
#include "customheightlistctrl.h"

const wxString filterTypes[] = { _("Filename"), _("Filesize"), _("Filetype") };
const wxString stringConditionTypes[] = { _("contains"), _("is equal to"), _("matches regex") };
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
END_EVENT_TABLE();

void CFilterEditDialog::OnOK(wxCommandEvent& event)
{
	EndModal(wxID_OK);
}

void CFilterEditDialog::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

bool CFilterEditDialog::Create(wxWindow* parent)
{
	if (!Load(parent, _T("ID_EDITFILTER")))
		return false;

	wxScrolledWindow* wnd = XRCCTRL(*this, "ID_CONDITIONS", wxScrolledWindow);
	m_pListCtrl = new wxCustomHeightListCtrl(wnd, wxID_ANY, wxDefaultPosition, wnd->GetSize(), wxVSCROLL|wxSUNKEN_BORDER);
	if (!m_pListCtrl)
		return false;

	m_choiceBoxHeight = 0;

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
	// First delete old controls
	for (unsigned int i = 0; i < m_filterControls.size(); i++)
	{
		CFilterControls& controls = m_filterControls[i];
		delete controls.pType;
		delete controls.pCondition;
		delete controls.pValue;
	}
	m_filterControls.clear();

	// Create new controls
	m_currentFilter = filter;
	for (unsigned int i = 0; i < filter.filters.size(); i++)
	{
		const CFilterCondition& cond = filter.filters[i];

		MakeControls(cond);
	}

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
	for (unsigned int i = 0; i < m_currentFilter.filters.size(); i++)
	{
		const CFilterControls& controls = m_filterControls[i];
		CFilterCondition condition = m_currentFilter.filters[i];
		
		// We don't have to save the type, it gets updated in OnFilterTypeChange
		condition.condition = controls.pCondition->GetSelection();
		condition.strValue = controls.pValue->GetValue();
	}
}

void CFilterEditDialog::OnNew(wxCommandEvent& event)
{
}

void CFilterEditDialog::OnDelete(wxCommandEvent& event)
{
}

void CFilterEditDialog::OnRename(wxCommandEvent& event)
{
}

void CFilterEditDialog::OnCopy(wxCommandEvent& event)
{
}
