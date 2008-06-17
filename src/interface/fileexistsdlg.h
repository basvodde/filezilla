#ifndef __FILEEXISTSDLG_H__
#define __FILEEXISTSDLG_H__

#include "dialogex.h"

class CFileExistsDlg: public wxDialogEx
{	
	DECLARE_EVENT_TABLE()

public:
	/// Constructors
	CFileExistsDlg(CFileExistsNotification *pNotification);

	/// Creation
	bool Create(wxWindow* parent);

	/// Creates the controls and sizers
	void CreateControls();
	
	enum CFileExistsNotification::OverwriteAction GetAction() const;
	bool Always(bool &directionOnly, bool &queueOnly) const;

protected:
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnCheck(wxCommandEvent& event);
	
	void LoadIcon(int id, const wxString &file);
	wxString GetPathEllipsis(wxString path, wxWindow *window);

	CFileExistsNotification *m_pNotification;
	wxRadioButton *m_pAction1, *m_pAction2, *m_pAction3, *m_pAction4, *m_pAction5;
	enum CFileExistsNotification::OverwriteAction m_action;
	bool m_always;
	bool m_directionOnly;
	bool m_queueOnly;
};

#endif //__FILEEXISTSDLG_H__
