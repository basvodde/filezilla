#include <wx/defs.h>
#ifdef __WXMSW__
// For AF_INET6
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "FileZilla.h"
#ifndef __WXMSW__
#include <sys/socket.h>
#endif

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

wxString GetIPV6LongForm(wxString short_address)
{
	if (short_address[0] == '[')
	{
		if (short_address.Last() != ']')
			return _T("");
		short_address.RemoveLast();
		short_address = short_address.Mid(1);
	}
	short_address.MakeLower();

	wxChar buffer[40] = { '0', '0', '0', '0', ':',
						  '0', '0', '0', '0', ':',
						  '0', '0', '0', '0', ':',
						  '0', '0', '0', '0', ':',
						  '0', '0', '0', '0', ':',
						  '0', '0', '0', '0', ':',
						  '0', '0', '0', '0', ':',
						  '0', '0', '0', '0', 0
						};
	wxChar* out = buffer;

	const unsigned int len = short_address.Len();
	if (len > 39)
		return _T("");

	// First part, before possible ::
	unsigned int i = 0;
	unsigned int grouplength = 0;
	for (i = 0; i < len + 1; i++)
	{
		const wxChar& c = short_address[i];
		if (c == ':' || !c)
		{
			if (!grouplength)
			{
				// Empty group length, not valid
				if (!c || short_address[i + 1] != ':')
					return _T("");
				i++;
				break;
			}

			out += 4 - grouplength;
			for (unsigned int j = grouplength; j > 0; j--)
				*out++ = short_address[i - j];
			// End of string...
			if (!c)
			{
				if (!*out)
					// ...on time
					return buffer;
				else
					// ...premature
					return _T("");
			}
			else if (!*out)
			{
				// Too long
				return _T("");
			}

			out++;

			grouplength = 0;
			if (short_address[i + 1] == ':')
			{
				i++;
				break;
			}
			continue;
		}
		else if ((c < '0' || c > '9') &&
				 (c < 'a' || c > 'f'))
		{
			// Invalid character
			return _T("");
		}
		// Too long group
		if (++grouplength > 4)
			return _T("");
	}

	// Second half after ::

	wxChar* end_first = out;
	out = &buffer[38];
	unsigned int stop = i;
	for (i = len - 1; i > stop; i--)
	{
		if (out < end_first)
		{
			// Too long
			return _T("");
		}

		const wxChar& c = short_address[i];
		if (c == ':')
		{
			if (!grouplength)
			{
				// Empty group length, not valid
				return _T("");
			}

			out -= 5 - grouplength;

			grouplength = 0;
			continue;
		}
		else if ((c < '0' || c > '9') &&
				 (c < 'a' || c > 'f'))
		{
			// Invalid character
			return _T("");
		}
		// Too long group
		if (++grouplength > 4)
			return _T("");
		*out-- = c;
	}
	if (!grouplength)
	{
		// Empty group length, not valid
		return _T("");
	}
	out -= 5 - grouplength;
	out += 2;

	int diff = out - end_first;
	if (diff < 0 || diff % 5)
		return _T("");

	return buffer;
}

static int DigitHexToDecNum(wxChar c)
{
	if (c >= 'a')
		return c - 'a' + 10;
	else
		return c - '0';
}

bool IsRoutableAddress(const wxString& address, int family)
{
	if (family == AF_INET6)
	{
		wxString long_address = GetIPV6LongForm(address);
		printf("\n--- %s ---\n", (const char*)long_address.mb_str());
		if (long_address.empty())
			return false;
		if (long_address[0] == '0')
		{
			// ::/128
			if (long_address == _T("0000:0000:0000:0000:0000:0000:0000:0000"))
				return false;
			// ::1/128
			if (long_address == _T("0000:0000:0000:0000:0000:0000:0000:0001"))
				return false;

			if (long_address.Left(30) == _T("0000:0000:0000:0000:0000:ffff:"))
			{
				// IPv4 mapped
				wxString ipv4 = wxString::Format(_T("%d.%d.%d.%d"),
						DigitHexToDecNum(long_address[30]) * 16 + DigitHexToDecNum(long_address[31]),
						DigitHexToDecNum(long_address[32]) * 16 + DigitHexToDecNum(long_address[33]),
						DigitHexToDecNum(long_address[35]) * 16 + DigitHexToDecNum(long_address[36]),
						DigitHexToDecNum(long_address[37]) * 16 + DigitHexToDecNum(long_address[38]));
				printf("\n--- %s ---\n", (const char*)ipv4.mb_str());
				return IsRoutableAddress(ipv4, AF_INET);
			}

			return true;
		}
		if (long_address[0] == 'f')
		{
			if (long_address[1] == 'e')
			{
				// fe80::/10 (link local)
				const wxChar& c = long_address[2];
				int v;
				if (c >= 'a')
					v = c - 'a' + 10;
				else
					v = c - '0';
				if ((v & 0xc) == 0x8)
					return false;

				return true;
			}
			else if (long_address[1] == 'c' || long_address[1] == 'd')
			{
				// fc00::/7 (site local)
				return false;
			}
		}

		return true;
	}
	else
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
}

bool IsIpAddress(const wxString& address)
{
	if (GetIPV6LongForm(address) != _T(""))
		return true;

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

// RAND_MAX is different depending on platform.
// For example on MSW, it is just 0x7fff, not even enough to cover
// a full range of tcp ports.
// On Linux it is identical to INT_MAX.
//
// The following code can generate random numbers using rand() for any
// RAND_MAX.
int GetRandomNumber(int low, int high)
{
	wxASSERT(low <= high);
	if (low == high)
		return low;

	const int range = high - low;
	if (RAND_MAX < range)
	{
		// Calculate number of random bits rand() returns
		int r = RAND_MAX;
		unsigned int random_bits = 0;
		bool zero = false;
		while (r)
		{
			if (!r & 1)
				zero = true;
			r >>= 1;
			random_bits++;
		}
		if (zero)
			random_bits--;

		// Calculate number of bits in the range
		unsigned int maxshift = 0;
		r = range;
		while (r)
		{
			r >>= 1;
			maxshift++;
		}

		unsigned int s = 0;
		do
		{
			unsigned int shift_remaining = maxshift;
			while (shift_remaining)
			{
				int m;
				if (shift_remaining >= random_bits)
				{
					m = (1 << random_bits) - 1;
					s <<= random_bits;
					s += (unsigned int)GetRandomNumber(0, m);
					shift_remaining -= random_bits;
				}
				else
				{
					m = (1 << shift_remaining) - 1;
					s <<= shift_remaining;
					s += (unsigned int)GetRandomNumber(0, m);
					shift_remaining = 0;
				}
			}
		} while (s > (unsigned int)range);

		return s;
	}
	else if (range == RAND_MAX)
		return low + rand();
	else
	{
		const unsigned int max = ((((unsigned int)RAND_MAX) + 1) / (range + 1)) * (range + 1);
		unsigned int r;
		do {
			r = (unsigned int)rand();
		} while (r >= max);
		return (int)((r % (range + 1)) + low);
	}
}
