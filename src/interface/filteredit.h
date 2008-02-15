#ifndef __FILTEREDIT_H__
#define __FILTEREDIT_H__

#include "filter.h"
#include "dialogex.h"

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
class CFilterEditDialog : public wxDialogEx
{
public:
	CFilterEditDialog() {}
	virtual ~CFilterEditDialog() { }

	bool Create(wxWindow* parent, const std::vector<CFilter>& filters, const std::vector<CFilterSet>& filterSets);

	const std::vector<CFilter>& GetFilters() const;
	const std::vector<CFilterSet>& GetFilterSets() const;

	bool Validate();

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
	void OnFilterSelect(wxCommandEvent& event);
		
	void ShowFilter(const CFilter& filter);
	void SaveFilter(CFilter& filter);
	void MakeControls(const CFilterCondition& condition, int i = -1);
	void DestroyControls();
	void UpdateCount();

	void CalcMinListWidth();

	void SetCtrlState(bool enabled);

	enum t_filterType GetTypeFromTypeSelection(int selection);
#ifndef __WXMSW__
	bool m_hasAttributes;
#else
	bool m_hasPermissions;
#endif

	wxCustomHeightListCtrl* m_pListCtrl;
	int m_choiceBoxHeight;

	wxListBox* m_pFilterListCtrl;
	int m_currentSelection;

	CFilter m_currentFilter;
	std::vector<CFilterControls> m_filterControls;

	std::vector<CFilter> m_filters;
	std::vector<CFilterSet> m_filterSets;
};

#endif //__FILTEREDIT_H__
