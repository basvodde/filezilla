#include "filezilla.h"
#include "Mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CFileZillaApp : public wxApp
{
public:
	virtual bool OnInit();
};

IMPLEMENT_APP(CFileZillaApp)

bool CFileZillaApp::OnInit()
{
	//TODO: If ! ressources found...
	if (0)
	{
		wxString msg = wxString::Format(_("Could not load ressource files for FileZilla from \"%s\", closing FileZilla"), "asdf");
		wxMessageBox(msg, _("FileZilla Error"), wxOK | wxICON_ERROR);
	}
	
	wxXmlResource::Get()->InitAllHandlers();
	wxXmlResource::Get()->Load("../interface/ressources/*.xrc");

	wxFrame *frame = new CMainFrame();
	SetTopWindow(frame);
	frame->Show(true);

	return true;
}
