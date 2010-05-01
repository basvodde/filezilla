#ifndef __CURSOR_RESETTER_H__
#define __CURSOR_RESETTER_H__

// See comment about mouse cursor handling in wx' src/gtk/window.cpp
// It is painfully slow, wasting gigantic amounts of CPU cycles.
// Manually reset everything to use wxNullCursor keeps things fast.

#ifdef __WXGTK__
void ResetCursor(wxWindow* wnd);
#endif __WXGTK__

#endif //__CURSOR_RESETTER_H__
