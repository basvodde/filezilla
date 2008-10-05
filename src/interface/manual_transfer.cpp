#include "FileZilla.h"
#include "manual_transfer.h"

void CManualTransfer::Show(wxWindow* pParent, const wxString& localPath, const CServerPath& remotePath, const CServer* pServer)
{
	if (!Load(pParent, _T("ID_MANUALTRANSFER")))
		return;

	ShowModal();
}
