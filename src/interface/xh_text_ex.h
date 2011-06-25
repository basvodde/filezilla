#ifndef __XH_TEXT_EX_H__
#define __XH_TEXT_EX_H__

#include <wx/xrc/xh_text.h>

#ifndef __WXMAC__
#define wxTextCtrlXmlHandlerEx wxTextCtrlXmlHandler
#else

class wxTextCtrlXmlHandlerEx : public wxTextCtrlXmlHandler
{
	DECLARE_DYNAMIC_CLASS(wxTextCtrlXmlHandlerEx);

public:
	virtual wxObject *DoCreateResource();
};

#endif

#endif //__XH_TEXT_EX_H__
