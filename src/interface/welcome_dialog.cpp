#include "filezilla.h"
#include "welcome_dialog.h"
#include "buildinfo.h"
#include <wx/hyperlink.h>
#include "Options.h"

bool CWelcomeDialog::Run(wxWindow* parent, bool force /*=false*/)
{
	const wxString ownVersion = CBuildInfo::GetVersion();
	wxString greetingVersion = COptions::Get()->GetOption(OPTION_GREETINGVERSION);

	if (!force)
	{
		greetingVersion = COptions::Get()->GetOption(OPTION_GREETINGVERSION);
		if (greetingVersion != _T("") &&
			CBuildInfo::ConvertToVersionNumber(ownVersion) <= CBuildInfo::ConvertToVersionNumber(greetingVersion))
		{
			// Been there done that
			return true;
		}
		COptions::Get()->SetOption(OPTION_GREETINGVERSION, ownVersion);
	}

	if (!wxXmlResource::Get()->LoadDialog(this, parent, _T("ID_WELCOME")))
		return false;

	XRCCTRL(*this, "ID_FZVERSION", wxStaticText)->SetLabel(_T("FileZilla ") + CBuildInfo::GetVersion());

	const wxString url = _T("http://welcome.filezilla-project.org/welcome?type=client&category=%s&version=") + ownVersion;

	wxHyperlinkCtrl* pNews = XRCCTRL(*this, "ID_LINK_NEWS", wxHyperlinkCtrl);
	pNews->SetURL(wxString::Format(url, _T("news")) + _T("&oldversion=") + greetingVersion);

	if (greetingVersion != _T(""))
	{
		wxHyperlinkCtrl* pNews = XRCCTRL(*this, "ID_LINK_NEWS", wxHyperlinkCtrl);
		pNews->SetLabel(wxString::Format(_("New features and improvements in %s"), CBuildInfo::GetVersion().c_str()));
	}
	else
	{
		XRCCTRL(*this, "ID_HEADING_NEWS", wxStaticText)->Hide();
		pNews->Hide();
	}

	XRCCTRL(*this, "ID_DOCUMENTATION_BASIC", wxHyperlinkCtrl)->SetURL(wxString::Format(url, _T("documentation_basic")));
	XRCCTRL(*this, "ID_DOCUMENTATION_NETWORK", wxHyperlinkCtrl)->SetURL(wxString::Format(url, _T("documentation_network")));
	XRCCTRL(*this, "ID_DOCUMENTATION_MORE", wxHyperlinkCtrl)->SetURL(wxString::Format(url, _T("documentation_more")));
	XRCCTRL(*this, "ID_SUPPORT_FORUM", wxHyperlinkCtrl)->SetURL(wxString::Format(url, _T("support_forum")));
	XRCCTRL(*this, "ID_SUPPORT_MORE", wxHyperlinkCtrl)->SetURL(wxString::Format(url, _T("support_more")));
	Layout();

	GetSizer()->Fit(this);

	ShowModal();

	return true;
}
