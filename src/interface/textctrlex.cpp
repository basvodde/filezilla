#include <filezilla.h>

#include "textctrlex.h"

#ifdef __WXMAC__

static wxTextAttr DoGetDefaultStyle(wxTextCtrl* ctrl)
{
	wxTextAttr style;
	style.SetFont(ctrl->GetFont());
	return style;
}

const wxTextAttr& GetDefaultTextCtrlStyle(wxTextCtrl* ctrl)
{
	static const wxTextAttr style = DoGetDefaultStyle(ctrl);
	return style;
}

void wxTextCtrlEx::Paste()
{
	wxTextCtrl::Paste();
	SetStyle(0, GetLastPosition(), GetDefaultTextCtrlStyle(this));
}

#endif
