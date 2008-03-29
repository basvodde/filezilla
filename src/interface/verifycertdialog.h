#ifndef __VERIFYCERTDIALOG_H__
#define __VERIFYCERTDIALOG_H__

#include "xmlfunctions.h"

class CVerifyCertDialog
{
public:
	virtual ~CVerifyCertDialog();

	bool IsTrusted(CCertificateNotification* pNotification);
	void ShowVerificationDialog(CCertificateNotification* pNotification, bool displayOnly = false);

protected:
	bool IsTrusted(const unsigned char* data, unsigned int len, bool permanentOnly);

	void ParseDN(wxDialog* pDlg, const wxString& dn, wxSizer* pSizer);
	void ParseDN_by_prefix(wxDialog* pDlg, std::list<wxString>& tokens, wxString prefix, const wxString& name, wxSizer* pSizer);

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
};

#endif //__VERIFYCERTDIALOG_H__
