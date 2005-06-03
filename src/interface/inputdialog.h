#ifndef __INPUTDIALOG_H__
#define __INPUTDIALOG_H__

#include "dialogex.h"

class CInputDialog : public wxDialogEx
{
public:
	CInputDialog() {}
	virtual ~CInputDialog() { }

	bool Create(wxWindow* parent, const wxString& title, const wxString& text);

	bool SetPasswordMode(bool password);
	void AllowEmpty(bool allowEmpty) { m_allowEmpty = allowEmpty; }

	void SetValue(const wxString& value);
	wxString GetValue() const;

	bool SelectText(int start, int end);

protected:
	bool m_allowEmpty;

	DECLARE_EVENT_TABLE();
	void OnValueChanged(wxCommandEvent& event);
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
};

#endif //__INPUTDIALOG_H__
