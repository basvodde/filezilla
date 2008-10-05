#ifndef __MANUAL_TRANSFER_H__
#define __MANUAL_TRANSFER_H__

#include "dialogex.h"

class CManualTransfer : public wxDialogEx
{
public:
	CManualTransfer() { }

	void Show(wxWindow* pParent, const wxString& localPath, const CServerPath& remotePath, const CServer* pServer);
};

#endif //__MANUAL_TRANSFER_H__