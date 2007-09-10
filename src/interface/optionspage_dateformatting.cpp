#include "FileZilla.h"

#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_dateformatting.h"
#include "updatewizard.h"

BEGIN_EVENT_TABLE(COptionsPageDateFormatting, COptionsPage)
EVT_RADIOBUTTON(wxID_ANY, COptionsPageDateFormatting::OnRadioChanged)
END_EVENT_TABLE()

bool COptionsPageDateFormatting::LoadPage()
{
	bool failure = false;

	const wxString& dateFormat = m_pOptions->GetOption(OPTION_DATE_FORMAT);
	if (dateFormat == _T("1"))
		SetRCheck(XRCID("ID_DATEFORMAT_ISO"), true, failure);
	else if (dateFormat[0] == '2')
	{
		SetRCheck(XRCID("ID_DATEFORMAT_CUSTOM"), true, failure);
		SetText(XRCID("ID_CUSTOM_DATEFORMAT"), dateFormat.Mid(1), failure);
	}
	else
		SetRCheck(XRCID("ID_DATEFORMAT_DEFAULT"), true, failure);

	const wxString& timeFormat = m_pOptions->GetOption(OPTION_TIME_FORMAT);
	if (timeFormat == _T("1"))
		SetRCheck(XRCID("ID_TIMEFORMAT_ISO"), true, failure);
	else if (timeFormat[0] == '2')
	{
		SetRCheck(XRCID("ID_TIMEFORMAT_CUSTOM"), true, failure);
		SetText(XRCID("ID_CUSTOM_TIMEFORMAT"), timeFormat.Mid(1), failure);
	}
	else
		SetRCheck(XRCID("ID_TIMEFORMAT_DEFAULT"), true, failure);

	if (!failure)
		SetCtrlState();

	return !failure;
}

bool COptionsPageDateFormatting::SavePage()
{
	wxString dateFormat;
	if (GetRCheck(XRCID("ID_DATEFORMAT_CUSTOM")))
		dateFormat = _T("2") + XRCCTRL(*this, "ID_CUSTOM_DATEFORMAT", wxTextCtrl)->GetValue();
	else if (GetRCheck(XRCID("ID_DATEFORMAT_ISO")))
		dateFormat = _T("1");
	else
		dateFormat = _T("0");
	m_pOptions->SetOption(OPTION_DATE_FORMAT, dateFormat);

	wxString timeFormat;
	if (GetRCheck(XRCID("ID_TIMEFORMAT_CUSTOM")))
		timeFormat = _T("2") + XRCCTRL(*this, "ID_CUSTOM_TIMEFORMAT", wxTextCtrl)->GetValue();
	else if (GetRCheck(XRCID("ID_TIMEFORMAT_ISO")))
		timeFormat = _T("1");
	else
		timeFormat = _T("0");
	m_pOptions->SetOption(OPTION_TIME_FORMAT, timeFormat);

	return true;
}

bool COptionsPageDateFormatting::Validate()
{
	if (GetRCheck(XRCID("ID_DATEFORMAT_CUSTOM")) && XRCCTRL(*this, "ID_CUSTOM_DATEFORMAT", wxTextCtrl)->GetValue() == _T(""))
	{
		XRCCTRL(*this, "ID_CUSTOM_DATEFORMAT", wxTextCtrl)->SetFocus();
		wxMessageBox(_("Please enter a custom date format."), validationFailed, wxICON_EXCLAMATION, this);
		return false;
	}

	if (GetRCheck(XRCID("ID_TIMEFORMAT_CUSTOM")) && XRCCTRL(*this, "ID_CUSTOM_TIMEFORMAT", wxTextCtrl)->GetValue() == _T(""))
	{
		XRCCTRL(*this, "ID_CUSTOM_TIMEFORMAT", wxTextCtrl)->SetFocus();
		wxMessageBox(_("Please enter a custom time format."), validationFailed, wxICON_EXCLAMATION, this);
		return false;
	}

	return true;
}

void COptionsPageDateFormatting::OnRadioChanged(wxCommandEvent& event)
{
	SetCtrlState();
}

void COptionsPageDateFormatting::SetCtrlState()
{
	XRCCTRL(*this, "ID_CUSTOM_DATEFORMAT", wxTextCtrl)->Enable(GetRCheck(XRCID("ID_DATEFORMAT_CUSTOM")));
	XRCCTRL(*this, "ID_CUSTOM_TIMEFORMAT", wxTextCtrl)->Enable(GetRCheck(XRCID("ID_TIMEFORMAT_CUSTOM")));
}
