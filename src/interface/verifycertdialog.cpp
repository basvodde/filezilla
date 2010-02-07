#include <filezilla.h>
#include "verifycertdialog.h"
#include <wx/tokenzr.h>
#include "dialogex.h"
#include "ipcmutex.h"

CVerifyCertDialog::~CVerifyCertDialog()
{
	for (std::list<t_certData>::iterator iter = m_trustedCerts.begin(); iter != m_trustedCerts.end(); iter++)
		delete [] iter->data;
	for (std::list<t_certData>::iterator iter = m_sessionTrustedCerts.begin(); iter != m_sessionTrustedCerts.end(); iter++)
		delete [] iter->data;
}

bool CVerifyCertDialog::DisplayCert(wxDialogEx* pDlg, const CCertificate& cert)
{
	bool warning = false;
	if (cert.GetActivationTime().IsValid())
	{
		if (cert.GetActivationTime() > wxDateTime::Now())
		{
			pDlg->SetLabel(XRCID("ID_ACTIVATION_TIME"), wxString::Format(_("%s - Not yet valid!"), cert.GetActivationTime().FormatDate().c_str()));
			warning = true;
		}
		else
			pDlg->SetLabel(XRCID("ID_ACTIVATION_TIME"), cert.GetActivationTime().FormatDate());
	}
	else
	{
		warning = true;
		pDlg->SetLabel(XRCID("ID_ACTIVATION_TIME"), _("Invalid date"));
	}

	if (cert.GetExpirationTime().IsValid())
	{
		if (cert.GetExpirationTime() < wxDateTime::Now())
		{
			pDlg->SetLabel(XRCID("ID_EXPIRATION_TIME"), wxString::Format(_("%s - Certificate expired!"), cert.GetExpirationTime().FormatDate().c_str()));
			warning = true;
		}
		else
			pDlg->SetLabel(XRCID("ID_EXPIRATION_TIME"), cert.GetExpirationTime().FormatDate());
	}
	else
	{
		warning = true;
		pDlg->SetLabel(XRCID("ID_EXPIRATION_TIME"), _("Invalid date"));
	}

	if (cert.GetSerial() != _T(""))
		pDlg->SetLabel(XRCID("ID_SERIAL"), cert.GetSerial());
	else
		pDlg->SetLabel(XRCID("ID_SERIAL"), _("None"));

	pDlg->SetLabel(XRCID("ID_PKALGO"), wxString::Format(_("%s with %d bits"), cert.GetPkAlgoName().c_str(), cert.GetPkAlgoBits()));

	pDlg->SetLabel(XRCID("ID_FINGERPRINT_MD5"), cert.GetFingerPrintMD5());
	pDlg->SetLabel(XRCID("ID_FINGERPRINT_SHA1"), cert.GetFingerPrintSHA1());

	ParseDN(pDlg, cert.GetSubject(), m_pSubjectSizer);
	ParseDN(pDlg, cert.GetIssuer(), m_pIssuerSizer);

	return warning;
}

void CVerifyCertDialog::ShowVerificationDialog(CCertificateNotification* pNotification, bool displayOnly /*=false*/)
{
	LoadTrustedCerts();

	m_pDlg = new wxDialogEx;
	if (displayOnly)
		m_pDlg->Load(0, _T("ID_DISPLAYCERT"));
	else
	{
		m_pDlg->Load(0, _T("ID_VERIFYCERT"));

		m_pDlg->WrapText(m_pDlg, XRCID("ID_DESC"), 400);
	}

	m_certificates = pNotification->GetCertificates();
	if (m_certificates.size() == 1)
	{
		XRCCTRL(*m_pDlg, "ID_CHAIN_DESC", wxStaticText)->Hide();
		XRCCTRL(*m_pDlg, "ID_CHAIN", wxChoice)->Hide();
	}
	else
	{
		wxChoice* pChoice = XRCCTRL(*m_pDlg, "ID_CHAIN", wxChoice);
		for (unsigned int i = 0; i < m_certificates.size(); i++)
		{
			pChoice->Append(wxString::Format(_T("%d"), i));
		}
		pChoice->SetSelection(0);

		pChoice->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(CVerifyCertDialog::OnCertificateChoice), 0, this);
	}

	m_pDlg->SetLabel(XRCID("ID_HOST"), wxString::Format(_T("%s:%d"), pNotification->GetHost().c_str(), pNotification->GetPort()));

	m_pSubjectSizer = XRCCTRL(*m_pDlg, "ID_SUBJECT_DUMMY", wxStaticText)->GetContainingSizer();
	XRCCTRL(*m_pDlg, "ID_SUBJECT_DUMMY", wxStaticText)->Destroy();

	m_pIssuerSizer = XRCCTRL(*m_pDlg, "ID_ISSUER_DUMMY", wxStaticText)->GetContainingSizer();
	XRCCTRL(*m_pDlg, "ID_ISSUER_DUMMY", wxStaticText)->Destroy();

	bool warning = DisplayCert(m_pDlg, m_certificates[0]);

	m_pDlg->SetLabel(XRCID("ID_CIPHER"), pNotification->GetSessionCipher());
	m_pDlg->SetLabel(XRCID("ID_MAC"), pNotification->GetSessionMac());

	if (warning)
	{
		XRCCTRL(*m_pDlg, "ID_IMAGE", wxStaticBitmap)->SetBitmap(wxArtProvider::GetBitmap(wxART_WARNING));
		if (!displayOnly)
			XRCCTRL(*m_pDlg, "ID_ALWAYS", wxCheckBox)->Enable(false);
	}

	m_pDlg->GetSizer()->Fit(m_pDlg);
	m_pDlg->GetSizer()->SetSizeHints(m_pDlg);

	int res = m_pDlg->ShowModal();

	if (!displayOnly)
	{
		if (res == wxID_OK)
		{
			wxASSERT(!IsTrusted(pNotification));

			pNotification->m_trusted = true;

			if (!warning && XRCCTRL(*m_pDlg, "ID_ALWAYS", wxCheckBox)->GetValue())
				SetPermanentlyTrusted(pNotification);
			else
			{
				t_certData cert;
				const unsigned char* data = m_certificates[0].GetRawData(cert.len);
				cert.data = new unsigned char[cert.len];
				memcpy(cert.data, data, cert.len);
				m_sessionTrustedCerts.push_back(cert);
			}
		}
		else
			pNotification->m_trusted = false;
	}

	delete m_pDlg;
	m_pDlg = 0;
}

void CVerifyCertDialog::ParseDN(wxDialog* pDlg, const wxString& dn, wxSizer* pSizer)
{
	pSizer->Clear(true);

	wxStringTokenizer tokens(dn, _T(","));

	std::list<wxString> tokenlist;
	while (tokens.HasMoreTokens())
		tokenlist.push_back(tokens.GetNextToken());

	ParseDN_by_prefix(pDlg, tokenlist, _T("CN"), _("Common name:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("O"), _("Organization:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("2.5.4.15"), _("Business category:"), pSizer, true);
	ParseDN_by_prefix(pDlg, tokenlist, _T("OU"), _("Unit:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("T"), _("Title:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("C"), _("Country:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("ST"), _("State or province:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("L"), _("Locality:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("2.5.4.17"), _("Postal code:"), pSizer, true);
	ParseDN_by_prefix(pDlg, tokenlist, _T("STREET"), _("Street:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("EMAIL"), _("E-Mail:"), pSizer);
	ParseDN_by_prefix(pDlg, tokenlist, _T("1.3.6.1.4.1.311.60.2.1.3"), _("Jurisdiction country:"), pSizer, true);
	ParseDN_by_prefix(pDlg, tokenlist, _T("1.3.6.1.4.1.311.60.2.1.2"), _("Jurisdiction state or province:"), pSizer, true);	
	ParseDN_by_prefix(pDlg, tokenlist, _T("1.3.6.1.4.1.311.60.2.1.1"), _("Jurisdiction locality:"), pSizer, true);

	if (!tokenlist.empty())
	{
		wxString value = tokenlist.front();
		for (std::list<wxString>::const_iterator iter = ++tokenlist.begin(); iter != tokenlist.end(); iter++)
			value += _T(",") + *iter;

		pSizer->Add(new wxStaticText(pDlg, wxID_ANY, _("Other:")));
		pSizer->Add(new wxStaticText(pDlg, wxID_ANY, value));
	}
}

void CVerifyCertDialog::ParseDN_by_prefix(wxDialog* pDlg, std::list<wxString>& tokens, wxString prefix, const wxString& name, wxSizer* pSizer, bool decode /*=false*/)
{
	prefix += _T("=");
	const int len = prefix.Length();
	
	wxString value;

	bool append = false;

	std::list<wxString>::iterator iter = tokens.begin();
	while (iter != tokens.end())
	{
		if (!append)
		{
			if (iter->Left(len) != prefix)
			{
				iter++;
				continue;
			}

			if (value != _T(""))
				value += _T("\n");
		}
		else
		{
			append = false;
			value += _T(",");
		}

		value += iter->Mid(len);

		if (iter->Last() == '\\')
		{
			value.RemoveLast();
			append = true;
		}

		std::list<wxString>::iterator remove = iter++;
		tokens.erase(remove);
	}

	if (decode)
		value = DecodeValue(value);

	if (value != _T(""))
	{
		pSizer->Add(new wxStaticText(pDlg, wxID_ANY, name));
		pSizer->Add(new wxStaticText(pDlg, wxID_ANY, value));
	}
}

bool CVerifyCertDialog::IsTrusted(CCertificateNotification* pNotification)
{
	LoadTrustedCerts();

	wxASSERT(pNotification);

	unsigned int len;
	CCertificate cert =  pNotification->GetCertificates()[0];
	const unsigned char* data = cert.GetRawData(len);

	return IsTrusted(data, len, false);
}

bool CVerifyCertDialog::IsTrusted(const unsigned char* data, unsigned int len, bool permanentOnly)
{
	for (std::list<t_certData>::const_iterator iter = m_trustedCerts.begin(); iter != m_trustedCerts.end(); iter++)
	{
		if (iter->len != len)
			continue;

		if (!memcmp(iter->data, data, len))
			return true;
	}

	if (permanentOnly)
		return false;

	for (std::list<t_certData>::const_iterator iter = m_sessionTrustedCerts.begin(); iter != m_sessionTrustedCerts.end(); iter++)
	{
		if (iter->len != len)
			continue;

		if (!memcmp(iter->data, data, len))
			return true;
	}

	return false;
}

wxString CVerifyCertDialog::ConvertHexToString(const unsigned char* data, unsigned int len)
{
	wxString str;
	for (unsigned int i = 0; i < len; i++)
	{
		const unsigned char& c = data[i];

		const unsigned char low = c & 0x0F;
		const unsigned char high = (c & 0xF0) >> 4;

		if (high < 10)
			str += '0' + high;
		else
			str += 'A' + high - 10;

		if (low < 10)
			str += '0' + low;
		else
			str += 'A' + low - 10;
	}

	return str;
}

unsigned char* CVerifyCertDialog::ConvertStringToHex(const wxString& str, unsigned int &len)
{
	len = str.Length() / 2;
	unsigned char* data = new unsigned char[len];

	unsigned int j = 0;
	for (unsigned int i = 0; i < str.Length(); i++, j++)
	{
		wxChar high = str[i++];
		wxChar low = str[i];

		if (high >= '0' && high <= '9')
			high -= '0';
		else if (high >= 'A' && high <= 'F')
			high -= 'A' - 10;
		else
		{
			delete [] data;
			return 0;
		}

		if (low >= '0' && low <= '9')
			low -= '0';
		else if (low >= 'A' && low <= 'F')
			low -= 'A' - 10;
		else
		{
			delete [] data;
			return 0;
		}

		data[j] = ((unsigned char)high << 4) + (unsigned char)low;
	}

	return data;
}

void CVerifyCertDialog::LoadTrustedCerts(bool close /*=true*/)
{
	CReentrantInterProcessMutexLocker mutex(MUTEX_TRUSTEDCERTS);
	if (!m_xmlFile.HasFileName() || m_xmlFile.Modified())
		m_xmlFile.Load(_T("trustedcerts"));
	else
		return;

	TiXmlElement* pElement = m_xmlFile.GetElement();
	if (!pElement)
	{
		if (close)
			m_xmlFile.Close();
		return;
	}

	m_trustedCerts.clear();

	if (!(pElement = pElement->FirstChildElement("TrustedCerts")))
		return;

	bool modified = false;

	TiXmlElement* pCert = pElement->FirstChildElement("Certificate");
	while (pCert)
	{
		wxString value = GetTextElement(pCert, "Data");

		TiXmlElement* pRemove = 0;
		
		t_certData data;
		if (value == _T("") || !(data.data = ConvertStringToHex(value, data.len)))
			pRemove = pCert;

		wxLongLong activationTime = GetTextElementLongLong(pCert, "ActivationTime", 0);
		if (activationTime == 0 || activationTime > wxDateTime::GetTimeNow())
			pRemove = pCert;

		wxLongLong expirationTime = GetTextElementLongLong(pCert, "ExpirationTime", 0);
		if (expirationTime == 0 || expirationTime < wxDateTime::GetTimeNow())
			pRemove = pCert;

		if (IsTrusted(data.data, data.len, true))
			pRemove = pCert;

		if (!pRemove)
			m_trustedCerts.push_back(data);
		else
			delete [] data.data;
		
		pCert = pCert->NextSiblingElement("Certificate");

		if (pRemove)
		{
			modified = true;
			pElement->RemoveChild(pRemove);
		}
	}

	if (modified)
		m_xmlFile.Save();

	if (close)
		m_xmlFile.Close();
}

void CVerifyCertDialog::SetPermanentlyTrusted(const CCertificateNotification* const pNotification)
{
	const CCertificate certificate = pNotification->GetCertificates()[0];
	unsigned int len;
	const unsigned char* const data = certificate.GetRawData(len);

	CReentrantInterProcessMutexLocker mutex(MUTEX_TRUSTEDCERTS);
	LoadTrustedCerts(false);

	if (IsTrusted(data, len, true))
	{
		m_xmlFile.Close();
		return;
	}

	t_certData cert;
	cert.len = len;
	cert.data = new unsigned char[len];
	memcpy(cert.data, data, len);
	m_trustedCerts.push_back(cert);

	TiXmlElement* pElement = m_xmlFile.GetElement();
	if (!pElement)
	{
		m_xmlFile.Load();
		pElement = m_xmlFile.GetElement();
	}

	if (!pElement)
	{
		m_xmlFile.Close();
		return;
	}

	TiXmlElement* pCerts = pElement->FirstChildElement("TrustedCerts");
	if (!pCerts)
		pCerts = pElement->LinkEndChild(new TiXmlElement("TrustedCerts"))->ToElement();

	TiXmlElement* pCert = pCerts->LinkEndChild(new TiXmlElement("Certificate"))->ToElement();

	AddTextElement(pCert, "Data", ConvertHexToString(data, len));

	wxLongLong time = certificate.GetActivationTime().GetTicks();
	AddTextElement(pCert, "ActivationTime", time.ToString());

	time = certificate.GetExpirationTime().GetTicks();
	AddTextElement(pCert, "ExpirationTime", time.ToString());

	m_xmlFile.Save();
	m_xmlFile.Close();
}

wxString CVerifyCertDialog::DecodeValue(const wxString& value)
{
	// Decodes string in hex notation
	// #xxxx466F6F626172 -> Foobar
	// First two encoded bytes are ignored, some weird type information I don't care about
	// Only accepts ASCII for now.
	if (value[0] != '#')
		return value;

	unsigned int len = value.Len();
	//if (!(len % 2))
	//	return value;

	wxString out;

	for (unsigned int i = 5; i < len; i += 2)
	{
		wxChar c = value[i];
		wxChar d = value[i + 1];
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'a' && c <= 'z')
			c -= 'a' - 10;
		else if (c >= 'A' && c <= 'Z')
			c -= 'A' - 10;
		else
			continue;
			//return value;
		if (d >= '0' && d <= '9')
			d -= '0';
		else if (d >= 'a' && d <= 'z')
			d -= 'a' - 10;
		else if (d >= 'A' && d <= 'Z')
			d -= 'A' - 10;
		else
			continue;
			//return value;

		c = c * 16 + d;
		//if (c > 127)
		//	return value;
		if (c > 127 || c < 32)
			continue;
		out += c;
	}

	return out;
}

void CVerifyCertDialog::OnCertificateChoice(wxCommandEvent& event)
{
	int sel = event.GetSelection();
	if (sel < 0 || sel > (int)m_certificates.size())
		return;
	DisplayCert(m_pDlg, m_certificates[sel]);

	m_pDlg->Layout();
	m_pDlg->GetSizer()->Fit(m_pDlg);
}
