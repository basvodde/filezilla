#include "FileZilla.h"
#include "cmdline.h"

CCommandLine::CCommandLine(int argc, wxChar** argv)
{
	m_parser.AddSwitch(_T("h"), _T("help"), _("Shows this help dialog"), wxCMD_LINE_OPTION_HELP);
	m_parser.AddSwitch(_T("s"), _T("site-manager"), _("Start with opened Site Manager"));
	m_parser.AddOption(_T("c"), _T("site"), _("Connect to specified Site Manager site"));
#ifdef __WXMSW__
	m_parser.AddSwitch(_T(""), _T("close"), _("Close all running instances of FileZilla"));
#endif
	m_parser.AddSwitch(_T(""), _T("verbose"), _("Verbose log messages from wxWidgets"));
	wxString str = _T("<");
	str += _("FTP URL");
	str += _T(">");
	m_parser.AddParam(str, wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	m_parser.SetCmdLine(argc, argv);

	m_parser.SetSwitchChars(_T("-"));
}

bool CCommandLine::HasSwitch(enum CCommandLine::t_switches s) const
{
	if (s == sitemanager)
		return m_parser.Found(_T("s"));
#ifdef __WXMSW__
	else if (s == close)
		return m_parser.Found(_T("close"));
#endif

	return false;
}

wxString CCommandLine::GetOption(enum CCommandLine::t_option option) const
{
	wxString value;
	switch (option)
	{
	case site:
		if (m_parser.Found(_T("c"), &value))
			return value;
		break;
	}

	return _T("");
}

bool CCommandLine::Parse()
{
	int res = m_parser.Parse(false);
	if (res != 0)
		return false;
		
	if (HasSwitch(sitemanager) && GetOption(site) != _T(""))
	{
		wxMessageBox(_("-s and -c cannot be present at the same time."), _("Syntax error in command line"));
		return false;
	}

	if (HasSwitch(sitemanager) && m_parser.GetParamCount())
	{
		wxMessageBox(_("-s cannot be used together with an FTP URL."), _("Syntax error in command line"));
		return false;
	}

	if (GetOption(site) != _T("") && m_parser.GetParamCount())
	{
		wxMessageBox(_("-c cannot be used together with an FTP URL."), _("Syntax error in command line"));
		return false;
	}

	if (m_parser.Found(_T("verbose")))
		wxLog::SetVerbose(true);

	return true;
}

void CCommandLine::DisplayUsage()
{
	m_parser.Usage();
}

wxString CCommandLine::GetParameter() const
{
	if (!m_parser.GetParamCount())
		return _T("");

	return m_parser.GetParam();
}
