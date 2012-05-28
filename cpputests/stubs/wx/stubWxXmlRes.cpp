
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "wx/xrc/xmlres.h"

IMPLEMENT_ABSTRACT_CLASS(wxXmlResourceHandler, wxObject);

wxXmlResourceHandler::wxXmlResourceHandler()
{
	FAIL("wxXmlResourceHandler::wxXmlResourceHandler");
}

wxColour wxXmlResourceHandler::GetColour(const wxString& param, const wxColour& defaultv)
{
	FAIL("wxXmlResourceHandler::GetColour");
	return wxColour();
}

wxString wxXmlResourceHandler::GetName()
{
	FAIL("wxXmlResourceHandler::GetName");
	return L"";
}

void wxXmlResourceHandler::AddWindowStyles()
{
	FAIL("wxXmlResourceHandler::AddWindowStyles");
}

void wxXmlResourceHandler::CreateChildren(wxObject *parent, bool this_hnd_only)
{
	FAIL("wxXmlResourceHandler::CreateChildren");
}

wxString wxXmlResourceHandler::GetText(const wxString& param, bool translate)
{
	FAIL("wxXmlResourceHandler::GetText");
	return L"";
}

void wxXmlResourceHandler::SetupWindow(wxWindow *wnd)
{
	FAIL("wxXmlResourceHandler::SetupWindow");
}

bool wxXmlResourceHandler::HasParam(const wxString& param)
{
	FAIL("wxXmlResourceHandler::HasParam");
	return true;
}

int wxXmlResourceHandler::GetID()
{
	FAIL("wxXmlResourceHandler::GetID");
	return 0;
}

wxBitmap wxXmlResourceHandler::GetBitmap(const wxString& param, const wxArtClient& defaultArtClient, wxSize size)
{
	FAIL("wxXmlResourceHandler::GetBitmap");
	return wxBitmap();
}

int wxXmlResourceHandler::GetStyle(const wxString& param, int defaults)
{
	FAIL("wxXmlResourceHandler::GetStyle");
	return 0;
}

wxPoint wxXmlResourceHandler::GetPosition(const wxString& param)
{
	FAIL("wxXmlResourceHandler::GetPosition");
	return wxPoint();
}

wxSize wxXmlResourceHandler::GetSize(const wxString& param, wxWindow *windowToUse)
{
	FAIL("wxXmlResourceHandler::GetSize");
	return wxSize();
}

long wxXmlResourceHandler::GetLong(const wxString& param, long defaultv)
{
	FAIL("wxXmlResourceHandler::GetLong");
	return 0;
}

wxXmlNode *wxXmlResourceHandler::GetParamNode(const wxString& param)
{
	FAIL("wxXmlResourceHandler::GetParamNode");
	return NULL;
}

void wxXmlResourceHandler::AddStyle(const wxString& name, int value)
{
	FAIL("wxXmlResourceHandler::AddStyle");
}

bool wxXmlResourceHandler::IsOfClass(wxXmlNode *node, const wxString& classname)
{
	FAIL("wxXmlResourceHandler::IsOfClass");
	return true;
}

bool wxXmlResourceHandler::GetBool(const wxString& param, bool defaultv)
{
	FAIL("wxXmlResourceHandler::GetBool");
	return true;
}



void wxXmlResource::AddHandler(wxXmlResourceHandler *handler)
{
	FAIL("wxXmlResource::AddHandler");
}

wxDialog *wxXmlResource::LoadDialog(wxWindow *parent, const wxString& name)
{
	FAIL("wxXmlResource::LoadDialog");
	return NULL;
}

wxXmlResource *wxXmlResource::Get()
{
	FAIL("wxXmlResource::Get");
	return NULL;
}

wxObject *wxXmlResource::CreateResFromNode(wxXmlNode *node, wxObject *parent, wxObject *instance, wxXmlResourceHandler *handlerToUse)
{
	FAIL("wxXmlResource::CreateResFromNode");
	return NULL;
}

wxMenuBar *wxXmlResource::LoadMenuBar(wxWindow *parent, const wxString& name)
{
	FAIL("wxXmlResource::LoadMenuBar");
	return NULL;
}

wxMenu *wxXmlResource::LoadMenu(const wxString& name)
{
	FAIL("wxXmlResource::LoadMenu");
	return NULL;
}

int wxXmlResource::GetXRCID(const wxChar *str_id, int value_if_not_found)
{
	return 0;
}

bool wxXmlResource::Load(const wxString& filemask)
{
	FAIL("wxXmlResource::Load");
	return true;
}

wxToolBar *wxXmlResource::LoadToolBar(wxWindow *parent, const wxString& name)
{
	FAIL("wxXmlResource::LoadToolBar");
	return NULL;
}


wxPanel *wxXmlResource::LoadPanel(wxWindow *parent, const wxString& name)
{
	FAIL("wxXmlResource::LoadPanel");
	return NULL;
}

bool wxXmlResource::LoadDialog(wxDialog *dlg, wxWindow *parent, const wxString& name)
{
	FAIL("wxXmlResource::LoadDialog");
	return true;
}

bool wxXmlResource::LoadPanel(wxPanel *panel, wxWindow *parent, const wxString& name)
{
	FAIL("wxXmlResource::LoadPanel");
	return true;
}


