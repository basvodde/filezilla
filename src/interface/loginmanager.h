#ifndef __LOGINMANAGER_H__
#define __LOGINMANAGER_H__

// The purpose of this class is to manage some aspects of the login
// behaviour. These are:
// - Password dialog for servers with ASK or INTERACTIVE logontype
// - Storage of passwords for ASK servers for duration of current session
// - Keep track of failed logins for the reconnection delay

class CLoginManager
{
public:
	static CLoginManager& Get() { return m_theLoginManager; }

	bool GetPassword(CServer& server, wxString name = _T(""), wxString challenge = _T(""));
	void CachedPasswordFailed(const CServer& server);

protected:
	static CLoginManager m_theLoginManager;

	// Session password cache for Ask-type servers
	struct t_passwordcache
	{
		wxString host;
		unsigned int port;
		wxString user;
		wxString password;
	};
	std::list<t_passwordcache> m_passwordCache;
};

#endif //__LOGINMANAGER_H__
