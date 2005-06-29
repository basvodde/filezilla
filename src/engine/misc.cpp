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
