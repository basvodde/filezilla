// Based upon include/wx/xrc/xh_menu.h from wxWidgets

#ifndef _WX_XH_MENU_EX_H_
#define _WX_XH_MENU_EX_H_

#include <wx/xrc/xmlres.h>
#include <wx/xrc/xh_menu.h>

class wxMenuBarXmlHandlerEx : public wxMenuBarXmlHandler
{
    DECLARE_DYNAMIC_CLASS(wxMenuBarXmlHandlerEx)

public:
    virtual wxObject *DoCreateResource();
};

#endif // _WX_XH_MENU_EX_H_
