#ifndef __FILTER_CONDITIONS_DIALOG_H__
#define __FILTER_CONDITIONS_DIALOG_H__

#include "dialogex.h"
#include "filter.h"
#include <set>

class CFilterControls
{
public:
	CFilterControls();
	
	void Reset();

	wxChoice* pType;
	wxChoice* pCondition;
	wxTextCtrl* pValue;
	wxChoice* pSet;
	wxButton* pRemove;
};

class wxCustomHeightListCtrl;
class CFilterConditionsDialog : public wxDialogEx
{
public:
	CFilterConditionsDialog();

	// has_foreign_type for attributes on MSW, permissions on *nix
	// has_foreign_type for attributes on *nix, permissions on MSW
	bool CreateListControl(int conditions);

	void EditFilter(const CFilter& filter);
	CFilter GetFilter();
	void ClearFilter(bool disable);
	bool ValidateFilter(wxString& error, bool allow_empty = false);

private:
	void CalcMinListWidth();

	enum t_filterType GetTypeFromTypeSelection(int selection);
	void SetSelectionFromType(wxChoice* pChoice, enum t_filterType);

	void MakeControls(const CFilterCondition& condition, int i = -1);
	void DestroyControls();
	void UpdateConditionsClientSize();

	void SetFilterCtrlState(bool disable);

	bool m_has_foreign_type;

	wxCustomHeightListCtrl* m_pListCtrl;
	wxSize m_lastListSize;
	int m_choiceBoxHeight;

	std::vector<CFilterControls> m_filterControls;

	CFilter m_currentFilter;

	wxArrayString filterTypes;
	std::vector<t_filterType> filter_type_map;

	wxButton* m_pAdd;
	wxSize m_button_size;

	void OnMore();
	void OnRemove(int item);
	void OnRemove(const std::set<int> &selected);

	void OnListSize(wxSizeEvent& event);

	DECLARE_EVENT_TABLE();
	void OnButton(wxCommandEvent& event);
	void OnFilterTypeChange(wxCommandEvent& event);
	void OnConditionSelectionChange(wxCommandEvent& event);
	void OnNavigationKeyEvent(wxNavigationKeyEvent& event);
};

#endif //__FILTER_CONDITIONS_DIALOG_H__
