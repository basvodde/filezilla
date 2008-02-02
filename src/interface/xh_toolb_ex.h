// Based upon include/wx/xrc/xh_toolb.h from wxWidgets

#ifndef _WX_XH_TOOLB_EX_H_
#define _WX_XH_TOOLB_EX_H_

#include <wx/xrc/xmlres.h>

class wxToolBar;

class WXDLLIMPEXP_XRC wxToolBarXmlHandlerEx : public wxXmlResourceHandler
{
public:
    wxToolBarXmlHandlerEx();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

	static void SetIconSize(const wxSize& size) { m_iconSize = size; }

private:
    bool m_isInside;
    wxToolBar *m_toolbar;

	static wxSize m_iconSize;
};

#endif //_WX_XH_TOOLB_EX_H_
