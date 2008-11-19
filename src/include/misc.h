#ifndef __MISC_H__
#define __MISC_H__

bool VerifySetDate(wxDateTime& date, int year, wxDateTime::Month month, int day, int hour = 0, int minute = 0, int second = 0);

wxString GetIPV6LongForm(wxString short_address);
bool IsRoutableAddress(const wxString& address, int family);

bool IsIpAddress(const wxString& address);

int GetRandomNumber(int low, int high);

// Under some locales (e.g. Turkish), there is a different
// relationship between the letters a-z and A-Z.
// In Turkish for example there are different types of i 
// (dotted and dotless), with i lowercase dotted and I
// uppercase dotless.
// If needed, use this function to transform the case manually
// and locale-independently
void MakeLowerAscii(wxString& str);

#endif //__MISC_H__
