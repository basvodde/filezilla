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
	wxListBox* pListBox = XRCCTRL(*this, "ID_LANGUAGES", wxListBox);
	if (!pListBox)
		return false;

	wxString currentLanguage = m_pOptions->GetOption(OPTION_LANGUAGE);

	pListBox->Clear();

	const wxString defaultName = _("Default system language");
	int n = pListBox->Append(defaultName);
	if (currentLanguage == _T(""))
		pListBox->SetSelection(n);

	n = pListBox->Append(_T("English"));
	if (currentLanguage == _T("en"))
		pListBox->SetSelection(n);
	m_localeMap[_T("en")] = _T("English");

	wxString localesDir = wxGetApp().GetLocalesDir();
	if (localesDir == _T("") || !wxDir::Exists(localesDir))
		return true;

	wxDir dir(localesDir);
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

		m_localeMap[locale] = name;

		int n = pListBox->Append(name);
		if (locale == currentLanguage)
			pListBox->SetSelection(n);
	}

	return true;
}

bool COptionsPageLanguage::SavePage()
{
	wxListBox* pListBox = XRCCTRL(*this, "ID_LANGUAGES", wxListBox);

	if (pListBox->GetSelection() == wxNOT_FOUND)
		return true;

	const wxString& selection = pListBox->GetStringSelection();
	wxString code;
	std::map<wxString, wxString>::const_iterator iter;
	for (iter = m_localeMap.begin(); iter != m_localeMap.end(); iter++)
	{
		if (iter->second == selection)
			break;
	}
	if (iter != m_localeMap.end())
		code = iter->first;

#ifdef __WXGTK__
	m_pOptions->SetOption(OPTION_LANGUAGE, code);
#else
	bool successful = false;
	if (code == _T(""))
	{
		wxGetApp().SetLocale(wxLANGUAGE_DEFAULT);

		// Default language cannot fail, has to silently fall back to English
		successful = true;
	}
	else
	{
		const wxLanguageInfo* pInfo = wxLocale::FindLanguageInfo(code);
		if (pInfo)
			successful = wxGetApp().SetLocale(pInfo->Language);
	}

	if (successful)
		m_pOptions->SetOption(OPTION_LANGUAGE, code);
	else
		wxMessageBox(wxString::Format(_("Failed to set language to %s, using default system language"), pListBox->GetStringSelection().c_str()), _("Failed to change language"), wxICON_EXCLAMATION, this);		
#endif
	return true;
}

bool COptionsPageLanguage::Validate()
{
	return true;
}
