// Based upon src/xrc/xh_menu.cpp from wxWidgets

#include <filezilla.h>
#include "xh_menu_ex.h"

IMPLEMENT_DYNAMIC_CLASS(wxMenuBarXmlHandlerEx, wxMenuBarXmlHandler)

wxObject *wxMenuBarXmlHandlerEx::DoCreateResource()
{
    wxMenuBar *menubar = 0;

	// Only difference from wx <=2.8.10, this one
	// has working subclassing
    int style = GetStyle();
    wxASSERT(!style || !m_instance);
    if (m_instance)
        menubar = wxDynamicCast(m_instance, wxMenuBar);
    if (!menubar)
        menubar = new wxMenuBar(style);
    CreateChildren(menubar);

    if (m_parentAsWindow)
    {
        wxFrame *parentFrame = wxDynamicCast(m_parent, wxFrame);
        if (parentFrame)
            parentFrame->SetMenuBar(menubar);
    }

    return menubar;
}
