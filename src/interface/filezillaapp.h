#ifndef __FILEZILLAAPP_H__
#define __FILEZILLAAPP_H__

#if wxUSE_DEBUGREPORT && wxUSE_ON_FATAL_EXCEPTION
#include <wx/debugrpt.h>
#endif

class CWrapEngine;
class CCommandLine;
class CFileZillaApp : public wxApp
{
public:
	CFileZillaApp();
	virtual ~CFileZillaApp();

	virtual bool OnInit();

	wxString GetResourceDir() const { return m_resourceDir; }
	wxString GetSettingsDir() const { return m_settingsDir; }
	wxString GetDefaultsDir() const { return m_defaultsDir; }
	wxString GetLocalesDir() const { return m_localesDir; }

	void CheckExistsFzsftp();

	bool SetLocale(int language);
	int GetCurrentLanguage() const;
	wxString GetCurrentLanguageCode() const;

	void DisplayEncodingWarning();

	CWrapEngine* GetWrapEngine();

	const CCommandLine* GetCommandLine() const { return m_pCommandLine; }

protected:
	bool InitDefaultsDir();
	bool InitSettingsDir();
	wxString GetSettingsDirFromDefaults();
	bool LoadResourceFiles();
	bool LoadLocales();

	wxLocale* m_pLocale;

	wxString m_resourceDir;
	wxString m_defaultsDir;
	wxString m_settingsDir;
	wxString m_localesDir;

#if wxUSE_DEBUGREPORT && wxUSE_ON_FATAL_EXCEPTION
	virtual void OnFatalException();
	void GenerateReport(wxDebugReport::Context ctx);
#endif

	wxString GetDataDir(wxString fileToFind) const;

	// FileExists acceppts full paths as parameter,
	// with the addition that path segments may be obmitted
	// with a wildcard (*). A matching directory will then be searched.
	// Example: FileExists(_T("/home/*/.filezilla/filezilla.xml"));
	bool FileExists(const wxString& file) const;

	CWrapEngine* m_pWrapEngine;

	CCommandLine* m_pCommandLine;
};

DECLARE_APP(CFileZillaApp)

#endif //__FILEZILLAAPP_H__
