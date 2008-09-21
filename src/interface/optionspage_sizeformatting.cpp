#include "FileZilla.h"

#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_sizeformatting.h"
#ifndef __WXMSW__
#include <langinfo.h>
#endif

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

	SetCheck(XRCID("ID_SIZEFORMAT_SEPARATE_THOUTHANDS"), m_pOptions->GetOptionVal(OPTION_SIZE_USETHOUSANDSEP) != 0, failure);

	XRCCTRL(*this, "ID_SIZEFORMAT_DECIMALPLACES", wxSpinCtrl)->SetValue(m_pOptions->GetOptionVal(OPTION_SIZE_DECIMALPLACES));

	UpdateControls();
	UpdateExamples();

	return !failure;
}

bool COptionsPageSizeFormatting::SavePage()
{
	m_pOptions->SetOption(OPTION_SIZE_FORMAT, GetFormat());

	m_pOptions->SetOption(OPTION_SIZE_USETHOUSANDSEP, GetCheck(XRCID("ID_SIZEFORMAT_SEPARATE_THOUTHANDS")) ? 1 : 0);

	m_pOptions->SetOption(OPTION_SIZE_DECIMALPLACES, XRCCTRL(*this, "ID_SIZEFORMAT_DECIMALPLACES", wxSpinCtrl)->GetValue());

	return true;
}

int COptionsPageSizeFormatting::GetFormat()
{
	if (GetRCheck(XRCID("ID_SIZEFORMAT_IEC")))
		return 1;
	else if (GetRCheck(XRCID("ID_SIZEFORMAT_SI_BINARY")))
		return 2;
	else if (GetRCheck(XRCID("ID_SIZEFORMAT_SI_DECIMAL")))
		return 3;

	return 0;
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
	const int format = GetFormat();

	if (!format)
	{
		if (!GetCheck(XRCID("ID_SIZEFORMAT_SEPARATE_THOUTHANDS")))
			return size.ToString();

#ifdef __WXMSW__
		wxChar sep[5];
		int count = ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, sep, 5);
		if (!count)
			return size.ToString();
#else
		char* chr = nl_langinfo(THOUSEP);
		if (!chr || !*chr)
			return size.ToString();
		wxChar sep = chr[0];
#endif

		wxString tmp = size.ToString();
		const int len = tmp.Len();
		if (len <= 3)
			return tmp;

		wxString result;
		int i = (len - 1) % 3 + 1;
		result = tmp.Left(i);
		while (i < len)
		{
			result += sep + tmp.Mid(i, 3);
			i += 3;
		}
		return result;
	}

	const int numplaces = XRCCTRL(*this, "ID_SIZEFORMAT_DECIMALPLACES", wxSpinCtrl)->GetValue();

	wxString places;

	int divider;
	if (format == 3)
		divider = 1000;
	else
		divider = 1024;

	// We always round up. Set to true if there's a reminder
	bool r2 = false;

	// Exponent (2^(10p) or 10^(3p) depending on option
	int p = 0;

	wxLongLong r = size;
	int remainder = 0;
	bool clipped = false;
	while (r > divider && p < 6)
	{
		const wxLongLong rr = r / divider;
		if (remainder != 0)
			clipped = true;
		remainder = (r - rr * divider).GetLo();
		r = rr;
		p++;
	}
	if (!numplaces)
	{
		if (remainder != 0)
			r++;
	}
	else
	{
		if (format != 3)
		{
			// Binary, need to convert 1024 into range from 1-1000
			if (clipped)
			{
				remainder++;
				clipped = false;
			}
			remainder = ceil((double)remainder * 1000 / 1024);
		}

		int max;
		switch (numplaces)
		{
		case 1:
			max = 9;
			divider = 100;
			break;
		case 2:
			max = 99;
			divider = 10;
			break;
		case 3:
			max = 999;
			break;
		}

		if (numplaces != 3)
		{
			if (remainder % divider)
				clipped = true;
			remainder /= divider;
		}

		if (clipped)
			remainder++;
		if (remainder > max)
		{
			r++;
			remainder = 0;
		}

		places.Printf(_T("%d"), remainder);
		const int len = places.Len();
		for (int i = len; i < numplaces; i++)
			places = _T("0") + places;
	}

	wxString result = r.ToString();
	if (places != _T(""))
	{
#ifdef __WXMSW__
		wxChar sep[5];
		int count = ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL , sep, 5);
		if (!count)
		{
			sep[0] = '.';
			sep[1] = 0;
		}
#else
		wxChar sep;
		char* chr = nl_langinfo(RADIXCHAR);
		if (!chr || !*chr)
			sep = 0;
		else
			sep = chr[0];
#endif

		result += sep;
		result += places;
	}
	result += ' ';
	if (!p)
		return result + _T("B");

	// We stop at Exa. If someone has files bigger than that, he can afford to
	// make a donation to have this changed ;)
	wxChar prefix[] = { ' ', 'K', 'M', 'G', 'T', 'P', 'E' };

	result += prefix[p];
	if (format == 1)
		result += 'i';
	result += 'B';

	return result;
}

void COptionsPageSizeFormatting::UpdateExamples()
{
	XRCCTRL(*this, "ID_EXAMPLE1", wxStaticText)->SetLabel(FormatSize(12));
	XRCCTRL(*this, "ID_EXAMPLE2", wxStaticText)->SetLabel(FormatSize(100));
	XRCCTRL(*this, "ID_EXAMPLE3", wxStaticText)->SetLabel(FormatSize(1234));
	XRCCTRL(*this, "ID_EXAMPLE4", wxStaticText)->SetLabel(FormatSize(1058817));
	XRCCTRL(*this, "ID_EXAMPLE5", wxStaticText)->SetLabel(FormatSize(123456789));
	XRCCTRL(*this, "ID_EXAMPLE6", wxStaticText)->SetLabel(FormatSize(63674225613426));

	GetSizer()->Layout();
}