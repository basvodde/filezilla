#include "filezilla.h"
#include "verifyhostkeydialog.h"
#include <wx/tokenzr.h>
#include "dialogex.h"
#include "ipcmutex.h"

std::list<CVerifyHostkeyDialog::t_keyData> CVerifyHostkeyDialog::m_sessionTrustedKeys;

void CVerifyHostkeyDialog::ShowVerificationDialog(wxWindow* parent, CHostKeyNotification* pNotification)
{
	wxDialogEx* pDlg = new wxDialogEx;
	bool loaded;
	if (pNotification->GetRequestID() == reqId_hostkey)
		loaded = pDlg->Load(parent, _T("ID_HOSTKEY"));
	else
		loaded = pDlg->Load(parent, _T("ID_HOSTKEYCHANGED"));
	if (!loaded)
	{
		pNotification->m_trust = false;
		pNotification->m_alwaysTrust = false;
		wxBell();
		return;
	}

	pDlg->WrapText(pDlg, XRCID("ID_DESC"), 400);

	const wxString host = wxString::Format(_T("%s:%d"), pNotification->GetHost().c_str(), pNotification->GetPort());
	pDlg->SetLabel(XRCID("ID_HOST"), host);
	pDlg->SetLabel(XRCID("ID_FINGERPRINT"), pNotification->GetFingerprint());

	pDlg->GetSizer()->Fit(pDlg);
	pDlg->GetSizer()->SetSizeHints(pDlg);

	int res = pDlg->ShowModal();

	if (res == wxID_OK)
	{
		pNotification->m_trust = true;
		pNotification->m_alwaysTrust = XRCCTRL(*pDlg, "ID_ALWAYS", wxCheckBox)->GetValue();

		struct t_keyData data;
		data.host = host;
		data.fingerprint = pNotification->GetFingerprint();
		m_sessionTrustedKeys.push_back(data);
		return;
	}

	pNotification->m_trust = false;
	pNotification->m_alwaysTrust = false;
}

bool CVerifyHostkeyDialog::IsTrusted(CHostKeyNotification* pNotification)
{
	const wxString host = wxString::Format(_T("%s:%d"), pNotification->GetHost().c_str(), pNotification->GetPort());

	for (std::list<struct t_keyData>::const_iterator iter = m_sessionTrustedKeys.begin(); iter != m_sessionTrustedKeys.end(); iter++)
	{
		if (iter->host == host && iter->fingerprint == pNotification->GetFingerprint())
			return true;
	}

	return false;
}
