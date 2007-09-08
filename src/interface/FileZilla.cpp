#include "FileZilla.h"
#include "filezillaapp.h"
#include "Mainfrm.h"
#include "Options.h"
#include "wrapengine.h"
#include "buildinfo.h"
#ifdef __WXMSW__
#include <wx/msw/registry.h> // Needed by CheckForWin2003FirewallBug
#endif
#include "xmlfunctions.h"
#include <wx/tokenzr.h>

#include <wx/xrc/xh_bmpbt.h>
#include <wx/xrc/xh_bttn.h>
#include <wx/xrc/xh_chckb.h>
#include <wx/xrc/xh_chckl.h>
#include <wx/xrc/xh_choic.h>
#include <wx/xrc/xh_dlg.h>
#include <wx/xrc/xh_gauge.h>
#include <wx/xrc/xh_listb.h>
#include <wx/xrc/xh_listc.h>
#include <wx/xrc/xh_menu.h>
#include <wx/xrc/xh_notbk.h>
#include <wx/xrc/xh_panel.h>
#include <wx/xrc/xh_radbt.h>
#include <wx/xrc/xh_scwin.h>
#include <wx/xrc/xh_sizer.h>
#include <wx/xrc/xh_spin.h>
#include <wx/xrc/xh_stbmp.h>
#include <wx/xrc/xh_stbox.h>
#include <wx/xrc/xh_stlin.h>
#include <wx/xrc/xh_sttxt.h>
#include <wx/xrc/xh_text.h>
#include <wx/xrc/xh_toolb.h>
#include <wx/xrc/xh_tree.h>
#include <wx/xrc/xh_hyperlink.h>

#if defined(__WXMAC__) || defined(__UNIX__)
#include <wx/stdpaths.h>
#endif

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
	m_pWrapEngine = 0;
	m_pLocale = 0;
}

CFileZillaApp::~CFileZillaApp()
{
	delete m_pLocale;
	delete m_pWrapEngine;
	COptions::Destroy();
}

#ifdef __WXMSW__
bool IsServiceRunning(const wxString& serviceName)
{
	SC_HANDLE hScm = OpenSCManager(0, 0, GENERIC_READ);
	if (!hScm)
	{
		//wxMessageBox(_T("OpenSCManager failed"));
		return false;
	}

	SC_HANDLE hService = OpenService(hScm, serviceName, GENERIC_READ);
	if (!hService)
	{
		CloseServiceHandle(hScm);
		//wxMessageBox(_T("OpenService failed"));
		return false;
	}

	SERVICE_STATUS status;
	if (!ControlService(hService, SERVICE_CONTROL_INTERROGATE, &status))
	{
		CloseServiceHandle(hService);
		CloseServiceHandle(hScm);
		//wxMessageBox(_T("ControlService failed"));
		return false;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hScm);

	if (status.dwCurrentState == 0x07 || status.dwCurrentState == 0x01)
		return false;

	return true;
}

bool CheckForWin2003FirewallBug()
{
	const wxString os = ::wxGetOsDescription();
	if (os.Find(_T("Windows Server 2003")) == -1)
		return false;

	if (!IsServiceRunning(_T("SharedAccess")))
		return false;

	if (!IsServiceRunning(_T("ALG")))
		return false;

	wxRegKey key(_T("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\StandardProfile"));
	if (!key.Exists() || !key.Open(wxRegKey::Read))
		return false;

	long value;
	if (!key.HasValue(_T("EnableFirewall")) || !key.QueryValue(_T("EnableFirewall"), &value))
		return false;

	if (!value)
		return false;

	return true;
}
#endif //__WXMSW__

bool CFileZillaApp::OnInit()
{
#if wxUSE_DEBUGREPORT && wxUSE_ON_FATAL_EXCEPTION
	//wxHandleFatalExceptions();
#endif
	wxApp::OnInit();

	wxSystemOptions::SetOption(_T("msw.remap"), 0);
	wxSystemOptions::SetOption(_T("mac.listctrl.always_use_generic"), 1);

	LoadLocales();
	InitDefaultsDir();
    InitSettingsDir();

	COptions::Init();
	wxString language = COptions::Get()->GetOption(OPTION_LANGUAGE);
	if (language != _T(""))
	{
		const wxLanguageInfo* pInfo = wxLocale::FindLanguageInfo(language);
		if (!pInfo || !SetLocale(pInfo->Language))
			wxMessageBox(wxString::Format(_("Failed to set language to %s, using default system language"), language.c_str()), _("Failed to change language"), wxICON_EXCLAMATION);
	}

#ifndef _DEBUG
	const wxString& buildType = CBuildInfo::GetBuildType();
	if (buildType == _T("nightly"))
        wxMessageBox(_T("You are using a nightly development version of FileZilla 3, do not expect anything to work.\r\nPlease use the official releases instead.\r\n\r\n\
Unless explicitly instructed otherwise,\r\n\
DO NOT post bugreports,\r\n\
DO NOT use it in production environments,\r\n\
DO NOT distribute the binaries,\r\n\
DO NOT complain about it\r\n\
USE AT OWN RISK"), _T("Important Information"));
	else if (buildType == _T("official"))
	{
		wxString greetingVersion = COptions::Get()->GetOption(OPTION_GREETINGVERSION);
		if (greetingVersion == _T("") ||
			CBuildInfo::ConvertToVersionNumber(CBuildInfo::GetVersion()) > CBuildInfo::ConvertToVersionNumber(greetingVersion))
		{
			COptions::Get()->SetOption(OPTION_GREETINGVERSION, CBuildInfo::GetVersion());

			wxMessageBox(wxString::Format(_T("Welcome to FileZilla %s\r\n\r\n\
Please report all bugs you may encounter.\
"), CBuildInfo::GetVersion().c_str()), _T("Welcome"));
		}
	}
#endif

	if (!LoadResourceFiles())
	{
		COptions::Destroy();
		return false;
	}

	CheckExistsFzsftp();

#ifdef __WXMSW__
	if (CheckForWin2003FirewallBug())
	{
		const wxString& error = _("Warning!\n\nA bug in Windows causes problems with FileZilla\n\n\
The bug occurs if you have\n\
- Windows Server 2003 or XP 64\n\
- Windows Firewall enabled\n\
- Application Layer Gateway service enabled\n\
See http://support.microsoft.com/kb/931130 for background information.\n\n\
Unless you either disable Windows Firewall or the Application Layer Gateway service,\n\
FileZilla will timeout on big transfers.\
");
		wxMessageBox(error, _("Operating system problem detected"), wxICON_EXCLAMATION);
	}
#endif

	// Turn off idle events, we don't need them
	wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);

	wxUpdateUIEvent::SetMode(wxUPDATE_UI_PROCESS_SPECIFIED);

	// Load the text wrapping engine
	m_pWrapEngine = new CWrapEngine();
	m_pWrapEngine->LoadCache();

	wxFrame *frame = new CMainFrame();
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

	for (node = pathList.begin(); node != pathList.end(); node++)
	{
		wxString cur = *node;
		if (FileExists(cur + _T("/../../") + fileToFind))
			return cur + _T("/../..");
	}

	return _T("");
}

bool CFileZillaApp::LoadResourceFiles()
{
	m_resourceDir = GetDataDir(_T("/resources/menus.xrc"));

	wxImage::AddHandler(new wxPNGHandler());

	if (m_resourceDir == _T(""))
	{
		wxString msg = _("Could not find the resource files for FileZilla, closing FileZilla.\nYou can set the data directory of FileZilla using the '--datadir <custompath>' commandline option or by setting the FZ_DATADIR environment variable.");
		wxMessageBox(msg, _("FileZilla Error"), wxOK | wxICON_ERROR);
		return false;
	}

	if (m_resourceDir[m_resourceDir.Length() - 1] != wxFileName::GetPathSeparator())
		m_resourceDir += wxFileName::GetPathSeparator();

	m_resourceDir += _T("resources/");

	wxXmlResource *pResource = wxXmlResource::Get();

#ifndef __WXDEBUG__
	pResource->SetFlags(pResource->GetFlags() | wxXRC_NO_RELOADING);
#endif

    pResource->AddHandler(new wxMenuXmlHandler);
    pResource->AddHandler(new wxMenuBarXmlHandler);
	pResource->AddHandler(new wxDialogXmlHandler);
	pResource->AddHandler(new wxPanelXmlHandler);
	pResource->AddHandler(new wxSizerXmlHandler);
	pResource->AddHandler(new wxButtonXmlHandler);
	pResource->AddHandler(new wxBitmapButtonXmlHandler);
    pResource->AddHandler(new wxStaticTextXmlHandler);
    pResource->AddHandler(new wxStaticBoxXmlHandler);
    pResource->AddHandler(new wxStaticBitmapXmlHandler);
    pResource->AddHandler(new wxTreeCtrlXmlHandler);
    pResource->AddHandler(new wxListCtrlXmlHandler);
    pResource->AddHandler(new wxCheckListBoxXmlHandler);
    pResource->AddHandler(new wxChoiceXmlHandler);
    pResource->AddHandler(new wxGaugeXmlHandler);
    pResource->AddHandler(new wxCheckBoxXmlHandler);
    pResource->AddHandler(new wxSpinCtrlXmlHandler);
    pResource->AddHandler(new wxRadioButtonXmlHandler);
    pResource->AddHandler(new wxNotebookXmlHandler);
    pResource->AddHandler(new wxTextCtrlXmlHandler);
    pResource->AddHandler(new wxListBoxXmlHandler);
    pResource->AddHandler(new wxToolBarXmlHandler);
	pResource->AddHandler(new wxStaticLineXmlHandler);
    pResource->AddHandler(new wxScrolledWindowXmlHandler);
    pResource->AddHandler(new wxHyperlinkCtrlXmlHandler);

	wxString resourceDir = m_resourceDir;
#if wxUSE_FILESYSTEM
	resourceDir.Replace(_T("%"), _T("%25"));
	resourceDir.Replace(_T(":"), _T("%3A"));
	resourceDir.Replace(_T("#"), _T("%23"));
#endif
	pResource->Load(resourceDir + _T("*.xrc"));

	return true;
}

bool CFileZillaApp::InitDefaultsDir()
{
	m_defaultsDir = GetDataDir(_T("/fzdefaults.xml"));

	return m_defaultsDir != _T("");
}

wxString CFileZillaApp::GetSettingsDirFromDefaults()
{
	if (GetDefaultsDir() == _T(""))
		return _T("");

	wxFileName fn(GetDefaultsDir(), _T("fzdefaults.xml"));
	if (!fn.IsOk() || !fn.FileExists())
		return _T("");

	CXmlFile file(fn);
	TiXmlElement* element = file.Load();
	if (!element)
		return _T("");

	TiXmlElement* settings = element->FirstChildElement("Settings");
	if (!settings)
		return _T("");

	TiXmlElement* setting = FindElementWithAttribute(settings, "Setting", "name", "Config Location");
	if (!setting)
		return _T("");

	wxString location = GetTextElement(setting);
	if (location == _T(""))
		return _T("");

	wxStringTokenizer tokenizer(location, _T("/\\"), wxTOKEN_RET_EMPTY_ALL);
	location = _T("");
	while (tokenizer.HasMoreTokens())
	{
		wxString token = tokenizer.GetNextToken();
		if (token[0] == '$')
		{
			if (token[1] == '$')
				token = token.Mid(1);
			else
			{
				wxString value;
				if (wxGetEnv(token.Mid(1), &value))
					token = value;
			}
		}
		location += token;
		const wxChar delimiter = tokenizer.GetLastDelimiter();
		if (delimiter)
			location += delimiter;

	}

	wxFileName norm(location, _T(""));
	norm.Normalize(wxPATH_NORM_ALL, fn.GetPath());

	location = norm.GetFullPath();
	return location;
}

bool CFileZillaApp::InitSettingsDir()
{
	m_settingsDir = GetSettingsDirFromDefaults();

	wxFileName fn;
	if (m_settingsDir != _T(""))
		fn = wxFileName(m_settingsDir, _T(""));
	else
	{

#ifdef __WXMSW__
		wxChar buffer[MAX_PATH * 2 + 1];

		if (SUCCEEDED(SHGetFolderPath(0, CSIDL_APPDATA, 0, SHGFP_TYPE_CURRENT, buffer)))
		{
			fn = wxFileName(buffer, _T(""));
			fn.AppendDir(_T("FileZilla"));
		}
		else
		{
			// Fall back to directory where the executable is
			if (GetModuleFileName(0, buffer, MAX_PATH * 2))
				fn = buffer;
		}
#else
		fn = wxFileName(wxGetHomeDir(), _T(""));
		fn.AppendDir(_T(".filezilla"));
#endif
	}
	if (!fn.DirExists())
		wxMkdir(fn.GetPath(), 0700);
	m_settingsDir = fn.GetPath();

	return true;
}

bool CFileZillaApp::LoadLocales()
{
	m_localesDir = GetDataDir(_T("/../locale/*/filezilla.mo"));
	if (m_localesDir != _T(""))
	{
		if (m_localesDir[m_localesDir.Length() - 1] != wxFileName::GetPathSeparator())
			m_localesDir += wxFileName::GetPathSeparator();

		m_localesDir += _T("../locale");
	}
	else
	{
		m_localesDir = GetDataDir(_T("/locales/*/filezilla.mo"));
		if (m_localesDir != _T(""))
		{
			if (m_localesDir[m_localesDir.Length() - 1] != wxFileName::GetPathSeparator())
				m_localesDir += wxFileName::GetPathSeparator();

			m_localesDir += _T("locales");
		}
	}

	if (m_localesDir != _T(""))
	{
		wxFileName fn(m_localesDir, _T(""));
		fn.Normalize();
		m_localesDir = fn.GetFullPath();

		wxLocale::AddCatalogLookupPathPrefix(m_localesDir);
	}

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
	//GenerateReport(wxDebugReport::Context_Exception);
}

void CFileZillaApp::GenerateReport(wxDebugReport::Context ctx)
{
	/*
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
	}*/
	//else: user cancelled the report
}

#endif

void CFileZillaApp::DisplayEncodingWarning()
{
	static bool displayedEncodingWarning = false;
	if (displayedEncodingWarning)
		return;

	displayedEncodingWarning = true;

	wxMessageBox(_("A local filename could not be decoded.\nPlease make sure the LC_CTYPE (or LC_ALL) environment variable is set correctly.\nUnless you fix this problem, files might be missing in the file listings.\nNo further warning will be displayed this session."), _("Character encoding issue"), wxICON_EXCLAMATION);
}

CWrapEngine* CFileZillaApp::GetWrapEngine()
{
	return m_pWrapEngine;
}

void CFileZillaApp::CheckExistsFzsftp()
{
	// Get the correct path to the fzsftp executable

	wxString program = _T("fzsftp");
#ifdef __WXMSW__
	program += _T(".exe");
#endif

	bool found = false;

	// First check the FZ_FZSFTP environment variable
	wxString executable;
	if (wxGetEnv(_T("FZ_FZSFTP"), &executable))
	{
		if (wxFileName::FileExists(executable))
			found = true;
	}

	if (!found)
	{
		wxPathList pathList;

		// Add current working directory
		const wxString &cwd = wxGetCwd();
		pathList.Add(cwd);
#ifdef __WXMSW__

		// Add executable path
		wxChar modulePath[1000];
		DWORD len = GetModuleFileName(0, modulePath, 999);
		if (len)
		{
			modulePath[len] = 0;
			wxString path(modulePath);
			int pos = path.Find('\\', true);
			if (pos != -1)
			{
				path = path.Left(pos);
				pathList.Add(path);
			}
		}
#endif

		// Add a few paths relative to the current working directory
		pathList.Add(cwd + _T("/bin"));
		pathList.Add(cwd + _T("/src/putty"));
		pathList.Add(cwd + _T("/putty"));

		executable = pathList.FindAbsoluteValidPath(program);
		if (executable != _T(""))
			found = true;
	}

	if (!found)
	{
#ifdef __WXMAC__
		// Get application path within the bundle
		wxFileName fn(wxStandardPaths::Get().GetDataDir() + _T("/../MacOS/"), program);
		fn.Normalize();
		if (fn.FileExists())
		{
			executable = fn.GetFullPath();
			found = true;
		}
#elif defined(__UNIX__)
		const wxString prefix = ((const wxStandardPaths&)wxStandardPaths::Get()).GetInstallPrefix();
		if (prefix != _T("/usr/local"))
		{
			// /usr/local is the fallback value. /usr/local/bin is most likely in the PATH
			// environment variable already so we don't have to check it. Furthermore, other
			// directories might be listed before it (For example a developer's own
			// application prefix)
			wxFileName fn(prefix + _T("/bin/"), program);
			fn.Normalize();
			if (fn.FileExists())
			{
				executable = fn.GetFullPath();
				found = true;
			}
		}
#endif
	}

	if (!found)
	{
		// Check PATH
		wxPathList pathList;
		pathList.AddEnvList(_T("PATH"));
		executable = pathList.FindAbsoluteValidPath(program);
		if (executable != _T(""))
			found = true;
	}

	// Quote path if it contains spaces
	if (executable.Find(_T(" ")) != -1 && executable[0] != '"' && executable[0] != '\'')
		executable = _T("\"") + executable + _T("\"");

	if (!found)
	{
		wxMessageBox(wxString::Format(_("%s could not be found. Without this component of FileZilla, SFTP will not work.\n\nPossible solutions:\n- Make sure %s is in a directory listed in your PATH environment variable.\n- Set the full path to %s in the FZ_FZSFTP environment variable."), program.c_str(), program.c_str(), program.c_str()),
			_("File not found"), wxICON_ERROR);
	}
	else
		COptions::Get()->SetOption(OPTION_FZSFTP_EXECUTABLE, executable);
}
