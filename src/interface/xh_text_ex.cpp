#include <filezilla.h>

#include "textctrlex.h"
#include "xh_text_ex.h"

#ifdef __WXMAC__

IMPLEMENT_DYNAMIC_CLASS(wxTextCtrlXmlHandlerEx, wxTextCtrlXmlHandler);

wxObject *wxTextCtrlXmlHandlerEx::DoCreateResource()
{
	XRC_MAKE_INSTANCE(text, wxTextCtrlEx);

	text->Create(m_parentAsWindow,
	             GetID(),
	             GetText(wxT("value")),
	             GetPosition(), GetSize(),
	             GetStyle(),
	             wxDefaultValidator,
	             GetName());

	SetupWindow(text);

	if (HasParam(wxT("maxlength")))
		text->SetMaxLength(GetLong(wxT("maxlength")));

	return text;
}

#endif
