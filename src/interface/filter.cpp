#include "FileZilla.h"
#include "filter.h"
#include "filteredit.h"
#include "ipcmutex.h"
#include "filezillaapp.h"
#include "../tinyxml/tinyxml.h"
#include "xmlfunctions.h"

bool CFilterDialog::m_loaded = false;
std::vector<CFilter> CFilterDialog::m_globalFilters;

BEGIN_EVENT_TABLE(CFilterDialog, wxDialogEx)
EVT_BUTTON(XRCID("wxID_OK"), CFilterDialog::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CFilterDialog::OnCancel)
EVT_BUTTON(XRCID("ID_EDIT"), CFilterDialog::OnEdit)
EVT_CHECKLISTBOX(wxID_ANY, CFilterDialog::OnFilterSelect)
END_EVENT_TABLE();

CFilterCondition::CFilterCondition()
{
	type = 0;
	condition = 0;
	value = 0;
}

CFilterDialog::CFilterDialog()
{
	m_shiftClick = false;
}

bool CFilterDialog::Create(wxWindow* parent)
{
	LoadFilters();

	if (!Load(parent, _T("ID_FILTER")))
		return false;

	XRCCTRL(*this, "ID_LOCALFILTERS", wxCheckListBox)->Connect(wxID_ANY, wxEVT_LEFT_DOWN, wxMouseEventHandler(CFilterDialog::OnMouseEvent), 0, this);
	XRCCTRL(*this, "ID_LOCALFILTERS", wxCheckListBox)->Connect(wxID_ANY, wxEVT_KEY_DOWN, wxKeyEventHandler(CFilterDialog::OnKeyEvent), 0, this);
	XRCCTRL(*this, "ID_REMOTEFILTERS", wxCheckListBox)->Connect(wxID_ANY, wxEVT_LEFT_DOWN, wxMouseEventHandler(CFilterDialog::OnMouseEvent), 0, this);
	XRCCTRL(*this, "ID_REMOTEFILTERS", wxCheckListBox)->Connect(wxID_ANY, wxEVT_KEY_DOWN, wxKeyEventHandler(CFilterDialog::OnKeyEvent), 0, this);

	DisplayFilters();

	return true;
}

void CFilterDialog::OnOK(wxCommandEvent& event)
{
	m_globalFilters = m_filters;
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
		AddTextElement(pFilter, "ApplyToDirs", filter.filterFiles ? _T("1") : _T("0"));
		AddTextElement(pFilter, "MatchType", filter.matchAll ? _T("All") : _T("Any"));

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

	SaveXmlFile(file, pDocument);
	delete pDocument->GetDocument();
}

void CFilterDialog::LoadFilters()
{
	if (m_loaded)
	{
		m_filters = m_globalFilters;
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

		filter.matchAll = GetTextElement(pFilter, "MatchType") == _T("All");

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
			if (condition.strValue == _T(""))
			{
				pCondition = pCondition->NextSiblingElement("Condition");
				continue;
			}

			filter.filters.push_back(condition);

			pCondition = pCondition->NextSiblingElement("Condition");
		}

		if (!filter.filters.empty())
			m_globalFilters.push_back(filter);

		pFilter = pFilter->NextSiblingElement("Filter");
	}

	delete pDocument->GetDocument();

	m_filters = m_globalFilters;
}

void CFilterDialog::DisplayFilters()
{
	wxCheckListBox* pLocalFilters = XRCCTRL(*this, "ID_LOCALFILTERS", wxCheckListBox);
	wxCheckListBox* pRemoteFilters = XRCCTRL(*this, "ID_REMOTEFILTERS", wxCheckListBox);

	pLocalFilters->Clear();
	pRemoteFilters->Clear();

	for (std::vector<CFilter>::const_iterator iter = m_filters.begin(); iter != m_filters.end(); iter++)
	{
		const CFilter& filter = *iter;
		pLocalFilters->Append(filter.name);
		pRemoteFilters->Append(filter.name);
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
	if (!m_shiftClick)
		return;

	wxCheckListBox* pLocal = XRCCTRL(*this, "ID_LOCALFILTERS", wxCheckListBox);
	wxCheckListBox* pRemote = XRCCTRL(*this, "ID_REMOTEFILTERS", wxCheckListBox);

	if (event.GetEventObject() == pLocal)
		pRemote->Check(event.GetSelection(), pLocal->IsChecked(event.GetSelection()));
	else
		pLocal->Check(event.GetSelection(), pRemote->IsChecked(event.GetSelection()));
}
