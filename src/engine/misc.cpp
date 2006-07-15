#include "FileZilla.h"

bool VerifySetDate(wxDateTime& date, int year, wxDateTime::Month month, int day, int hour /*=0*/, int minute /*=0*/, int second /*=0*/)
{
	if (year < 1900 || year > 3000)
		return false;

	if (month < 0 || month >= 12)
		return false;

	int maxDays = wxDateTime::GetNumberOfDays(month, year);
	if (day > maxDays)
		return false;

	date.Set(day, month, year, hour, minute, second);

	return date.IsValid();
}

bool IsRoutableAddress(const wxString& address)
{
	// Assumes address is already a valid IP address
	if (address.Left(3) == _T("127") ||
		address.Left(3) == _T("10.") ||
		address.Left(7) == _T("192.168") ||
		address.Left(7) == _T("169.254"))
		return false;

	if (address.Left(3) == _T("172"))
	{
		wxString middle = address.Mid(4);
		int pos = address.Find(_T("."));
		wxASSERT(pos != -1);
		long part;
		middle.Left(pos).ToLong(&part);

		if (part >= 16 && part <= 31)
			return false;
	}

	return true;
}

bool IsIpAddress(const wxString& address)
{
	int segment = 0;
	int dotcount = 0;
	for (size_t i = 0; i < address.Len(); i++)
	{
		const char& c = address[i];
		if (c == '.')
		{
			if (address[i + 1] == '.')
				// Disallow multiple dots in a row
				return false;

			if (segment > 255)
				return false;
			if (!dotcount && !segment)
				return false;
			dotcount++;
			segment = 0;
		}
		else if (c < '0' || c > '9')
			return false;

		segment = segment * 10 + c - '0';
	}
	if (dotcount != 3)
		return false;

	if (segment > 255)
		return false;

	return true;
}
