#ifndef __SERVERCAPABILITIES_H__
#define __SERVERCAPABILITIES_H__

enum capabilities
{
	unknown,
	yes,
	no
};

enum capabilityNames
{
	resume2GBbug,
	resume4GBbug,

	// FTP-protocol specific
	syst_command, // reply of SYST command as option
	feat_command,
	clnt_command, // set to 'yes' if CLNT should be sent
	utf8_command, // set to yes if OPTS UTF8 ON should be sent
	mlsd_command,
	mfmt_command,
	pret_command,
	mdtm_command,
	size_command,
	mode_z_support,
	list_hidden_support, // LIST -a command

	// Server timezone offset. If using FTP, LIST details are unspecified and
	// can return different times than the UTC based times using the MLST or
	// MDTM commands.
	// Not that the user can invoke an additional timezone offset on top of
	// this for server not supporting auto-detection or to compensate 
	// unsynchronized clocks.
	timezone_offset
};

class CCapabilities
{
public:
	enum capabilities GetCapability(enum capabilityNames name, wxString* pOption = 0) const;
	enum capabilities GetCapability(enum capabilityNames name, int* pOption) const;
	void SetCapability(enum capabilityNames name, enum capabilities cap, const wxString& option = _T(""));
	void SetCapability(enum capabilityNames name, enum capabilities cap, int option);

protected:
	struct t_cap
	{
		enum capabilities cap;
		wxString option;
		int number;
	};
	std::map<enum capabilityNames, struct t_cap> m_capabilityMap;
};

class CServerCapabilities
{
public:
	// If return value isn't 'yes', pOptions remains unchanged
	static enum capabilities GetCapability(const CServer& server, enum capabilityNames name, wxString* pOption = 0);
	static enum capabilities GetCapability(const CServer& server, enum capabilityNames name, int* option);

	static void SetCapability(const CServer& server, enum capabilityNames name, enum capabilities cap, const wxString& option = _T(""));
	static void SetCapability(const CServer& server, enum capabilityNames name, enum capabilities cap, int option);

protected:
	static std::map<CServer, CCapabilities> m_serverMap;
};

#endif //__SERVERCAPABILITIES_H__
