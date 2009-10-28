#ifndef __BUILDINFO_H__
#define __BUILDINFO_H__

class CBuildInfo
{
protected:
	CBuildInfo() { }

public:

	static wxString GetVersion();
	static wxLongLong ConvertToVersionNumber(const wxChar* version);
	static wxString GetBuildDateString();
	static wxString GetBuildTimeString();
	static wxDateTime GetBuildDate();
	static wxString GetBuildType();
	static wxString GetCompiler();
	static wxString GetCompilerFlags();
	static wxString GetHostname();
	static wxString GetBuildSystem();
	static bool IsUnstable(); // Returns true on beta or rc releases.
};

#endif //__BUILDINFO_H__
