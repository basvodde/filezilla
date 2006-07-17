#ifndef __BUILDINFO_H__
#define __BUILDINFO_H__

class CBuildInfo
{
protected:
	CBuildInfo() { }

public:

	static wxString GetVersion();
	static wxString GetBuildDate();
	static wxString GetBuildType();
	static wxString GetCompiler();
	static wxString GetCompilerFlags();
};

#endif //__BUILDINFO_H__
