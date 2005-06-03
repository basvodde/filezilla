#ifndef __SETTINGSDIALOG_H__
#define __SETTINGSDIALOG_H__

#include "dialogex.h"

class COptions;
class COptionsPage;
class CSettingsDialog : public wxDialogEx
{
public:
	CSettingsDialog(COptions* pOptions);
	virtual ~CSettingsDialog();

	bool Create(wxWindow* parent);

protected:
	bool LoadPages();

	COptions* m_pOptions;
	
	COptionsPage* m_activePanel;

	struct t_page
	{
		wxTreeItemId id;
		COptionsPage* page;
	};
	std::vector<t_page> m_pages;

	DECLARE_EVENT_TABLE()
	void OnPageChanging(wxTreeEvent& event);
	void OnPageChanged(wxTreeEvent& event);
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
};

#endif //__SETTINGSDIALOG_H__
