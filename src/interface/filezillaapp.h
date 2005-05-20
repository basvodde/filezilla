#ifndef __FILEZILLAAPP_H__
#define __FILEZILLAAPP_H__

#ifdef _MSC_VER
#include <msvc/wx/setup.h>
#endif

#if wxUSE_DEBUGREPORT && wxUSE_ON_FATAL_EXCEPTION
#include <wx/debugrpt.h>
#endif

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
};

DECLARE_APP(CFileZillaApp)

#endif //__FILEZILLAAPP_H__
