#ifndef __FILEEXISTSDLG_H__
#define __FILEEXISTSDLG_H__

class CFileExistsDlg: public wxDialog
{	
	DECLARE_EVENT_TABLE()

public:
	/// Constructors
	CFileExistsDlg(CFileExistsNotification *pNotification);

	/// Creation
	bool Create(wxWindow* parent);

	/// Creates the controls and sizers
	void CreateControls();
	
	int GetAction() const;

protected:
	virtual void OnOK(wxCommandEvent& event);
	virtual void OnCancel(wxCommandEvent& event);
	
	void LoadIcon(int id, const wxString &file);

	CFileExistsNotification *m_pNotification;
	wxRadioButton *m_pAction1, *m_pAction2, *m_pAction3, *m_pAction4, *m_pAction5;
	int m_action;
};

#endif //__FILEEXISTSDLG_H__
