#include "FileZilla.h"
#include "Mainfrm.h"

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

bool CFileZillaApp::OnInit()
{
	wxSystemOptions::SetOption(wxT("msw.remap"), 0);

	if (!LoadResourceFiles())
		return false;

	wxFrame *frame = new CMainFrame();
	SetTopWindow(frame);
	frame->Show(true);

	return true;
}

wxString resourcePath;
bool CFileZillaApp::LoadResourceFiles()
{
	wxPathList pathList;
	// FIXME: --datadir cmdline
	
	pathList.AddEnvList(_T("FZ_DATADIR"));
	pathList.Add(wxGetCwd());
	pathList.AddEnvList(_T("PATH"));
#ifdef __WXMSW_
#else
	pathList.Add(_T("/usr/local/share/filezilla/"));
	pathList.Add(_T("/usr/share/filezilla/"));
#endif

	wxString wxPath;
	for (wxPathList::Node *node = pathList.GetFirst(); node; node = node->GetNext())
	{
		wxString cur = node->GetData();
		if (wxFileExists(cur + _T("/resources/menus.xrc")))
			wxPath = cur;
		else if (wxFileExists(cur + _T("/share/filezilla/resources/menus.xrc")))
			wxPath = cur + _T("/share/filezilla");
		else if (wxFileExists(cur + _T("/../resources/menus.xrc")))
			wxPath = cur + _T("/..");
		else if (wxFileExists(cur + _T("/../share/filezilla/resources/menus.xrc")))
			wxPath = cur + _T("/../share/filezilla");
		else if (wxFileExists(cur + _T("/filezilla/resources/menus.xrc")))
			wxPath = cur + _T("/filezilla");

		if (wxPath != _T(""))
			break;
	}
	
	wxImage::AddHandler(new wxPNGHandler());
	wxImage::AddHandler(new wxXPMHandler());
	wxLocale::AddCatalogLookupPathPrefix(wxPath + _T("/locales"));
	m_locale.Init(wxLANGUAGE_GERMAN);
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

	return true;
}
