#ifndef __VERIFYCERTDIALOG_H__
#define __VERIFYCERTDIALOG_H__

#include "xmlfunctions.h"

class wxDialogEx;
class CVerifyCertDialog : protected wxEvtHandler
{
public:
	virtual ~CVerifyCertDialog();

	bool IsTrusted(CCertificateNotification* pNotification);
	void ShowVerificationDialog(CCertificateNotification* pNotification, bool displayOnly = false);

protected:
	bool IsTrusted(const unsigned char* data, unsigned int len, bool permanentOnly);

	bool DisplayCert(wxDialogEx* pDlg, const CCertificate& cert);

	void ParseDN(wxDialog* pDlg, const wxString& dn, wxSizer* pSizer);
	void ParseDN_by_prefix(wxDialog* pDlg, std::list<wxString>& tokens, wxString prefix, const wxString& name, wxSizer* pSizer, bool decode = false);
	
	wxString DecodeValue(const wxString& value);

	wxString ConvertHexToString(const unsigned char* data, unsigned int len);
	unsigned char* ConvertStringToHex(const wxString& str, unsigned int &len);

	void SetPermanentlyTrusted(const CCertificateNotification* const pNotification);

	void LoadTrustedCerts(bool close = true);

	struct t_certData
	{
		unsigned char* data;
		unsigned int len;
	};
	std::list<t_certData> m_trustedCerts;
	std::list<t_certData> m_sessionTrustedCerts;

	CXmlFile m_xmlFile;

	std::vector<CCertificate> m_certificates;
	wxDialogEx* m_pDlg;
	wxSizer* m_pSubjectSizer;
	wxSizer* m_pIssuerSizer;

	void OnCertificateChoice(wxCommandEvent& event);
};

#endif //__VERIFYCERTDIALOG_H__
