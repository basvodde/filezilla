#include <filezilla.h>
#include "cursor_resetter.h"

#ifdef __WXGTK__
void ResetCursor(wxWindow* wnd)
{
	wxASSERT(wnd);

	wnd->wxWindowBase::SetCursor(wxNullCursor);

	wxWindowList children = wnd->GetChildren();
	for (wxWindowList::const_iterator iter = children.begin(); iter != children.end(); ++iter)
		ResetCursor(*iter);
}
#endif
