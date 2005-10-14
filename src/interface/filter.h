#ifndef __FILTER_H__
#define __FILTER_H__

#include "dialogex.h"
#include <wx/regex.h>

class CFilterCondition
{
public:
	CFilterCondition();
	virtual ~CFilterCondition();
	int type;
	int condition;
	wxString strValue;
	bool matchCase;
	wxRegEx* pRegEx;

	CFilterCondition& operator=(const CFilterCondition& cond);
};

class CFilter
{
public:
	CFilter();

	wxString name;

	bool filterFiles;
	bool filterDirs;
	bool matchAll;
	bool matchCase;

	std::vector<CFilterCondition> filters;
};

class CFilterSet
{
public:
	std::vector<bool> local;
	std::vector<bool> remote;
};

class CFilterDialog : public wxDialogEx
{
public:
	CFilterDialog();
	virtual ~CFilterDialog() { }

	bool Create(wxWindow* parent);

	bool FilenameFiltered(const wxString& name, bool local) const;

protected:

	bool CompileRegexes();
	bool FilenameFilteredByFilter(const wxString& name, unsigned int filterIndex) const;

	void SaveFilters();
	void LoadFilters();

	void DisplayFilters();

	static bool m_loaded;

	static std::vector<CFilter> m_globalFilters;
	std::vector<CFilter> m_filters;

	static std::vector<CFilterSet> m_globalFilterSets;
	std::vector<CFilterSet> m_filterSets;

	DECLARE_EVENT_TABLE();
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnEdit(wxCommandEvent& event);
	void OnFilterSelect(wxCommandEvent& event);
	void OnMouseEvent(wxMouseEvent& event);
	void OnKeyEvent(wxKeyEvent& event);

	bool m_shiftClick;
};

#endif //__FILTER_H__
