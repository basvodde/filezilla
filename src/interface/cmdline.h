#ifndef __CMDLINE_H__
#define __CMDLINE_H__

#include <wx/cmdline.h>

class CCommandLine
{
public:
	enum t_switches
	{
		sitemanager
	};

	enum t_option
	{
		site
	};

	CCommandLine(int argc, wxChar** argv);
	bool Parse();
	void DisplayUsage();

	bool HasSwitch(enum t_switches s) const;
	wxString GetOption(enum t_option option) const;
	wxString GetParameter() const;

protected:
	wxCmdLineParser m_parser;
};

#endif //__CMDLINE_H__
