#ifndef __CHMODDIALOG_H__
#define __CHMODDIALOG_H__

#include "dialogex.h"

class CChmodDialog : public wxDialogEx
{
public:
	CChmodDialog() {}
	virtual ~CChmodDialog() { }

	bool Create(wxWindow* parent, int fileCount, int dirCount, 
				const wxString& name, const char permissions[9]);

	wxString GetPermissions(const char* previousPermissions);

	bool Recursive() const ;
	int GetApplyType() const { return m_applyType; }

protected:

	DECLARE_EVENT_TABLE();
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnRecurseChanged(wxCommandEvent& event);

	wxCheckBox* m_checkBoxes[9];
	char m_permissions[9];

	void OnCheckboxClick(wxCommandEvent& event);
	void OnNumericChanged(wxCommandEvent& event);

	bool m_noUserTextChange;
	wxString oldNumeric;
	bool lastChangedNumeric;

	bool m_recursive;
	int m_applyType;
};

#endif //__CHMODDIALOG_H__
