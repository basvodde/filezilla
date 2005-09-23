#ifndef __FILTEREDIT_H__
#define __FILTEREDIT_H__

#include "filter.h"
#include "dialogex.h"

class CFilterControls
{
public:
	CFilterControls();

	wxChoice* pType;
	wxChoice* pCondition;
	wxTextCtrl* pValue;
};

class wxCustomHeightListCtrl;
class CFilterEditDialog : public wxDialogEx
{
public:
	CFilterEditDialog() {}
	virtual ~CFilterEditDialog() { }

	bool Create(wxWindow* parent);

protected:

	DECLARE_EVENT_TABLE();
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnMore(wxCommandEvent& event);
	void OnRemove(wxCommandEvent& event);
	void OnFilterTypeChange(wxCommandEvent& event);
	void OnNew(wxCommandEvent& event);
	void OnDelete(wxCommandEvent& event);
	void OnRename(wxCommandEvent& event);
	void OnCopy(wxCommandEvent& event);
		
	void ShowFilter(const CFilter& filter);
	void SaveFilter(CFilter& filter);
	void MakeControls(const CFilterCondition& condition);
	void UpdateCount();

	wxCustomHeightListCtrl* m_pListCtrl;
	int m_choiceBoxHeight;

	CFilter m_currentFilter;
	std::vector<CFilterControls> m_filterControls;
};

#endif //__FILTEREDIT_H__
