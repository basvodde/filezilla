#ifndef __BOOKMARKS_DIALOG_H__
#define __BOOKMARKS_DIALOG_H__

#include "dialogex.h"

class CNewBookmarkDialog : public wxDialogEx
{
public:
	CNewBookmarkDialog(wxWindow* parent, wxString& site_path, const CServer* server);
	virtual ~CNewBookmarkDialog() { }

	virtual int ShowModal(const wxString &local_path, const CServerPath &remote_path);

protected:
	wxWindow* m_parent;
	wxString &m_site_path;
	const CServer* m_server;

	DECLARE_EVENT_TABLE()
	void OnOK(wxCommandEvent& event);
	void OnBrowse(wxCommandEvent& event);
};

#endif //__BOOKMARKS_DIALOG_H__
