#ifndef __SERVER_H__
#define __SERVER_H__

enum ServerProtocol
{
	FTP
};

class CServer
{
public:
	CServer();
	CServer(wxString host, int port);
	CServer(wxString host, int port, wxString user, wxString pass = "");
	CServer(enum ServerProtocol protocol, wxString host, int port);
	CServer(enum ServerProtocol protocol, wxString host, int port, wxString user, wxString pass = "");

	void SetUser(wxString user, wxString pass = "");

	CServer& operator=(const CServer &op);

protected:
	int m_Protocol;
	wxString m_Host;
	int m_Port;
	wxString m_User;
	wxString m_Pass;
};

#endif
