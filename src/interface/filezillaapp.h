#ifndef __FILEZILLAAPP_H__
#define __FILEZILLAAPP_H__

class CFileZillaApp : public wxApp
{
public:
	virtual bool OnInit();
	
	wxString GetResourceDir() const { return m_resourceDir; }
	wxString GetSettingsDir() const { return m_settingsDir; }

protected:
	bool InitSettingsDir();
	bool LoadResourceFiles();

	wxLocale m_locale;

	wxString m_resourceDir;
	wxString m_settingsDir;
};

DECLARE_APP(CFileZillaApp)

#endif //__FILEZILLAAPP_H__
