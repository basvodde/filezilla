#ifndef __MISC_H__
#define __MISC_H__

#include "socket.h"

bool VerifySetDate(wxDateTime& date, int year, wxDateTime::Month month, int day, int hour = 0, int minute = 0, int second = 0);

// Also verifies that it is a correct IPv6 address
wxString GetIPV6LongForm(wxString short_address);

int DigitHexToDecNum(wxChar c);

bool IsRoutableAddress(const wxString& address, enum CSocket::address_family family);

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

enum _dependency
{
	dependency_gnutls
};

wxString GetDependencyVersion(enum _dependency dependency);

#endif //__MISC_H__
