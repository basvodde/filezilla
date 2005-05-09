#ifndef __ABOUTDIALOG_H__
#define __ABOUTDIALOG_H__

class CAboutDialog : public wxDialog
{
public:
	CAboutDialog() {}
	virtual ~CAboutDialog() { }

	bool Create(wxWindow* parent);

protected:
	wxString GetBuildDate() const;
	wxString GetCompiler() const;

	DECLARE_EVENT_TABLE();
	void OnOK(wxCommandEvent& event);
	void OnChar(wxKeyEvent& event);
};

#endif //__ABOUTDIALOG_H__
