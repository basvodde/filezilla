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
	pathList.Add(_T("/usr/local/share/filezilla/"));
	pathList.Add(_T("/usr/share/filezilla/"));
#endif

	wxString wxPath = "";
	for (wxPathList::Node *node = pathList.GetFirst(); node; node = node->GetNext())
	{
		wxString cur = node->GetData();
		if (wxFileExists(cur + "/resources/menus.xrc"))
			wxPath = cur;
		else if (wxFileExists(cur + "/share/filezilla/resources/menus.xrc"))
			wxPath = cur + "/share/filezilla";
		else if (wxFileExists(cur + "/../resources/menus.xrc"))
			wxPath = cur + "/..";
		else if (wxFileExists(cur + "/../share/filezilla/resources/menus.xrc"))
			wxPath = cur + "/../share/filezilla";
		else if (wxFileExists(cur + "/filezilla/resources/menus.xrc"))
			wxPath = cur + "/filezilla";

		if (wxPath != "")
			break;
	}
	
	wxImage::AddHandler(new wxPNGHandler());
	wxLocale::AddCatalogLookupPathPrefix(wxPath + _T("/locales"));
	m_locale.Init(wxLANGUAGE_GERMAN);
	m_locale.AddCatalog(_T("filezilla"));
	
	if (wxPath == "")
	{
		wxString msg = _("Could not find the resource files for FileZilla, closing FileZilla.\nYou can set the data directory of FileZilla using the '--datadir <custompath>' commandline option or by setting the FZ_DATADIR environment variable.");
		wxMessageBox(msg, _("FileZilla Error"), wxOK | wxICON_ERROR);
		return false;
	}
	
	wxXmlResource::Get()->InitAllHandlers();
	wxXmlResource::Get()->Load(wxPath + "/resources/*.xrc");

	return true;
}
