#ifndef __SERVER_H__
#define __SERVER_H__

enum ServerProtocol
{
	UNKNOWN = -1,
	FTP,
	SFTP,
	HTTP,
	FTPS, // Implicit SSL
	FTPES // Explicit SSL
};

enum ServerType
{
	DEFAULT,
	UNIX,
	VMS,
	DOS,
	MVS,
	VXWORKS
};

enum LogonType
{
	ANONYMOUS,
	NORMAL,
	ASK, // ASK should not be sent to the engine, it's intendet to be used by the interface
	INTERACTIVE,
	ACCOUNT
};

enum PasvMode
{
	MODE_DEFAULT,
	MODE_ACTIVE,
	MODE_PASSIVE
};

enum CharsetEncoding
{
	ENCODING_AUTO,
	ENCODING_UTF8,
	ENCODING_CUSTOM
};

class CServerPath;
class CServer
{
public:

	// No error checking is done in the constructors
	CServer();
	CServer(wxString host, unsigned int);
	CServer(wxString host, unsigned int, wxString user, wxString pass = _T(""));
	CServer(enum ServerProtocol protocol, enum ServerType type, wxString host, unsigned int);
	CServer(enum ServerProtocol protocol, enum ServerType type, wxString host, unsigned int, wxString user, wxString pass = _T(""), wxString account = _T(""));

	void SetType(enum ServerType type);
	
	enum ServerProtocol GetProtocol() const;
	enum ServerType GetType() const;
	wxString GetHost() const;
	unsigned int GetPort() const;
	enum LogonType GetLogonType() const;
	wxString GetUser() const;
	wxString GetPass() const;
	wxString GetAccount() const;
	int GetTimezoneOffset() const;
	enum PasvMode GetPasvMode() const;
	int MaximumMultipleConnections() const;
	
	// Return true if URL could be parsed correctly, false otherwise.
	// If parsing fails, pError is filled with the reason and the CServer instance may be left an undefined state.
	bool ParseUrl(wxString host, unsigned int, wxString user, wxString pass, wxString &error, CServerPath &path);
	
	void SetProtocol(enum ServerProtocol serverProtocol);
	bool SetHost(wxString Host, unsigned int);

	void SetLogonType(enum LogonType logonType);
	bool SetUser(const wxString& user, const wxString& pass = _T(""));
	bool SetAccount(const wxString& account);

	CServer& operator=(const CServer &op);
	bool operator==(const CServer &op) const;
	bool operator<(const CServer &op) const;
	bool operator!=(const CServer &op) const;

	bool SetTimezoneOffset(int minutes);	
	void SetPasvMode(enum PasvMode pasvMode);
	void MaximumMultipleConnections(int maximum);

	wxString FormatHost() const;
	wxString FormatServer() const;

	bool SetEncodingType(enum CharsetEncoding type, const wxString& encoding = _T(""));
	bool SetCustomEncoding(const wxString& encoding);
	enum CharsetEncoding GetEncodingType() const;
	wxString GetCustomEncoding() const;

	static unsigned int GetDefaultPort(enum ServerProtocol protocol);
	static enum ServerProtocol GetProtocolFromPort(unsigned int port);

	static wxString GetProtocolName(enum ServerProtocol protocol);
	static enum ServerProtocol GetProtocolFromName(const wxString& name);

	// These commands will be executed after a successful login.
	const std::vector<wxString>& GetPostLoginCommands() const { return m_postLoginCommands; }
	bool SetPostLoginCommands(const std::vector<wxString>& postLoginCommands);

protected:
	void Initialize();

	enum ServerProtocol m_protocol;
	enum ServerType m_type;
	wxString m_host;
	unsigned int m_port;
	enum LogonType m_logonType;
	wxString m_user;
	wxString m_pass;
	wxString m_account;
	int m_timezoneOffset;
	enum PasvMode m_pasvMode;
	int m_maximumMultipleConnections;
	enum CharsetEncoding m_encodingType;
	wxString m_customEncoding;

	std::vector<wxString> m_postLoginCommands;
};

#endif
