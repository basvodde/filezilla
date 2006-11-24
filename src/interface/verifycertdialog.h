#ifndef __VERIFYCERTDIALOG_H__
#define __VERIFYCERTDIALOG_H__

class CVerifyCertDialog
{
public:
	bool IsTrusted(CCertificateNotification* pNotification);
	void ShowVerificationDialog(CCertificateNotification* pNotification);

protected:
	void ParseDN(wxDialog* pDlg, const wxString& dn, wxSizer* pSizer);
	void ParseDN_by_prefix(wxDialog* pDlg, std::list<wxString>& tokens, wxString prefix, const wxString& name, wxSizer* pSizer);

	struct t_certData
	{
		unsigned char* data;
		unsigned int len;
	};
	static std::list<t_certData> m_trustedCerts;
};

#endif //__VERIFYCERTDIALOG_H__
