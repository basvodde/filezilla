#ifndef __MANUAL_TRANSFER_H__
#define __MANUAL_TRANSFER_H__

#include "dialogex.h"

class CState;
class CManualTransfer : public wxDialogEx
{
public:
	CManualTransfer();
	virtual ~CManualTransfer();

	void Show(wxWindow* pParent, CState* pState);

protected:
	void DisplayServer();
	bool UpdateServer();
	bool VerifyServer();

	void SetControlState();
	void SetAutoAsciiState();
	void SetServerState();

	bool m_local_file_exists;

	CServer *m_pServer;
	CServer* m_pLastSite;

	CState* m_pState;

	DECLARE_EVENT_TABLE()
	void OnLocalChanged(wxCommandEvent& event);
	void OnLocalBrowse(wxCommandEvent& event);
	void OnRemoteChanged(wxCommandEvent& event);
	void OnDirection(wxCommandEvent& event);
	void OnServerTypeChanged(wxCommandEvent& event);
	void OnOK(wxCommandEvent& event);
	void OnSelectSite(wxCommandEvent& event);
	void OnSelectedSite(wxCommandEvent& event);
	void OnLogontypeSelChanged(wxCommandEvent& event);
};

#endif //__MANUAL_TRANSFER_H__
