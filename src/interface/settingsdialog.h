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

	DECLARE_EVENT_TABLE()
	void OnPageChanged(wxTreeEvent& event);

	COptionsPage* m_activePanel;

	struct t_page
	{
		wxTreeItemId id;
		COptionsPage* page;
	};
	std::vector<t_page> m_pages;
};

#endif //__SETTINGSDIALOG_H__