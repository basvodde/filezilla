#ifndef __MISC_H__
#define __MISC_H__

bool VerifySetDate(wxDateTime& date, int year, wxDateTime::Month month, int day, int hour = 0, int minute = 0, int second = 0);

wxString GetIPV6LongForm(wxString short_address);
bool IsRoutableAddress(const wxString& address, int family);

bool IsIpAddress(const wxString& address);

int GetRandomNumber(int low, int high);

#endif //__MISC_H__
