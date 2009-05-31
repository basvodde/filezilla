#ifndef __FILTER_CONDITIONS_DIALOG_H__
#define __FILTER_CONDITIONS_DIALOG_H__

#include "dialogex.h"
#include "filter.h"

class CFilterControls
{
public:
	CFilterControls();
	
	void Reset();

	wxChoice* pType;
	wxChoice* pCondition;
	wxTextCtrl* pValue;
	wxChoice* pSet;
};

class wxCustomHeightListCtrl;
class CFilterConditionsDialog : public wxDialogEx
{
public:
	CFilterConditionsDialog();

	// has_foreign_type for attributes on *nix, permissions on MSW
	bool CreateListControl(bool has_foreign_type);

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
	int m_choiceBoxHeight;

	std::vector<CFilterControls> m_filterControls;

	CFilter m_currentFilter;

	DECLARE_EVENT_TABLE();
	void OnMore(wxCommandEvent& event);
	void OnRemove(wxCommandEvent& event);
	void OnFilterTypeChange(wxCommandEvent& event);
	void OnConditionSelectionChange(wxCommandEvent& event);
};

#endif //__FILTER_CONDITIONS_DIALOG_H__
