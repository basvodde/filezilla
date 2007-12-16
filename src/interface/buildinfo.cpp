#include "FileZilla.h"
#include "buildinfo.h"

wxString CBuildInfo::GetVersion()
{
	return wxString(PACKAGE_VERSION, wxConvLocal);
}

wxString CBuildInfo::GetBuildDateString()
{
	// Get build date. Unfortunately it is in the ugly Mmm dd yyyy format.
	// Make a good yyyy-mm-dd out of it
	wxString date(__DATE__, wxConvLocal);
	while (date.Replace(_T("  "), _T(" ")));

	const wxChar months[][4] = { _T("Jan"), _T("Feb"), _T("Mar"),
								_T("Apr"), _T("May"), _T("Jun"), 
								_T("Jul"), _T("Aug"), _T("Sep"),
								_T("Oct"), _T("Nov"), _T("Dev") };

	int pos = date.Find(_T(" "));
	if (pos == -1)
		return date;

	wxString month = date.Left(pos);
	int i = 0;
	for (i = 0; i < 12; i++)
	{
		if (months[i] == month)
			break;
	}
	if (i == 12)
		return date;

	wxString tmp = date.Mid(pos + 1);
	pos = tmp.Find(_T(" "));
	if (pos == -1)
		return date;

	long day;
	if (!tmp.Left(pos).ToLong(&day))
		return date;

	long year;
	if (!tmp.Mid(pos + 1).ToLong(&year))
		return date;

	return wxString::Format(_T("%04d-%02d-%02d"), year, i + 1, day);
}

wxString CBuildInfo::GetBuildTimeString()
{
	return wxString(__TIME__, wxConvLocal);
}

wxDateTime CBuildInfo::GetBuildDate()
{
	wxDateTime date;
	date.ParseDate(GetBuildDateString());
	return date;
}

wxString CBuildInfo::GetCompiler()
{
#ifdef USED_COMPILER
	return wxString(USED_COMPILER, wxConvLocal);
#elif defined __VISUALC__
	int version = __VISUALC__;
	return wxString::Format(_T("Visual C++ %d"), version);
#else
	return _T("Unknown compiler");
#endif
}

wxString CBuildInfo::GetCompilerFlags()
{
#ifndef USED_CXXFLAGS
	return _T("");
#else
	wxString flags(USED_CXXFLAGS, wxConvLocal);
	return flags;
#endif
}

wxString CBuildInfo::GetBuildType()
{
#ifdef BUILDTYPE
	wxString buildtype(BUILDTYPE, wxConvLocal);
	if (buildtype == _T("official") || buildtype == _T("nightly"))
		return buildtype;
#endif
	return _T("");
}

wxULongLong CBuildInfo::ConvertToVersionNumber(const wxChar* version)
{
	// Crude conversion from version string into number for easy comparison
	// Supported version formats:
	// 1.2.4
	// 11.22.33.44
	// 1.2.3-rc3
	// 1.2.3.4-beta5
	// All numbers can be as large as 1024, with the exception of the release candidate.

	// Only either rc or beta can exist at the same time)
	//
	// The version string A.B.C.D-rcE-betaF expands to the following binary representation:
	// 0000aaaaaaaaaabbbbbbbbbbccccccccccddddddddddxeeeeeeeeeffffffffff
	//
	// x will be set to 1 if neither rc nor beta are set. 0 otherwise.
	//
	// Example:
	// 2.2.26-beta3 will be converted into
	// 0000000010 0000000010 0000011010 0000000000 0000000000 0000000011
	// which in turn corresponds to the simple 64-bit number 2254026754228227
	// And these can be compared easily

	wxASSERT(*version >= '0' && *version <= '9');

	wxULongLong v = 0;
	int segment = 0;

	int shifts = 0;

	for (; *version; version++)
	{
		if (*version == '.' || *version == '-' || *version == 'b')
		{
			v += segment;
			segment = 0;
			v <<= 10;
			shifts++;
		}
		if (*version == '-' && shifts < 4)
		{
			v <<= (4 - shifts) * 10;
			shifts = 4;
		}
		else if (*version >= '0' && *version <= '9')
		{
			segment *= 10;
			segment += *version - '0';
		}
	}
	v += segment;
	v <<= (5 - shifts) * 10;

	// Make sure final releases have a higher version number than rc or beta releases
	if ((v & 0x0FFFFF) == 0)
		v |= 0x080000;

	return v;
}

wxString CBuildInfo::GetHostname()
{
#ifndef USED_HOST
	return _T("");
#else
	wxString flags(USED_HOST, wxConvLocal);
	return flags;
#endif
}

wxString CBuildInfo::GetBuildSystem()
{
#ifndef USED_BUILD
	return _T("");
#else
	wxString flags(USED_BUILD, wxConvLocal);
	return flags;
#endif
}
