#ifndef __OPTIONSPAGE_THEMES_H__
#define __OPTIONSPAGE_THEMES_H__

class COptionsPageThemes : public COptionsPage
{
public:
	virtual ~COptionsPageThemes();
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_THEMES"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:
	bool DisplayTheme(const wxString& theme);

	DECLARE_EVENT_TABLE();
	void OnThemeChange(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_THEMES_H__
