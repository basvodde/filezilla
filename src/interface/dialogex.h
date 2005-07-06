#ifndef __DIALOGEX_H__
#define __DIALOGEX_H__

class wxDialogEx : public wxDialog
{
public:
	bool Load(wxWindow *pParent, const wxString& name);

	static wxString WrapText(const wxString &text, unsigned long maxLength, wxWindow* pWindow);
	bool WrapText(int id, unsigned long maxLength);
	bool SetLabel(int id, const wxString& label, unsigned long maxLength = 0);
	wxString GetLabel(int id);

protected:

	DECLARE_EVENT_TABLE();
	virtual void OnChar(wxKeyEvent& event);
};

#endif //__DIALOGEX_H__
