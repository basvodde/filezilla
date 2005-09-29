#ifndef __FILTER_H__
#define __FILTER_H__

#include "dialogex.h"

class CFilterCondition
{
public:
	CFilterCondition();
	int type;
	int condition;
	wxString strValue;
	int value;
};

class CFilter
{
public:
	wxString name;

	bool filterFiles;
	bool filterDirs;
	bool matchAll;

	std::vector<CFilterCondition> filters;
};

class CFilterDialog : public wxDialogEx
{
public:
	CFilterDialog();
	virtual ~CFilterDialog() { }

	bool Create(wxWindow* parent);

protected:

	void SaveFilters();
	void LoadFilters();
	static bool m_loaded;
	static std::vector<CFilter> m_globalFilters;
	std::vector<CFilter> m_filters;

	DECLARE_EVENT_TABLE();
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnEdit(wxCommandEvent& event);
};

#endif //__FILTER_H__
