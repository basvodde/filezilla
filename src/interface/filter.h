#ifndef __FILTER_H__
#define __FILTER_H__

#include "dialogex.h"

class CFilterDialog : public wxDialogEx
{
public:
	CFilterDialog() {}
	virtual ~CFilterDialog() { }

	bool Create(wxWindow* parent);

protected:

	DECLARE_EVENT_TABLE();
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnEdit(wxCommandEvent& event);
};

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
	bool filterFiles;
	bool filterDirs;
	bool matchAll;

	std::vector<CFilterCondition> filters;
};

#endif //__FILTER_H__
