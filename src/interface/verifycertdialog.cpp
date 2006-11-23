#include "FileZilla.h"
#include "verifycertdialog.h"
#include <wx/tokenzr.h>
#include "dialogex.h"

std::list<CVerifyCertDialog::t_certData> CVerifyCertDialog::m_trustedCerts;

void CVerifyCertDialog::ShowVerificationDialog(CCertificateNotification* pNotification)
{
	wxDialogEx* pDlg = new wxDialogEx;
	pDlg->Load(0, _T("ID_VERIFYCERT"));

	pDlg->WrapText(pDlg, XRCID("ID_DESC"), 400);

	pDlg->SetLabel(XRCID("ID_HOST"), wxString::Format(_T("%s:%d"), pNotification->GetHost().c_str(), pNotification->GetPort()));

	if (pNotification->GetActivationTime().IsValid())
		pDlg->SetLabel(XRCID("ID_ACTIVATION_TIME"), pNotification->GetActivationTime().FormatDate());
	else
		pDlg->SetLabel(XRCID("ID_ACTIVATION_TIME"), _("Invalid date"));

	if (pNotification->GetExpirationTime().IsValid())
		pDlg->SetLabel(XRCID("ID_EXPIRATION_TIME"), pNotification->GetExpirationTime().FormatDate());
	else
		pDlg->SetLabel(XRCID("ID_EXPIRATION_TIME"), _("Invalid date"));

	if (pNotification->GetSerial() != _T(""))
		pDlg->SetLabel(XRCID("ID_SERIAL"), pNotification->GetSerial());
	else
		pDlg->SetLabel(XRCID("ID_SERIAL"), _("None"));

	pDlg->SetLabel(XRCID("ID_PKALGO"), wxString::Format(_("%s with %d bits"), pNotification->GetPkAlgoName().c_str(), pNotification->GetPkAlgoBits()));

	pDlg->SetLabel(XRCID("ID_FINGERPRINT_MD5"), pNotification->GetFingerPrintMD5());
	pDlg->SetLabel(XRCID("ID_FINGERPRINT_SHA1"), pNotification->GetFingerPrintSHA1());

	wxSizer* pSizer = XRCCTRL(*pDlg, "ID_SUBJECT_DUMMY", wxStaticText)->GetContainingSizer();
	ParseDN(pDlg, pNotification->GetSubject(), pSizer);
	XRCCTRL(*pDlg, "ID_SUBJECT_DUMMY", wxStaticText)->Destroy();

	pSizer = XRCCTRL(*pDlg, "ID_ISSUER_DUMMY", wxStaticText)->GetContainingSizer();
	ParseDN(pDlg, pNotification->GetIssuer(), pSizer);
	XRCCTRL(*pDlg, "ID_ISSUER_DUMMY", wxStaticText)->Destroy();

	pDlg->GetSizer()->Fit(pDlg);
	pDlg->GetSizer()->SetSizeHints(pDlg);

	int res = pDlg->ShowModal();

	if (res == wxID_OK)
	{
		wxASSERT(!IsTrusted(pNotification));

		pNotification->m_trusted = true;

		t_certData cert;
		const unsigned char* data = pNotification->GetRawData(cert.len);
		cert.data = new unsigned char[cert.len];
		memcpy(cert.data, data, cert.len);
		m_trustedCerts.push_back(cert);
	}
	else
		pNotification->m_trusted = false;

	delete pDlg;
}

void CVerifyCertDialog::ParseDN(wxDialog* pDlg, const wxString& dn, wxSizer* pSizer)
{
	wxStringTokenizer tokens(dn, _T(","));

	std::list<wxString> tokenlist;
	while (tokens.HasMoreTokens())
		tokenlist.push_back(tokens.GetNextToken());

	ParseDN_by_prefix(pDlg, tokenlist, _T("CN"), _("Common name:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("O"), _("Organization:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("OU"), _("Unit:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("T"), _("Title:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("C"), _("Country:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("ST"), _("State:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("L"), _("Locality:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("STREET"), _("Street:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("EMAIL"), _("E-Mail:"), pSizer);

	if (!tokenlist.empty())
	{
		wxString value = tokenlist.front();
		for (std::list<wxString>::const_iterator iter = ++tokenlist.begin(); iter != tokenlist.end(); iter++)
			value += _T(",") + *iter;

		pSizer->Add(new wxStaticText(pDlg, wxID_ANY, _("Other:")));
		pSizer->Add(new wxStaticText(pDlg, wxID_ANY, value));
	}
}

void CVerifyCertDialog::ParseDN_by_prefix(wxDialog* pDlg, std::list<wxString>& tokens, wxString prefix, const wxString& name, wxSizer* pSizer)
{
	prefix += _T("=");
	const int len = prefix.Length();
	
	wxString value;

	std::list<wxString>::iterator iter = tokens.begin();
	while (iter != tokens.end())
	{
		if (iter->Left(len) != prefix)
		{
			iter++;
			continue;
		}

		if (value != _T(""))
			value += _T("\n");

		value += iter->Mid(len);

		std::list<wxString>::iterator remove = iter++;
		tokens.erase(remove);
	}

	if (value != _T(""))
	{
		pSizer->Add(new wxStaticText(pDlg, wxID_ANY, name));
		pSizer->Add(new wxStaticText(pDlg, wxID_ANY, value));
	}
}

bool CVerifyCertDialog::IsTrusted(CCertificateNotification* pNotification)
{
	unsigned int len;
	const unsigned char* data = pNotification->GetRawData(len);

	for (std::list<t_certData>::const_iterator iter = m_trustedCerts.begin(); iter != m_trustedCerts.end(); iter++)
	{
		if (iter->len != len)
			continue;
		
		if (!memcmp(iter->data, data, len))
			return true;
	}

	return false;
}
