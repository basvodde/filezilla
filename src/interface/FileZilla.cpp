#include "FileZilla.h"
#include "filezillaapp.h"
#include "Mainfrm.h"
#include "Options.h"

#ifdef ENABLE_BINRELOC
	#define BR_PTHREADS 0
	#include "prefix.h"
#endif

#ifdef __WXMSW__
	#include <shlobj.h>

	// Needed for MinGW:
	#ifndef SHGFP_TYPE_CURRENT
		#define SHGFP_TYPE_CURRENT 0
	#endif
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_APP(CFileZillaApp);

CFileZillaApp::CFileZillaApp()
{
	m_pLocale = 0;
}

CFileZillaApp::~CFileZillaApp()
{
	delete m_pLocale;
}

bool CFileZillaApp::OnInit()
{
#if wxUSE_DEBUGREPORT && wxUSE_ON_FATAL_EXCEPTION
	//wxHandleFatalExceptions();
#endif
	wxApp::OnInit();

	wxSystemOptions::SetOption(wxT("msw.remap"), 0);

	LoadLocales();
    InitSettingsDir();

	COptions* pOptions = new COptions;
	wxString language = pOptions->GetOption(OPTION_LANGUAGE);
	if (language != _T(""))
	{
		const wxLanguageInfo* pInfo = wxLocale::FindLanguageInfo(language);
		if (!pInfo || !SetLocale(pInfo->Language))
			wxMessageBox(wxString::Format(_("Failed to set language to %s, using default system language"), language.c_str()), _("Failed to change language"), wxICON_EXCLAMATION);
	}

#ifndef _DEBUG
	wxMessageBox(_T("This software is still alpha software in early development, don't expect anything to work\r\n\
DO NOT post bugreports,\r\n\
DO NOT use it in production environments,\r\n\
DO NOT distribute it,\r\n\
DO NOT complain about it\r\n\
USE AT OWN RISK"), _T("Important Information"));
#endif

	if (!LoadResourceFiles())
	{
		delete pOptions;
		return false;
	}

	// Turn off idle events, we don't need them
	wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);

	wxFrame *frame = new CMainFrame(pOptions);
	frame->Show(true);
	SetTopWindow(frame);

	return true;
}

bool CFileZillaApp::FileExists(const wxString& file) const
{
	int pos = file.Find('*');
	if (pos == -1)
		return wxFileExists(file);

	wxASSERT(pos > 0);
	wxASSERT(file[pos - 1] == '/');
	wxASSERT(file[pos + 1] == '/');

	wxLogNull nullLog;
	wxDir dir(file.Left(pos));
	if (!dir.IsOpened())
		return false;

	wxString subDir;
	bool found = dir.GetFirst(&subDir, _T(""), wxDIR_DIRS);
	while (found)
	{
		if (FileExists(file.Left(pos) + subDir + file.Mid(pos + 1)))
			return true;

		found = dir.GetNext(&subDir);
	}

	return false;
}

wxString CFileZillaApp::GetDataDir(wxString fileToFind) const
{
	/*
	 * Finding the resources in all cases is a difficult task,
	 * due to the huge variety of diffent systems and their filesystem
	 * structure.
	 * Basically we just check a couple of paths for presence of the resources,
	 * and hope we find them. If not, the user can still specify on the cmdline
	 * and using environment variables where the resources are.
	 */

	wxPathList pathList;
	// FIXME: --datadir cmdline
	
	// First try the user specified data dir.
	pathList.AddEnvList(_T("FZ_DATADIR"));
	
	// Next try the current path and the current executable path.
	// Without this, running development versions would be difficult.
	
	pathList.Add(wxGetCwd());
	
#ifdef ENABLE_BINRELOC
	const char* path = SELFPATH;
	if (path && *path)
	{
		wxString datadir(SELFPATH , *wxConvCurrent);
		wxFileName fn(datadir);
		datadir = fn.GetPath();
		if (datadir != _T(""))
			pathList.Add(datadir);

	}
	path = DATADIR;
	if (path && *path)
	{
		wxString datadir(DATADIR, *wxConvCurrent);
		if (datadir != _T(""))
			pathList.Add(datadir);
	}
#elif defined __WXMSW__
	wxChar path[1024];
	int res = GetModuleFileName(0, path, 1000);
	if (res > 0 && res < 1000)
	{
		wxFileName fn(path);
		pathList.Add(fn.GetPath());
	}
#endif

	// Now scan through the path
	pathList.AddEnvList(_T("PATH"));

#ifdef __WXMSW__
#else
	// Try some common paths
	pathList.Add(_T("/usr/share/filezilla"));
	pathList.Add(_T("/usr/local/share/filezilla"));
#endif

	// For each path, check for the resources
	wxPathList::const_iterator node;
	for (node = pathList.begin(); node != pathList.end(); node++)
	{
		wxString cur = *node;
		if (FileExists(cur + fileToFind))
			return cur;
		if (FileExists(cur + _T("/share/filezilla") + fileToFind))
			return cur + _T("/share/filezilla");
		if (FileExists(cur + _T("/filezilla") + fileToFind))
			return cur + _T("/filezilla");
#if defined(__WXMAC__) || defined(__WXCOCOA__)
		if (FileExists(cur + _T("/FileZilla.app/Contents/SharedSupport") + fileToFind))
			return cur + _T("/FileZilla.app/Contents/SharedSupport");
#endif
	}
	
	for (node = pathList.begin(); node != pathList.end(); node++)
	{
		wxString cur = *node;
		if (FileExists(cur + _T("/..") + fileToFind))
			return cur + _T("/..");
		if (FileExists(cur + _T("/../share/filezilla") + fileToFind))
			return cur + _T("/../share/filezilla");
	}
	
	return _T("");
}

bool CFileZillaApp::LoadResourceFiles()
{
	m_resourceDir = GetDataDir(_T("/resources/menus.xrc"));

	wxImage::AddHandler(new wxPNGHandler());
	wxImage::AddHandler(new wxXPMHandler());
	
	if (m_resourceDir == _T(""))
	{
		wxString msg = _("Could not find the resource files for FileZilla, closing FileZilla.\nYou can set the data directory of FileZilla using the '--datadir <custompath>' commandline option or by setting the FZ_DATADIR environment variable.");
		wxMessageBox(msg, _("FileZilla Error"), wxOK | wxICON_ERROR);
		return false;
	}

	if (m_resourceDir[m_resourceDir.Length() - 1] != wxFileName::GetPathSeparator())
		m_resourceDir += wxFileName::GetPathSeparator();

	m_resourceDir += _T("resources/");
	
	wxXmlResource::Get()->InitAllHandlers();
	wxXmlResource::Get()->Load(m_resourceDir + _T("*.xrc"));

	return true;
}

bool CFileZillaApp::InitSettingsDir()
{
#ifdef __WXMSW__
	wxChar buffer[MAX_PATH * 2 + 1];
	wxFileName fn;

	if (SUCCEEDED(SHGetFolderPath(0, CSIDL_APPDATA, 0, SHGFP_TYPE_CURRENT, buffer)))
	{
		fn = wxFileName(buffer, _T(""));
		fn.AppendDir(_T("FileZilla"));
		if (!fn.DirExists())
			wxMkdir(fn.GetPath(), 0700);
	}
	else
	{
		// Fall back to directory where the executable is
		if (GetModuleFileName(0, buffer, MAX_PATH * 2))
			fn = buffer;
	}
#else
	wxFileName fn = wxFileName(wxGetHomeDir(), _T(""));
	fn.AppendDir(_T(".filezilla"));
	if (!fn.DirExists())
		wxMkdir(fn.GetPath(), 0700);
#endif
	m_settingsDir = fn.GetPath();

	return true;
}

bool CFileZillaApp::LoadLocales()
{
	m_localesDir = GetDataDir(_T("/locales/*/filezilla.mo"));

	if (!m_localesDir.Length() || m_localesDir[m_localesDir.Length() - 1] != wxFileName::GetPathSeparator())
		m_localesDir += wxFileName::GetPathSeparator();

	m_localesDir += _T("locales/");

	wxLocale::AddCatalogLookupPathPrefix(m_localesDir);

	SetLocale(wxLANGUAGE_DEFAULT);
	
	return true;
}

bool CFileZillaApp::SetLocale(int language)
{
	// First check if we can load the new locale
	wxLocale* pLocale = new wxLocale();
	wxLogNull log;
	pLocale->Init(language);
	if (!pLocale->IsOk() || !pLocale->AddCatalog(_T("filezilla")))
	{
		delete pLocale;
		return false;
	}

	// Now unload old locale
	// We unload new locale as well, else the internal locale chain in wxWidgets get's broken.
	delete pLocale;
	delete m_pLocale;
	m_pLocale = 0;

	// Finally load new one
	pLocale = new wxLocale();
	pLocale->Init(language);
	if (!pLocale->IsOk() || !pLocale->AddCatalog(_T("filezilla")))
	{
		delete pLocale;
		return false;
	}
	m_pLocale = pLocale;

	return true;
}

int CFileZillaApp::GetCurrentLanguage() const
{
	if (!m_pLocale)
		return wxLANGUAGE_ENGLISH;

	return m_pLocale->GetLanguage();
}


#if wxUSE_DEBUGREPORT && wxUSE_ON_FATAL_EXCEPTION

void CFileZillaApp::OnFatalException()
{
	GenerateReport(wxDebugReport::Context_Exception);
}

void CFileZillaApp::GenerateReport(wxDebugReport::Context ctx)
{
	wxDebugReport report;

	// add all standard files: currently this means just a minidump and an
	// XML file with system info and stack trace
	report.AddAll(ctx);

	// calling Show() is not mandatory, but is more polite
	if ( wxDebugReportPreviewStd().Show(report) )
	{
		if ( report.Process() )
		{
			// report successfully uploaded
		}
	}
	//else: user cancelled the report
}

#endif
