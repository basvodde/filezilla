#include "FileZilla.h"
#include "filter.h"
#include "filteredit.h"
#include "ipcmutex.h"
#include "filezillaapp.h"
#include "../tinyxml/tinyxml.h"
#include "xmlfunctions.h"
#include <wx/regex.h>
#include "Mainfrm.h"
#include "inputdialog.h"
#include "state.h"

bool CFilterDialog::m_loaded = false;
std::vector<CFilter> CFilterDialog::m_globalFilters;
std::vector<CFilterSet> CFilterDialog::m_globalFilterSets;
unsigned int CFilterDialog::m_globalCurrentFilterSet = 0;

BEGIN_EVENT_TABLE(CFilterDialog, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CFilterDialog::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CFilterDialog::OnCancel)
EVT_BUTTON(XRCID("wxID_APPLY"), CFilterDialog::OnApply)
EVT_BUTTON(XRCID("ID_EDIT"), CFilterDialog::OnEdit)
EVT_CHECKLISTBOX(wxID_ANY, CFilterDialog::OnFilterSelect)
EVT_BUTTON(XRCID("ID_SAVESET"), CFilterDialog::OnSaveAs)
EVT_BUTTON(XRCID("ID_DELETESET"), CFilterDialog::OnDeleteSet)
EVT_CHOICE(XRCID("ID_SETS"), CFilterDialog::OnSetSelect)

EVT_BUTTON(XRCID("ID_LOCAL_ENABLEALL"), CFilterDialog::OnChangeAll)
EVT_BUTTON(XRCID("ID_LOCAL_DISABLEALL"), CFilterDialog::OnChangeAll)
EVT_BUTTON(XRCID("ID_REMOTE_ENABLEALL"), CFilterDialog::OnChangeAll)
EVT_BUTTON(XRCID("ID_REMOTE_DISABLEALL"), CFilterDialog::OnChangeAll)
END_EVENT_TABLE();

CFilterCondition::CFilterCondition()
{
	type = 0;
	condition = 0;
	pRegEx = 0;
	matchCase = true;
	value = 0;
}

CFilterCondition::~CFilterCondition()
{
	delete pRegEx;
}

CFilterCondition& CFilterCondition::operator=(const CFilterCondition& cond)
{
	type = cond.type;
	condition = cond.condition;
	strValue = cond.strValue;
	value = cond.value;
	matchCase = cond.matchCase;
	delete pRegEx;
	pRegEx = 0;

	return *this;
}

CFilter::CFilter()
{
	matchType = all;
	filterDirs = true;
	filterFiles = true;

	// Filenames on Windows ignore case
#ifdef __WXMSW__
	matchCase = false;
#else
	matchCase = true;
#endif
}

CFilterDialog::CFilterDialog()
{
	m_currentFilterSet = 0;
	m_pMainFrame = 0;
	m_shiftClick = false;
	
	LoadFilters();
	CompileRegexes();

	if (m_globalFilterSets.empty())
	{
		CFilterSet set;
		set.local.resize(m_filters.size(), false);
		set.remote.resize(m_filters.size(), false);

		m_globalFilterSets.push_back(set);
		m_filterSets.push_back(set);
	}
}

bool CFilterDialog::Create(CMainFrame* parent)
{
	m_pMainFrame = parent;

	if (!Load(parent, _T("ID_FILTER")))
		return false;

	XRCCTRL(*this, "ID_LOCALFILTERS", wxCheckListBox)->Connect(wxID_ANY, wxEVT_LEFT_DOWN, wxMouseEventHandler(CFilterDialog::OnMouseEvent), 0, this);
	XRCCTRL(*this, "ID_LOCALFILTERS", wxCheckListBox)->Connect(wxID_ANY, wxEVT_KEY_DOWN, wxKeyEventHandler(CFilterDialog::OnKeyEvent), 0, this);
	XRCCTRL(*this, "ID_REMOTEFILTERS", wxCheckListBox)->Connect(wxID_ANY, wxEVT_LEFT_DOWN, wxMouseEventHandler(CFilterDialog::OnMouseEvent), 0, this);
	XRCCTRL(*this, "ID_REMOTEFILTERS", wxCheckListBox)->Connect(wxID_ANY, wxEVT_KEY_DOWN, wxKeyEventHandler(CFilterDialog::OnKeyEvent), 0, this);

	DisplayFilters();

	wxChoice* pChoice = XRCCTRL(*this, "ID_SETS", wxChoice);
	wxString name = _("Custom filter set");
	pChoice->Append(_T("<") + name + _T(">"));
	for (unsigned int i = 1; i < m_filterSets.size(); i++)
		pChoice->Append(m_filterSets[i].name);
	pChoice->SetSelection(m_currentFilterSet);

	GetSizer()->Fit(this);

	return true;
}

void CFilterDialog::OnOK(wxCommandEvent& event)
{
	m_globalFilters = m_filters;
	CompileRegexes();
	m_globalFilterSets = m_filterSets;
	m_globalCurrentFilterSet = m_currentFilterSet;

	SaveFilters();
	EndModal(wxID_OK);
}

void CFilterDialog::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

void CFilterDialog::OnEdit(wxCommandEvent& event)
{
	CFilterEditDialog dlg;
	if (!dlg.Create(this, m_filters, m_filterSets))
		return;
	
	if (dlg.ShowModal() != wxID_OK)
		return;

	m_filters = dlg.GetFilters();
	m_filterSets = dlg.GetFilterSets();

	DisplayFilters();
}

void CFilterDialog::SaveFilters()
{
	CInterProcessMutex(MUTEX_FILTERS);

	wxFileName file(wxGetApp().GetSettingsDir(), _T("filters.xml"));
	TiXmlElement* pDocument = GetXmlFile(file);
	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager could not be saved."), file.GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return;
	}

	TiXmlElement *pFilters = pDocument->FirstChildElement("Filters");
	while (pFilters)
	{
		pDocument->RemoveChild(pFilters);
		pFilters = pDocument->FirstChildElement("Filters");
	}

	pFilters = pDocument->InsertEndChild(TiXmlElement("Filters"))->ToElement();

	for (std::vector<CFilter>::const_iterator iter = m_globalFilters.begin(); iter != m_globalFilters.end(); iter++)
	{
		const CFilter& filter = *iter;
		TiXmlElement* pFilter = pFilters->InsertEndChild(TiXmlElement("Filter"))->ToElement();

		AddTextElement(pFilter, "Name", filter.name);
		AddTextElement(pFilter, "ApplyToFiles", filter.filterFiles ? _T("1") : _T("0"));
		AddTextElement(pFilter, "ApplyToDirs", filter.filterDirs ? _T("1") : _T("0"));
		AddTextElement(pFilter, "MatchType", (filter.matchType == CFilter::any) ? _T("Any") : ((filter.matchType == CFilter::none) ? _T("None") : _T("All")));
		AddTextElement(pFilter, "MatchCase", filter.matchCase ? _T("1") : _T("0"));

		TiXmlElement* pConditions = pFilter->InsertEndChild(TiXmlElement("Conditions"))->ToElement();
		for (std::vector<CFilterCondition>::const_iterator conditionIter = filter.filters.begin(); conditionIter != filter.filters.end(); conditionIter++)
		{
			const CFilterCondition& condition = *conditionIter;
			TiXmlElement* pCondition = pConditions->InsertEndChild(TiXmlElement("Condition"))->ToElement();

			AddTextElement(pCondition, "Type", wxString::Format(_T("%d"), condition.type));
			AddTextElement(pCondition, "Condition", wxString::Format(_T("%d"), condition.condition));
			AddTextElement(pCondition, "Value", condition.strValue);
		}
	}

	TiXmlElement *pSets = pDocument->FirstChildElement("Sets");
	while (pSets)
	{
		pDocument->RemoveChild(pSets);
		pSets = pDocument->FirstChildElement("Sets");
	}

	pSets = pDocument->InsertEndChild(TiXmlElement("Sets"))->ToElement();
	SetTextAttribute(pSets, "Current", wxString::Format(_T("%d"), m_currentFilterSet));

	for (std::vector<CFilterSet>::const_iterator iter = m_globalFilterSets.begin(); iter != m_globalFilterSets.end(); iter++)
	{
		const CFilterSet& set = *iter;
		TiXmlElement* pSet = pSets->InsertEndChild(TiXmlElement("Set"))->ToElement();

		if (iter != m_globalFilterSets.begin())
			AddTextElement(pSet, "Name", set.name);

		for (unsigned int i = 0; i < set.local.size(); i++)
		{
			TiXmlElement* pItem = pSet->InsertEndChild(TiXmlElement("Item"))->ToElement();
			AddTextElement(pItem, "Local", set.local[i] ? _T("1") : _T("0"));
			AddTextElement(pItem, "Remote", set.remote[i] ? _T("1") : _T("0"));
		}
	}

	SaveXmlFile(file, pDocument);
	delete pDocument->GetDocument();
}

void CFilterDialog::LoadFilters()
{
	if (m_loaded)
	{
		m_filters = m_globalFilters;
		m_filterSets = m_globalFilterSets;
		return;
	}
	m_loaded = true;

	CInterProcessMutex(MUTEX_FILTERS);

	wxFileName file(wxGetApp().GetSettingsDir(), _T("filters.xml"));
	TiXmlElement* pDocument = GetXmlFile(file);
	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed.\nAny changes made in the Site Manager could not be saved."), file.GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return;
	}

	TiXmlElement *pFilters = pDocument->FirstChildElement("Filters");

	if (!pFilters)
	{
		delete pDocument->GetDocument();
		return;
	}

	TiXmlElement *pFilter = pFilters->FirstChildElement("Filter");
	while (pFilter)
	{
		CFilter filter;
		filter.name = GetTextElement(pFilter, "Name");
		if (filter.name == _T(""))
		{
			pFilter = pFilter->NextSiblingElement("Filter");
			continue;
		}

		filter.filterFiles = GetTextElement(pFilter, "ApplyToFiles") == _T("1");
		filter.filterDirs = GetTextElement(pFilter, "ApplyToDirs") == _T("1");

		wxString type = GetTextElement(pFilter, "MatchType");
		if (type == _T("Any"))
			filter.matchType = CFilter::any;
		else if (type == _T("None"))
			filter.matchType = CFilter::none;
		else
			filter.matchType = CFilter::all;
		filter.matchCase = GetTextElement(pFilter, "MatchCase") == _T("1");

		TiXmlElement *pConditions = pFilter->FirstChildElement("Conditions");
		if (!pConditions)
		{
			pFilter = pFilter->NextSiblingElement("Filter");
			continue;
		}
		
		TiXmlElement *pCondition = pConditions->FirstChildElement("Condition");
		while (pCondition)
		{
			CFilterCondition condition;
			condition.type = GetTextElementInt(pCondition, "Type", 0);
			condition.condition = GetTextElementInt(pCondition, "Condition", 0);
			condition.strValue = GetTextElement(pCondition, "Value");
			condition.matchCase = filter.matchCase;
			if (condition.strValue == _T(""))
			{
				pCondition = pCondition->NextSiblingElement("Condition");
				continue;
			}

			// TODO: 64bit filesize
			if (condition.type == 1)
			{
				unsigned long tmp;
				condition.strValue.ToULong(&tmp);
				condition.value = tmp;
			}

			filter.filters.push_back(condition);

			pCondition = pCondition->NextSiblingElement("Condition");
		}

		if (!filter.filters.empty())
			m_globalFilters.push_back(filter);

		pFilter = pFilter->NextSiblingElement("Filter");
	}
	m_filters = m_globalFilters;

	TiXmlElement* pSets = pDocument->FirstChildElement("Sets");
	if (!pSets)
	{
		delete pDocument->GetDocument();
		return;
	}

	for (TiXmlElement* pSet = pSets->FirstChildElement("Set"); pSet; pSet = pSet->NextSiblingElement("Set"))
	{
		CFilterSet set;
		TiXmlElement* pItem = pSet->FirstChildElement("Item");
		while (pItem)
		{
			wxString local = GetTextElement(pItem, "Local");
			wxString remote = GetTextElement(pItem, "Remote");
			set.local.push_back(local == _T("1") ? true : false);
			set.remote.push_back(remote == _T("1") ? true : false);

			pItem = pItem->NextSiblingElement("Item");
		}

		if (!m_globalFilterSets.empty())
		{
			set.name = GetTextElement(pSet, "Name");
			if (set.name == _T(""))
				continue;
		}

		if (set.local.size() == m_filters.size())
			m_globalFilterSets.push_back(set);
	}
	m_filterSets = m_globalFilterSets;

	wxString attribute = GetTextAttribute(pSets, "Current");
	unsigned long value;
	if (attribute.ToULong(&value))
	{
		if (value < m_globalFilterSets.size())
			m_globalCurrentFilterSet = value;
	}

	m_currentFilterSet = m_globalCurrentFilterSet;

	delete pDocument->GetDocument();
}

void CFilterDialog::DisplayFilters()
{
	wxCheckListBox* pLocalFilters = XRCCTRL(*this, "ID_LOCALFILTERS", wxCheckListBox);
	wxCheckListBox* pRemoteFilters = XRCCTRL(*this, "ID_REMOTEFILTERS", wxCheckListBox);

	pLocalFilters->Clear();
	pRemoteFilters->Clear();

	for (unsigned int i = 0; i < m_filters.size(); i++)
	{
		const CFilter& filter = m_filters[i];
		pLocalFilters->Append(filter.name);
		pRemoteFilters->Append(filter.name);

		pLocalFilters->Check(i, m_filterSets[m_currentFilterSet].local[i]);
		pRemoteFilters->Check(i, m_filterSets[m_currentFilterSet].remote[i]);
	}
}

void CFilterDialog::OnMouseEvent(wxMouseEvent& event)
{
	m_shiftClick = event.ShiftDown();
	event.Skip();
}

void CFilterDialog::OnKeyEvent(wxKeyEvent& event)
{
	m_shiftClick = event.ShiftDown();
	event.Skip();
}

void CFilterDialog::OnFilterSelect(wxCommandEvent& event)
{
	wxCheckListBox* pLocal = XRCCTRL(*this, "ID_LOCALFILTERS", wxCheckListBox);
	wxCheckListBox* pRemote = XRCCTRL(*this, "ID_REMOTEFILTERS", wxCheckListBox);

	int item = event.GetSelection();

	if (m_shiftClick)
	{
		if (event.GetEventObject() == pLocal)
			pRemote->Check(item, pLocal->IsChecked(event.GetSelection()));
		else
			pLocal->Check(item, pRemote->IsChecked(event.GetSelection()));
	}

	if (m_currentFilterSet)
	{
		m_filterSets[0] = m_filterSets[m_currentFilterSet];
		m_currentFilterSet = 0;
		wxChoice* pChoice = XRCCTRL(*this, "ID_SETS", wxChoice);
		pChoice->SetSelection(0);
	}

	bool localChecked = pLocal->IsChecked(event.GetSelection());
	bool remoteChecked = pRemote->IsChecked(event.GetSelection());
	m_filterSets[0].local[item] = localChecked;
	m_filterSets[0].remote[item] = remoteChecked;
}

bool CFilterDialog::FilenameFiltered(const wxString& name, bool dir, wxLongLong size, bool local) const
{
	wxASSERT(m_currentFilterSet < m_filterSets.size());

	// Check active filters
	for (unsigned int i = 0; i < m_filters.size(); i++)
	{
		if (local)
		{
			if (m_filterSets[m_currentFilterSet].local[i])
				if (FilenameFilteredByFilter(name, dir, size, i))
					return true;
		}
		else
		{
			if (m_filterSets[m_currentFilterSet].remote[i])
				if (FilenameFilteredByFilter(name, dir, size, i))
					return true;
		}
	}

	return false;
}

bool CFilterDialog::FilenameFilteredByFilter(const wxString& name, bool dir, wxLongLong size, unsigned int filterIndex) const
{
	wxRegEx regex;
	const CFilter& filter = m_globalFilters[filterIndex];

	if (dir && !filter.filterDirs)
		return false;
	else if (!dir && !filter.filterFiles)
		return false;
    
	for (std::vector<CFilterCondition>::const_iterator iter = filter.filters.begin(); iter != filter.filters.end(); iter++)
	{
		bool match = false;
		const CFilterCondition& condition = *iter;

		switch (condition.type)
		{
		case 0:
			switch (condition.condition)
			{
			case 0:
				if (filter.matchCase)
				{
					if (name.Contains(condition.strValue))
						match = true;
				}
				else
				{
					if (name.Lower().Contains(condition.strValue.Lower()))
						match = true;
				}
				break;
			case 1:
				if (filter.matchCase)
				{
					if (name == condition.strValue)
						match = true;
				}
				else
				{
					if (!name.CmpNoCase(condition.strValue))
						match = true;
				}
				break;
			case 2:
				{
					const wxString& left = name.Left(condition.strValue.Len());
					if (filter.matchCase)
					{
						if (left == condition.strValue)
							match = true;
					}
					else
					{
						if (!left.CmpNoCase(condition.strValue))
							match = true;
					}
				}
				break;
			case 3:
				{
					const wxString& right = name.Right(condition.strValue.Len());
					if (filter.matchCase)
					{
						if (right == condition.strValue)
							match = true;
					}
					else
					{
						if (!right.CmpNoCase(condition.strValue))
							match = true;
					}
				}
				break;
			case 4:
				wxASSERT(condition.pRegEx);
				if (condition.pRegEx && condition.pRegEx->Matches(name))
					match = true;
			}
			break;
		case 1:
			if (size == -1)
				continue;
			switch (condition.condition)
			{
			case 0:
				if (size > condition.value)
					match = true;
				break;
			case 1:
				if (size == condition.value)
					match = true;
				break;
			case 2:
				if (size < condition.value)
					match = true;
				break;
			}
			break;
		}
		if (match)
		{
			if (filter.matchType == CFilter::any)
				return true;
			else if (filter.matchType == CFilter::none)
				return false;
		}
		else
		{
			if (filter.matchType == CFilter::all)
				return false;
		}
	}

	if (filter.matchType != CFilter::any)
		return true;
	
	return false;
}

bool CFilterDialog::CompileRegexes()
{
	for (unsigned int i = 0; i < m_globalFilters.size(); i++)
	{
		CFilter& filter = m_globalFilters[i];
		for (std::vector<CFilterCondition>::iterator iter = filter.filters.begin(); iter != filter.filters.end(); iter++)
		{
			CFilterCondition& condition = *iter;
			delete condition.pRegEx;
			if (!condition.type && condition.condition == 4)
				condition.pRegEx = new wxRegEx(condition.strValue);
			else
				condition.pRegEx = 0;
		}
	}
	return true;
}

void CFilterDialog::OnSaveAs(wxCommandEvent& event)
{
	CInputDialog dlg;
	dlg.Create(this, _("Enter name for filterset"), _("Please enter a unique name for this filter set"));
	if (dlg.ShowModal() != wxID_OK)
		return;

	wxString name = dlg.GetValue();
	wxChoice* pChoice = XRCCTRL(*this, "ID_SETS", wxChoice);
	int pos = pChoice->FindString(name);
	if (pos != wxNOT_FOUND)
	{
		if (wxMessageBox(_("Given filterset name already exists, overwrite filter set?"), _("Filter set already exists"), wxICON_QUESTION | wxYES_NO) != wxYES)
			return;
	}

	if (pos == wxNOT_FOUND)
	{
		pos = m_filterSets.size();
		m_filterSets.push_back(m_filterSets[0]);
		pChoice->Append(name);
	}
	else
		m_filterSets[pos] = m_filterSets[0];

	m_filterSets[pos].name = name;

	pChoice->SetSelection(pos);
	m_currentFilterSet = pos;
}

void CFilterDialog::OnDeleteSet(wxCommandEvent& event)
{
	wxChoice* pChoice = XRCCTRL(*this, "ID_SETS", wxChoice);
	int pos = pChoice->GetSelection();
	if (pos == -1)
		return;

	if (!pos)
	{
		wxMessageBox(_("This filter set cannot be removed"));
		return;
	}

	m_filterSets[0] = m_filterSets[pos];

	pChoice->Delete(pos);
	m_filterSets.erase(m_filterSets.begin() + pos);
	wxASSERT(!m_filterSets.empty());

	pChoice->SetSelection(0);
	m_currentFilterSet = 0;
}

void CFilterDialog::OnSetSelect(wxCommandEvent& event)
{
	m_currentFilterSet = event.GetSelection();
	DisplayFilters();
}

void CFilterDialog::OnChangeAll(wxCommandEvent& event)
{
	bool check = true;
	if (event.GetId() == XRCID("ID_LOCAL_DISABLEALL") || event.GetId() == XRCID("ID_REMOTE_DISABLEALL"))
		check = false;

	std::vector<bool>* pValues;
	wxCheckListBox* pListBox;
	if (event.GetId() == XRCID("ID_LOCAL_ENABLEALL") || event.GetId() == XRCID("ID_LOCAL_DISABLEALL"))
	{
		pListBox = XRCCTRL(*this, "ID_LOCALFILTERS", wxCheckListBox);
		pValues = &m_filterSets[0].local;
	}
	else
	{
		pListBox = XRCCTRL(*this, "ID_REMOTEFILTERS", wxCheckListBox);
		pValues = &m_filterSets[0].remote;
	}

	if (m_currentFilterSet)
	{
		m_filterSets[0] = m_filterSets[m_currentFilterSet];
		m_currentFilterSet = 0;
		wxChoice* pChoice = XRCCTRL(*this, "ID_SETS", wxChoice);
		pChoice->SetSelection(0);
	}

	for (size_t i = 0; i < pListBox->GetCount(); i++)
	{
		pListBox->Check(i, check);
		(*pValues)[i] = check;
	}
}

void CFilterDialog::OnApply(wxCommandEvent& event)
{
	m_globalFilters = m_filters;
	CompileRegexes();
	m_globalFilterSets = m_filterSets;
	m_globalCurrentFilterSet = m_currentFilterSet;

	SaveFilters();

	m_pMainFrame->GetState()->ApplyCurrentFilter();
}

bool CFilterDialog::HasActiveFilters() const
{
	wxASSERT(m_currentFilterSet < m_filterSets.size());

	for (unsigned int i = 0; i < m_filters.size(); i++)
	{
		if (m_filterSets[m_currentFilterSet].local[i])
			return true;

		if (m_filterSets[m_currentFilterSet].remote[i])
			return true;
	}

	return false;
}
