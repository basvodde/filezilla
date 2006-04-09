#ifndef __DIALOGEX_H__
#define __DIALOGEX_H__

#include "wrapengine.h"

class wxDialogEx : public wxDialog, public CWrapEngine
{
public:
	bool Load(wxWindow *pParent, const wxString& name);

	bool SetLabel(int id, const wxString& label, unsigned long maxLength = 0);
	wxString GetLabel(int id);

	virtual int ShowModal();

protected:

	DECLARE_EVENT_TABLE();
	virtual void OnChar(wxKeyEvent& event);
};

#endif //__DIALOGEX_H__
