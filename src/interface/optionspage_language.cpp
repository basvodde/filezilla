#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_language.h"
#include "filezillaapp.h"

BEGIN_EVENT_TABLE(COptionsPageLanguage, COptionsPage)
END_EVENT_TABLE();

bool COptionsPageLanguage::LoadPage()
{
	bool failure = false;

	wxListBox* pListBox = XRCCTRL(*this, "ID_LANGUAGES", wxListBox);
	if (!pListBox)
		return false;

	wxString localesDir = wxGetApp().GetLocalesDir();
	wxDir dir(localesDir);
	
	int language = wxGetApp().GetCurrentLanguage();

	int n = pListBox->Append(_T("English"));

	const wxLanguageInfo* pInfo = wxLocale::FindLanguageInfo(_T("en"));
	if (pInfo && pInfo->Language == language)
		pListBox->SetSelection(n);

	wxLogNull log;

	wxString locale;
	for (bool found = dir.GetFirst(&locale); found; found = dir.GetNext(&locale))
	{
		if (!wxFileName::FileExists(localesDir + locale + _T("/filezilla.mo")))
			continue;
		
		wxString name;
		const wxLanguageInfo* pInfo = wxLocale::FindLanguageInfo(locale);
		if (!pInfo)
			continue;
		if (pInfo->Description != _T(""))
			name = pInfo->Description;
		else
			name = locale;

		int n = pListBox->Append(name);
		if (pInfo->Language == language)
			pListBox->SetSelection(n);
	}
	
	return !failure;
}

bool COptionsPageLanguage::SavePage()
{
	wxListBox* pListBox = XRCCTRL(*this, "ID_LANGUAGES", wxListBox);

	if (pListBox->GetSelection() == wxNOT_FOUND)
		return true;

	const wxLanguageInfo* pInfo = wxLocale::FindLanguageInfo(pListBox->GetStringSelection());
	if (!pInfo || !wxGetApp().SetLocale(pInfo->Language))
		wxMessageBox(wxString::Format(_("Failed to set language to %s, using default system language"), pListBox->GetStringSelection().c_str()), _("Failed to change language"), wxICON_EXCLAMATION, this);
	else
		m_pOptions->SetOption(OPTION_LANGUAGE, pListBox->GetStringSelection());
	return true;
}

bool COptionsPageLanguage::Validate()
{
	
	return true;
}
