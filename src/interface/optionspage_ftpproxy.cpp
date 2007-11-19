#include "FileZilla.h"

#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_ftpproxy.h"

BEGIN_EVENT_TABLE(COptionsPageFtpProxy, COptionsPageFtpProxy::COptionsPage)
EVT_RADIOBUTTON(XRCID("ID_PROXYTYPE_NONE"), COptionsPageFtpProxy::OnProxyTypeChanged)
EVT_RADIOBUTTON(XRCID("ID_PROXYTYPE_USER"), COptionsPageFtpProxy::OnProxyTypeChanged)
EVT_RADIOBUTTON(XRCID("ID_PROXYTYPE_SITE"), COptionsPageFtpProxy::OnProxyTypeChanged)
EVT_RADIOBUTTON(XRCID("ID_PROXYTYPE_OPEN"), COptionsPageFtpProxy::OnProxyTypeChanged)
EVT_TEXT(XRCID("ID_LOGINSEQUENCE"), COptionsPageFtpProxy::OnLoginSequenceChanged)
END_EVENT_TABLE()

bool COptionsPageFtpProxy::LoadPage()
{
	bool failure = false;

	SetTextFromOption(XRCID("ID_PROXY_HOST"), OPTION_FTP_PROXY_HOST, failure);
	SetTextFromOption(XRCID("ID_PROXY_USER"), OPTION_FTP_PROXY_USER, failure);
	SetTextFromOption(XRCID("ID_PROXY_PASS"), OPTION_FTP_PROXY_PASS, failure);

	int type = m_pOptions->GetOptionVal(OPTION_FTP_PROXY_TYPE);
	switch (type)
	{
	default:
	case 0:
		SetRCheck(XRCID("ID_PROXYTYPE_NONE"), true, failure);
		break;
	case 1:
		SetRCheck(XRCID("ID_PROXYTYPE_USER"), true, failure);
		break;
	case 2:
		SetRCheck(XRCID("ID_PROXYTYPE_SITE"), true, failure);
		break;
	case 3:
		SetRCheck(XRCID("ID_PROXYTYPE_OPEN"), true, failure);
		break;
	case 4:
		SetRCheck(XRCID("ID_PROXYTYPE_CUSTOM"), true, failure);
		SetTextFromOption(XRCID("ID_LOGINSEQUENCE"), OPTION_FTP_PROXY_CUSTOMLOGINSEQUENCE, failure);
		break;
	}

	if (!failure)
		SetCtrlState();

	return !failure;
}

bool COptionsPageFtpProxy::SavePage()
{
	SetOptionFromText(XRCID("ID_PROXY_HOST"), OPTION_FTP_PROXY_HOST);
	SetOptionFromText(XRCID("ID_PROXY_USER"), OPTION_FTP_PROXY_USER);
	SetOptionFromText(XRCID("ID_PROXY_PASS"), OPTION_FTP_PROXY_PASS);

	int type = 0;
	if (GetRCheck(XRCID("ID_PROXYTYPE_USER")))
		type = 1;
	else if (GetRCheck(XRCID("ID_PROXYTYPE_SITE")))
		type = 2;
	else if (GetRCheck(XRCID("ID_PROXYTYPE_OPEN")))
		type = 3;
	else if (GetRCheck(XRCID("ID_PROXYTYPE_CUSTOM")))
	{
		SetOptionFromText(XRCID("ID_LOGINSEQUENCE"), OPTION_FTP_PROXY_CUSTOMLOGINSEQUENCE);
		type = 4;
	}
	m_pOptions->SetOption(OPTION_FTP_PROXY_TYPE, type);

	return true;
}

bool COptionsPageFtpProxy::Validate()
{
	if (!XRCCTRL(*this, "ID_PROXYTYPE_NONE", wxRadioButton)->GetValue())
	{
		wxTextCtrl* pTextCtrl = XRCCTRL(*this, "ID_PROXY_HOST", wxTextCtrl);
		if (pTextCtrl->GetValue() == _T(""))
			return DisplayError(_T("ID_PROXY_HOST"), _("You need to enter a proxy host."));
	}

	if (XRCCTRL(*this, "ID_PROXYTYPE_CUSTOM", wxRadioButton)->GetValue())
	{
		wxTextCtrl* pTextCtrl = XRCCTRL(*this, "ID_LOGINSEQUENCE", wxTextCtrl);
		if (pTextCtrl->GetValue() == _T(""))
			return DisplayError(_T("ID_LOGINSEQUENCE"), _("The custom login sequence cannot be empty."));
	}

	return true;
}

void COptionsPageFtpProxy::SetCtrlState()
{
	wxTextCtrl* pTextCtrl = XRCCTRL(*this, "ID_LOGINSEQUENCE", wxTextCtrl);
	if (!pTextCtrl)
		return;

	if (XRCCTRL(*this, "ID_PROXYTYPE_NONE", wxRadioButton)->GetValue())
	{
		pTextCtrl->ChangeValue(_T(""));
		pTextCtrl->Enable(false);
		pTextCtrl->SetEditable(false);
		return;
	}

	pTextCtrl->Enable(true);
	if (XRCCTRL(*this, "ID_PROXYTYPE_CUSTOM", wxRadioButton)->GetValue())
		return;

	wxString loginSequence = _T("USER %s\nPASS %w\n");

	if (XRCCTRL(*this, "ID_PROXYTYPE_USER", wxRadioButton)->GetValue())
		loginSequence += _T("USER %u@%h\n");
	else
	{
		if (XRCCTRL(*this, "ID_PROXYTYPE_SITE", wxRadioButton)->GetValue())
			loginSequence += _T("SITE %h\n");
		else
			loginSequence += _T("OPEN %h\n");
		loginSequence += _T("USER %u\n");
	}

	loginSequence += _T("PASS %p\nACCT %a");

	pTextCtrl->ChangeValue(loginSequence);
}

void COptionsPageFtpProxy::OnProxyTypeChanged(wxCommandEvent& event)
{
	SetCtrlState();
}

void COptionsPageFtpProxy::OnLoginSequenceChanged(wxCommandEvent& event)
{
	XRCCTRL(*this, "ID_PROXYTYPE_CUSTOM", wxRadioButton)->SetValue(true);
}
