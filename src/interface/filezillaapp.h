#ifndef __FILEZILLAAPP_H__
#define __FILEZILLAAPP_H__

class CFileZillaApp : public wxApp
{
public:
	CFileZillaApp();
	virtual ~CFileZillaApp();

	virtual bool OnInit();
	
	wxString GetResourceDir() const { return m_resourceDir; }
	wxString GetSettingsDir() const { return m_settingsDir; }
	wxString GetLocalesDir() const { return m_localesDir; }

	bool SetLocale(int language);
	int GetCurrentLanguage() const;

protected:
	bool InitSettingsDir();
	bool LoadResourceFiles();
	bool LoadLocales();

	wxLocale* m_pLocale;

	wxString m_resourceDir;
	wxString m_settingsDir;
	wxString m_localesDir;
};

DECLARE_APP(CFileZillaApp)

#endif //__FILEZILLAAPP_H__
