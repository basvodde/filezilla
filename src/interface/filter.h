#ifndef __FILTER_H__
#define __FILTER_H__

#include "dialogex.h"
#include <wx/regex.h>

enum t_filterType
{
	name,
	size,
	attributes,
	permissions,

	filterType_size
};

class CFilterCondition
{
public:
	CFilterCondition();
	CFilterCondition(const CFilterCondition& cond);

	virtual ~CFilterCondition();
	enum t_filterType type;
	int condition;
	wxString strValue;
	wxLongLong value;
	bool matchCase;
	wxRegEx* pRegEx;

	CFilterCondition& operator=(const CFilterCondition& cond);
};

class CFilter
{
public:
	enum t_matchType
	{
		all,
		any,
		none
	};

	CFilter();

	wxString name;

	bool filterFiles;
	bool filterDirs;
	enum t_matchType matchType;
	bool matchCase;

	std::vector<CFilterCondition> filters;

	bool HasConditionOfType(enum t_filterType type) const;
};

class CFilterSet
{
public:
	wxString name;
	std::vector<bool> local;
	std::vector<bool> remote;
};

class CFilterManager
{
public:
	CFilterManager();

	// Note: Under non-windows, attributes are permissions
	bool FilenameFiltered(const wxString& name, bool dir, wxLongLong size, bool local, int attributes) const;
	bool HasActiveFilters() const;

	bool HasSameLocalAndRemoteFilters() const;

protected:
	bool CompileRegexes();
	bool FilenameFilteredByFilter(const wxString& name, bool dir, wxLongLong size, unsigned int filterIndex, int attributes) const;

	void LoadFilters();
	static bool m_loaded;

	static std::vector<CFilter> m_globalFilters;
	std::vector<CFilter> m_filters;

	static std::vector<CFilterSet> m_globalFilterSets;
	std::vector<CFilterSet> m_filterSets;
	unsigned int m_currentFilterSet;
	static unsigned int m_globalCurrentFilterSet;
};

class CMainFrame;
class CFilterDialog : public wxDialogEx, public CFilterManager
{
public:
	CFilterDialog();
	virtual ~CFilterDialog() { }

	bool Create(CMainFrame* parent);

protected:

	void SaveFilters();

	void DisplayFilters();

	DECLARE_EVENT_TABLE();
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnApply(wxCommandEvent& event);
	void OnEdit(wxCommandEvent& event);
	void OnFilterSelect(wxCommandEvent& event);
	void OnMouseEvent(wxMouseEvent& event);
	void OnKeyEvent(wxKeyEvent& event);
	void OnSaveAs(wxCommandEvent& event);
	void OnDeleteSet(wxCommandEvent& event);
	void OnSetSelect(wxCommandEvent& event);

	void OnChangeAll(wxCommandEvent& event);

	bool m_shiftClick;

	CMainFrame* m_pMainFrame;
};

#endif //__FILTER_H__
