#ifndef __SERVER_H__
#define __SERVER_H__

enum ServerProtocol
{
	FTP
};

enum ServerType
{
	DEFAULT,
	UNIX,
	VMS,
	DOS,
	MVS
};

enum LogonType
{
	ANONYMOUS,
	NORMAL
};

class CServerPath;
class CServer
{
public:

	// No error checking is done in the constructors
	CServer();
	CServer(wxString host, int port);
	CServer(wxString host, int port, wxString user, wxString pass = _T(""));
	CServer(enum ServerProtocol protocol, enum ServerType type, wxString host, int port);
	CServer(enum ServerProtocol protocol, enum ServerType type, wxString host, int port, wxString user, wxString pass = _T(""));

	void SetType(enum ServerType type);
	
	enum ServerProtocol GetProtocol() const;
	enum ServerType GetType() const;
	wxString GetHost() const;
	int GetPort() const;
	enum LogonType GetLogonType() const;
	wxString GetUser() const;
	wxString GetPass() const;
	

	// Return true if URL could be parsed correctly, false otherwise.
	// If parsing fails, pError is filled with the reason and the CServer instance may be left an undefined state.
	bool ParseUrl(wxString host, int port, wxString user, wxString pass, wxString &error, CServerPath &path);
	
	void SetUser(wxString user, wxString pass = _T(""));

	CServer& operator=(const CServer &op);
	bool operator==(const CServer &op) const;

protected:
	enum ServerProtocol m_Protocol;
	enum ServerType m_Type;
	wxString m_Host;
	int m_Port;
	enum LogonType m_logonType;
	wxString m_User;
	wxString m_Pass;
};

#endif
