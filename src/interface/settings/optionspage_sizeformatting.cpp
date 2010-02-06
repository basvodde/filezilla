#include "FileZilla.h"

#include "../Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_sizeformatting.h"

BEGIN_EVENT_TABLE(COptionsPageSizeFormatting, COptionsPage)
EVT_RADIOBUTTON(wxID_ANY, COptionsPageSizeFormatting::OnRadio)
EVT_CHECKBOX(wxID_ANY, COptionsPageSizeFormatting::OnCheck)
EVT_SPINCTRL(wxID_ANY, COptionsPageSizeFormatting::OnSpin)
END_EVENT_TABLE()

bool COptionsPageSizeFormatting::LoadPage()
{
	bool failure = false;

	const int format = m_pOptions->GetOptionVal(OPTION_SIZE_FORMAT);
	switch (format)
	{
	case 1:
		SetRCheck(XRCID("ID_SIZEFORMAT_IEC"), true, failure);
		break;
	case 2:
		SetRCheck(XRCID("ID_SIZEFORMAT_SI_BINARY"), true, failure);
		break;
	case 3:
		SetRCheck(XRCID("ID_SIZEFORMAT_SI_DECIMAL"), true, failure);
		break;
	default:
		SetRCheck(XRCID("ID_SIZEFORMAT_BYTES"), true, failure);
		break;
	}

	SetCheckFromOption(XRCID("ID_SIZEFORMAT_SEPARATE_THOUTHANDS"), OPTION_SIZE_USETHOUSANDSEP, failure);

	XRCCTRL(*this, "ID_SIZEFORMAT_DECIMALPLACES", wxSpinCtrl)->SetValue(m_pOptions->GetOptionVal(OPTION_SIZE_DECIMALPLACES));

	UpdateControls();
	UpdateExamples();

	return !failure;
}

bool COptionsPageSizeFormatting::SavePage()
{
	m_pOptions->SetOption(OPTION_SIZE_FORMAT, GetFormat());

	SetOptionFromCheck(XRCID("ID_SIZEFORMAT_SEPARATE_THOUTHANDS"), OPTION_SIZE_USETHOUSANDSEP);

	m_pOptions->SetOption(OPTION_SIZE_DECIMALPLACES, XRCCTRL(*this, "ID_SIZEFORMAT_DECIMALPLACES", wxSpinCtrl)->GetValue());

	return true;
}

CSizeFormat::_format COptionsPageSizeFormatting::GetFormat() const
{
	if (GetRCheck(XRCID("ID_SIZEFORMAT_IEC")))
		return CSizeFormat::iec;
	else if (GetRCheck(XRCID("ID_SIZEFORMAT_SI_BINARY")))
		return CSizeFormat::si1024;
	else if (GetRCheck(XRCID("ID_SIZEFORMAT_SI_DECIMAL")))
		return CSizeFormat::si1000;

	return CSizeFormat::bytes;
}

bool COptionsPageSizeFormatting::Validate()
{
	return true;
}

void COptionsPageSizeFormatting::OnRadio(wxCommandEvent& event)
{
	UpdateControls();
	UpdateExamples();
}

void COptionsPageSizeFormatting::OnCheck(wxCommandEvent& event)
{
	UpdateExamples();
}

void COptionsPageSizeFormatting::OnSpin(wxSpinEvent& event)
{
	UpdateExamples();
}

void COptionsPageSizeFormatting::UpdateControls()
{
	const int format = GetFormat();
	XRCCTRL(*this, "ID_SIZEFORMAT_DECIMALPLACES", wxSpinCtrl)->Enable(format != 0);
}

wxString COptionsPageSizeFormatting::FormatSize(const wxLongLong& size)
{
	const CSizeFormat::_format format = GetFormat();
	const bool thousands_separator = GetCheck(XRCID("ID_SIZEFORMAT_SEPARATE_THOUTHANDS"));
	const int num_decimal_places = XRCCTRL(*this, "ID_SIZEFORMAT_DECIMALPLACES", wxSpinCtrl)->GetValue();

	return CSizeFormat::Format(size, false, format, thousands_separator, num_decimal_places);
}

void COptionsPageSizeFormatting::UpdateExamples()
{
	XRCCTRL(*this, "ID_EXAMPLE1", wxStaticText)->SetLabel(FormatSize(12));
	XRCCTRL(*this, "ID_EXAMPLE2", wxStaticText)->SetLabel(FormatSize(100));
	XRCCTRL(*this, "ID_EXAMPLE3", wxStaticText)->SetLabel(FormatSize(1234));
	XRCCTRL(*this, "ID_EXAMPLE4", wxStaticText)->SetLabel(FormatSize(1058817));
	XRCCTRL(*this, "ID_EXAMPLE5", wxStaticText)->SetLabel(FormatSize(123456789));
	XRCCTRL(*this, "ID_EXAMPLE6", wxStaticText)->SetLabel(FormatSize(wxLongLong(0x39E9, 0x4F995A72)));

	GetSizer()->Layout();
}
