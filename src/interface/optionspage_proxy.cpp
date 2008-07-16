#include "FileZilla.h"

#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_proxy.h"

BEGIN_EVENT_TABLE(COptionsPageProxy, COptionsPageProxy::COptionsPage)
EVT_RADIOBUTTON(XRCID("ID_PROXYTYPE_NONE"), COptionsPageProxy::OnProxyTypeChanged)
EVT_RADIOBUTTON(XRCID("ID_PROXYTYPE_HTTP"), COptionsPageProxy::OnProxyTypeChanged)
EVT_RADIOBUTTON(XRCID("ID_PROXYTYPE_SOCKS5"), COptionsPageProxy::OnProxyTypeChanged)
END_EVENT_TABLE()

bool COptionsPageProxy::LoadPage()
{
	bool failure = false;

	SetTextFromOption(XRCID("ID_PROXY_HOST"), OPTION_PROXY_HOST, failure);
	SetTextFromOption(XRCID("ID_PROXY_PORT"), OPTION_PROXY_PORT, failure);
	SetTextFromOption(XRCID("ID_PROXY_USER"), OPTION_PROXY_USER, failure);
	SetTextFromOption(XRCID("ID_PROXY_PASS"), OPTION_PROXY_PASS, failure);

	int type = m_pOptions->GetOptionVal(OPTION_PROXY_TYPE);
	switch (type)
	{
	default:
	case 0:
		SetRCheck(XRCID("ID_PROXYTYPE_NONE"), true, failure);
		break;
	case 1:
		SetRCheck(XRCID("ID_PROXYTYPE_HTTP"), true, failure);
		break;
	case 2:
		SetRCheck(XRCID("ID_PROXYTYPE_SOCKS5"), true, failure);
		break;
	}

	if (!failure)
		SetCtrlState();

	return !failure;
}

bool COptionsPageProxy::SavePage()
{
	SetOptionFromText(XRCID("ID_PROXY_HOST"), OPTION_PROXY_HOST);
	SetOptionFromText(XRCID("ID_PROXY_PORT"), OPTION_PROXY_PORT);
	SetOptionFromText(XRCID("ID_PROXY_USER"), OPTION_PROXY_USER);
	SetOptionFromText(XRCID("ID_PROXY_PASS"), OPTION_PROXY_PASS);

	int type;
	if (GetRCheck(XRCID("ID_PROXYTYPE_HTTP")))
		type = 1;
	else if (GetRCheck(XRCID("ID_PROXYTYPE_SOCKS5")))
		type = 2;
	else
		type = 0;
	m_pOptions->SetOption(OPTION_PROXY_TYPE, type);

	return true;
}

bool COptionsPageProxy::Validate()
{
	if (XRCCTRL(*this, "ID_PROXYTYPE_NONE", wxRadioButton)->GetValue())
		return true;

	wxTextCtrl* pTextCtrl = XRCCTRL(*this, "ID_PROXY_HOST", wxTextCtrl);
	if (pTextCtrl->GetValue() == _T(""))
		return DisplayError(_T("ID_PROXY_HOST"), _("You need to enter a proxy host."));

	pTextCtrl = XRCCTRL(*this, "ID_PROXY_PORT", wxTextCtrl);
	unsigned long port;
	if (!pTextCtrl->GetValue().ToULong(&port) || port < 1 || port > 65536)
		return DisplayError(_T("ID_PROXY_PORT"), _("You need to enter a proxy port in the range from 1 to 65535"));

	return true;
}

void COptionsPageProxy::SetCtrlState()
{
	bool enabled = XRCCTRL(*this, "ID_PROXYTYPE_NONE", wxRadioButton)->GetValue() == 0;

	XRCCTRL(*this, "ID_PROXY_HOST", wxTextCtrl)->Enable(enabled);
	XRCCTRL(*this, "ID_PROXY_PORT", wxTextCtrl)->Enable(enabled);
	XRCCTRL(*this, "ID_PROXY_USER", wxTextCtrl)->Enable(enabled);
	XRCCTRL(*this, "ID_PROXY_PASS", wxTextCtrl)->Enable(enabled);
}

void COptionsPageProxy::OnProxyTypeChanged(wxCommandEvent& event)
{
	SetCtrlState();
}
