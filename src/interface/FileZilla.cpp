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
	wxLocale m_locale;
};

IMPLEMENT_APP(CFileZillaApp)

bool CFileZillaApp::OnInit()
{
	wxImage::AddHandler(new wxPNGHandler());
	wxLocale::AddCatalogLookupPathPrefix(_T("../../locales"));
	m_locale.Init(wxLANGUAGE_GERMAN);
	m_locale.AddCatalog(_T("filezilla"));

	//TODO: If ! resources found...
	if (0)
	{
		wxString msg = wxString::Format(_("Could not load resource files for FileZilla from \"%s\", closing FileZilla"), "asdf");
		wxMessageBox(msg, _("FileZilla Error"), wxOK | wxICON_ERROR);
	}
	
	wxXmlResource::Get()->InitAllHandlers();
	wxXmlResource::Get()->Load("../interface/resources/*.xrc");
	wxFrame *frame = new CMainFrame();
	SetTopWindow(frame);
	frame->Show(true);

	return true;
}
