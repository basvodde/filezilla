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
	DOS
};

class CServerPath;
class CServer
{
public:

	// No error checking is done in the constructor
	CServer();
	CServer(wxString host, int port);
	CServer(wxString host, int port, wxString user, wxString pass = _T(""));
	CServer(enum ServerProtocol protocol, wxString host, int port);
	CServer(enum ServerProtocol protocol, wxString host, int port, wxString user, wxString pass = _T(""));

	ServerProtocol GetProtocol() const;
	ServerType GetType() const;
	wxString GetHost() const;
	int GetPort() const;
	wxString GetUser() const;
	wxString GetPass() const;
	

	// Return true if URL could be parsed correctly, false otherwise.
	// If parsing fails, pError is filled with the reason and the CServer instance may be left an undefined state.
	bool ParseUrl(wxString host, int port, wxString user, wxString pass, wxString &error, CServerPath &path);
	
	void SetUser(wxString user, wxString pass = _T(""));

	CServer& operator=(const CServer &op);

protected:
	ServerProtocol m_Protocol;
	ServerType m_Type;
	wxString m_Host;
	int m_Port;
	wxString m_User;
	wxString m_Pass;
};

#endif
