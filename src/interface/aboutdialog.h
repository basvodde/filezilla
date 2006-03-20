#ifndef __ABOUTDIALOG_H__
#define __ABOUTDIALOG_H__

#include "dialogex.h"

class CAboutDialog : public wxDialogEx
{
public:
	CAboutDialog() {}
	virtual ~CAboutDialog() { }

	bool Create(wxWindow* parent);

	static wxString GetBuildDate();
	static wxString GetCompiler();

protected:

	DECLARE_EVENT_TABLE();
	void OnOK(wxCommandEvent& event);
};

#endif //__ABOUTDIALOG_H__
