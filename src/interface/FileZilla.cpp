#include "FileZilla.h"
#include "filezillaapp.h"
#include "Mainfrm.h"

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

bool CFileZillaApp::OnInit()
{
	wxSystemOptions::SetOption(wxT("msw.remap"), 0);

	InitSettingsDir();

#ifndef _DEBUG
	wxMessageBox(_T("This software is still alpha software in early development, don't expect anything to work\r\n\
DO NOT post bugreports,\r\n\
DO NOT use it in production environments,\r\n\
DO NOT distrubute it,\r\n\
DO NOT complain about it\r\n\
USE AT OWN RISK"), _T("Important Information"));
#endif

	if (!LoadResourceFiles())
		return false;

	// Turn off idle events, we don't need them
	wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);

	wxFrame *frame = new CMainFrame();
	SetTopWindow(frame);
	frame->Show(true);

	return true;
}

wxString GetDataDir(wxString fileToFind)
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
		if (wxFileExists(cur + fileToFind))
			return cur;
		if (wxFileExists(cur + _T("/share/filezilla") + fileToFind))
			return cur + _T("/share/filezilla");
		if (wxFileExists(cur + _T("/filezilla") + fileToFind))
			return cur + _T("/filezilla");
#if defined(__WXMAC__) || defined(__WXCOCOA__)
		if (wxFileExists(cur + _T("/FileZilla.app/Contents/SharedSupport") + fileToFind))
			return cur + _T("/FileZilla.app/Contents/SharedSupport");
#endif
	}
	
	for (node = pathList.begin(); node != pathList.end(); node++)
	{
		wxString cur = *node;
		if (wxFileExists(cur + _T("/..") + fileToFind))
			return cur + _T("/..");
		if (wxFileExists(cur + _T("/../share/filezilla") + fileToFind))
			return cur + _T("/../share/filezilla");
	}
	
	return _T("");
}

bool CFileZillaApp::LoadResourceFiles()
{
	m_resourceDir = GetDataDir(_T("/resources/menus.xrc"));

	wxImage::AddHandler(new wxPNGHandler());
	wxImage::AddHandler(new wxXPMHandler());
	wxLocale::AddCatalogLookupPathPrefix(m_resourceDir + _T("/locales"));
	m_locale.Init(wxLANGUAGE_DEFAULT);
	m_locale.AddCatalog(_T("filezilla"));

	if (m_resourceDir == _T(""))
	{
		wxString msg = _("Could not find the resource files for FileZilla, closing FileZilla.\nYou can set the data directory of FileZilla using the '--datadir <custompath>' commandline option or by setting the FZ_DATADIR environment variable.");
		wxMessageBox(msg, _("FileZilla Error"), wxOK | wxICON_ERROR);
		return false;
	}

	if (m_resourceDir[m_resourceDir.Length() - 1] != wxFileName::GetPathSeparator())
		m_resourceDir += wxFileName::GetPathSeparator();

	m_resourceDir += "/resources/";
	
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
