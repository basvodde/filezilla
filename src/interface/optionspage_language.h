#ifndef __OPTIONSPAGE_LANGUAGE_H__
#define __OPTIONSPAGE_LANGUAGE_H__

class COptionsPageLanguage : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_LANGUAGE"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:
	DECLARE_EVENT_TABLE();

	std::map<wxString, wxString> m_localeMap; // Maps language codes to language names
};

#endif //__OPTIONSPAGE_LANGUAGE_H__
