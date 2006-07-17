#include "FileZilla.h"
#include "buildinfo.h"

wxString CBuildInfo::GetVersion()
{
	return wxString(PACKAGE_VERSION, wxConvLocal);
}

wxString CBuildInfo::GetBuildDate()
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
	wxString buildtype = (BUILDTYPE, wxConvLocal);
	if (buildtype == _T("official") || buildtype == _T("nightly"))
		return buildtype;
#endif
	return _T("");
}
