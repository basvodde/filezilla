#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"

wxString validationFailed = _("Failed to validate settings");

bool COptionsPage::CreatePage(COptions* pOptions, wxWindow* parent, wxSize& maxSize)
{
	m_pOptions = pOptions;

	if (!wxXmlResource::Get()->LoadPanel(this, parent, GetResourceName()))
		return false;

	wxSize size = GetSize();
	if (size.GetWidth() > maxSize.GetWidth())
		maxSize.SetWidth(size.GetWidth());
	if (size.GetHeight() > maxSize.GetHeight())
		maxSize.SetHeight(size.GetHeight());

	Hide();

	return true;
}

void COptionsPage::SetCheck(const char* id, bool checked, bool& failure)
{
	wxCheckBox* pCheckBox = XRCCTRL(*this, id, wxCheckBox);
	if (!pCheckBox)
	{
		failure = true;
		return;
	}

	pCheckBox->SetValue(checked);
}

bool COptionsPage::GetCheck(const char* id)
{
	wxCheckBox* pCheckBox = XRCCTRL(*this, id, wxCheckBox);
	wxASSERT(pCheckBox);

	return pCheckBox->GetValue();
}

void COptionsPage::SetText(const char* id, const wxString& text, bool& failure)
{
	wxTextCtrl* pTextCtrl = XRCCTRL(*this, id, wxTextCtrl);
	if (!pTextCtrl)
	{
		failure = true;
		return;
	}

	pTextCtrl->SetValue(text);
}

wxString COptionsPage::GetText(const char* id)
{
	wxTextCtrl* pTextCtrl = XRCCTRL(*this, id, wxTextCtrl);
	wxASSERT(pTextCtrl);

	return pTextCtrl->GetValue();
}
