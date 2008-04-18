#include "FileZilla.h"
#include "sftp_crypt_info_dlg.h"
#include "dialogex.h"

void CSftpEncryptioInfoDialog::ShowDialog(CSftpEncryptionNotification* pNotification)
{
	wxDialogEx* pDlg = new wxDialogEx;
	pDlg->Load(0, _T("ID_SFTP_ENCRYPTION"));

	SetLabel(pDlg, XRCID("ID_KEXALGO"), pNotification->kexAlgorithm);
	SetLabel(pDlg, XRCID("ID_KEXHASH"), pNotification->kexHash);
	SetLabel(pDlg, XRCID("ID_FINGERPRINT"), pNotification->hostKey);
	SetLabel(pDlg, XRCID("ID_C2S_CIPHER"), pNotification->cipherClientToServer);
	SetLabel(pDlg, XRCID("ID_C2S_MAC"), pNotification->macClientToServer);
	SetLabel(pDlg, XRCID("ID_S2C_CIPHER"), pNotification->cipherServerToClient);
	SetLabel(pDlg, XRCID("ID_S2C_MAC"), pNotification->macServerToClient);

	pDlg->GetSizer()->Fit(pDlg);
	pDlg->GetSizer()->SetSizeHints(pDlg);

	pDlg->ShowModal();

	delete pDlg;
}

void CSftpEncryptioInfoDialog::SetLabel(wxDialogEx* pDlg, int id, const wxString& text)
{
	if (text == _T(""))
		pDlg->SetLabel(id, _("unknown"));
	else
		pDlg->SetLabel(id, text);
}
