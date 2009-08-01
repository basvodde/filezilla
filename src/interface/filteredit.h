#ifndef __FILTEREDIT_H__
#define __FILTEREDIT_H__

#include "filter.h"
#include "filter_conditions_dialog.h"

class wxCustomHeightListCtrl;
class CWindowStateManager;
class CFilterEditDialog : public CFilterConditionsDialog
{
public:
	CFilterEditDialog();
	virtual ~CFilterEditDialog();

	bool Create(wxWindow* parent, const std::vector<CFilter>& filters, const std::vector<CFilterSet>& filterSets);

	const std::vector<CFilter>& GetFilters() const;
	const std::vector<CFilterSet>& GetFilterSets() const;

	bool Validate();

protected:

	DECLARE_EVENT_TABLE();
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnNew(wxCommandEvent& event);
	void OnDelete(wxCommandEvent& event);
	void OnRename(wxCommandEvent& event);
	void OnCopy(wxCommandEvent& event);
	void OnFilterSelect(wxCommandEvent& event);
		
	void ShowFilter(const CFilter& filter);
	void SaveFilter(CFilter& filter);

	void SetCtrlState(bool enabled);

	wxListBox* m_pFilterListCtrl;
	int m_currentSelection;

	std::vector<CFilter> m_filters;
	std::vector<CFilterSet> m_filterSets;

	CWindowStateManager* m_pWindowStateManager;
};

#endif //__FILTEREDIT_H__
