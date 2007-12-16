#ifndef __BUILDINFO_H__
#define __BUILDINFO_H__

class CBuildInfo
{
protected:
	CBuildInfo() { }

public:

	static wxString GetVersion();
	static wxULongLong ConvertToVersionNumber(const wxChar* version);
	static wxString GetBuildDateString();
	static wxString GetBuildTimeString();
	static wxDateTime GetBuildDate();
	static wxString GetBuildType();
	static wxString GetCompiler();
	static wxString GetCompilerFlags();
	static wxString GetHostname();
	static wxString GetBuildSystem();
};

#endif //__BUILDINFO_H__
