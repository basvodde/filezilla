#ifndef __SETTINGSDIALOG_H__
#define __SETTINGSDIALOG_H__

class COptions;
class COptionsPage;
class CSettingsDialog : public wxDialog
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
	void OnChar(wxKeyEvent& event);
};

#endif //__SETTINGSDIALOG_H__
