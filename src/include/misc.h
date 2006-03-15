#ifndef __MISC_H__
#define __MISC_H__

bool VerifySetDate(wxDateTime& date, int year, wxDateTime::Month month, int day, int hour = 0, int minute = 0, int second = 0);

bool IsRoutableAddress(const wxString& address);

#endif //__MISC_H__
