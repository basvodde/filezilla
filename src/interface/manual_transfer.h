#ifndef __MANUAL_TRANSFER_H__
#define __MANUAL_TRANSFER_H__

#include "dialogex.h"

class CManualTransfer : public wxDialogEx
{
public:
	CManualTransfer();

	void Show(wxWindow* pParent, wxString localPath, const CServerPath& remotePath, const CServer* pServer);

protected:
	void DisplayServer(const CServer& server);

	void SetControlState();
	void SetAutoAsciiState();

	bool m_local_file_exists;

	CServer m_server;

	DECLARE_EVENT_TABLE()
	void OnLocalChanged(wxCommandEvent& event);
	void OnLocalBrowse(wxCommandEvent& event);
	void OnRemoteChanged(wxCommandEvent& event);
	void OnDirection(wxCommandEvent& event);
};

#endif //__MANUAL_TRANSFER_H__