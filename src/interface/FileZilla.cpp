#include "FileZilla.h"
#include "Mainfrm.h"

#ifdef __WXMSW__
	#include <shlobj.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CFileZillaApp : public wxApp
{
public:
	virtual bool OnInit();
protected:
	bool LoadResourceFiles();

	wxLocale m_locale;
};

IMPLEMENT_APP(CFileZillaApp)

wxString resourcePath;
wxString dataPath; // Settings and such things

bool CFileZillaApp::OnInit()
{
	wxSystemOptions::SetOption(wxT("msw.remap"), 0);

#ifdef __WXMSW__
	wxChar buffer[MAX_PATH * 2 + 1];
	wxFileName fn;

	if (SUCCEEDED(SHGetFolderPath(0, CSIDL_APPDATA, 0, SHGFP_TYPE_CURRENT, buffer)))
	{
		fn = wxFileName(buffer, _T(""));
		fn.AppendDir(_T("FileZilla"));
		if (!fn.DirExists())
			wxMkdir(fn.GetPath(), 700);
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
		wxMkdir(fn.GetPath(), 700);
#endif
	dataPath = fn.GetPath();

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

	wxFrame *frame = new CMainFrame();
	SetTopWindow(frame);
	frame->Show(true);

	return true;
}

bool CFileZillaApp::LoadResourceFiles()
{
	wxPathList pathList;
	// FIXME: --datadir cmdline
	
	pathList.AddEnvList(_T("FZ_DATADIR"));
	pathList.Add(wxGetCwd());
	pathList.AddEnvList(_T("PATH"));
#ifdef __WXMSW_
#else
	pathList.Add(_T("/usr/share/filezilla"));
	pathList.Add(_T("/usr/local/share/filezilla"));
#endif

	wxString wxPath;

	wxPathList::const_iterator node;
	for (node = pathList.begin(); node != pathList.end() && wxPath == _T(""); node++)
	{
		wxString cur = *node;
		if (wxFileExists(cur + _T("/resources/menus.xrc")))
			wxPath = cur;
		else if (wxFileExists(cur + _T("/share/filezilla/resources/menus.xrc")))
			wxPath = cur + _T("/share/filezilla");
		else if (wxFileExists(cur + _T("/filezilla/resources/menus.xrc")))
			wxPath = cur + _T("/filezilla");
		else if (wxFileExists(cur + _T("/FileZilla.app/Contents/MacOS/resources/menus.xrc")))
			wxPath = cur + _T("/FileZilla.app/Contents/MacOS");
	}
	
	for (node = pathList.begin(); node != pathList.end() && wxPath == _T(""); node++)
	{
		wxString cur = *node;
		if (wxFileExists(cur + _T("/../resources/menus.xrc")))
			wxPath = cur + _T("/..");
		else if (wxFileExists(cur + _T("/../share/filezilla/resources/menus.xrc")))
			wxPath = cur + _T("/../share/filezilla");
	}

	wxImage::AddHandler(new wxPNGHandler());
	wxImage::AddHandler(new wxXPMHandler());
	wxLocale::AddCatalogLookupPathPrefix(wxPath + _T("/locales"));
	m_locale.Init(wxLANGUAGE_DEFAULT);
	m_locale.AddCatalog(_T("filezilla"));
	
	if (wxPath == _T(""))
	{
		wxString msg = _("Could not find the resource files for FileZilla, closing FileZilla.\nYou can set the data directory of FileZilla using the '--datadir <custompath>' commandline option or by setting the FZ_DATADIR environment variable.");
		wxMessageBox(msg, _("FileZilla Error"), wxOK | wxICON_ERROR);
		return false;
	}
	
	wxXmlResource::Get()->InitAllHandlers();
	wxXmlResource::Get()->Load(wxPath + _T("/resources/*.xrc"));

	resourcePath = wxPath;
	if (resourcePath[resourcePath.Length() - 1] != wxFileName::GetPathSeparator())
		resourcePath += wxFileName::GetPathSeparator();
	resourcePath += _T("resources");
	resourcePath += wxFileName::GetPathSeparator();

	return true;
}
