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

	wxRadioButton* m_pAction;

protected:
	CFileExistsNotification *m_pNotification;
};

#endif //__FILEEXISTSDLG_H__
