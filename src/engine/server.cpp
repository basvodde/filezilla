#include "FileZilla.h"

CServer::CServer()
{
	m_port = 21;
	m_protocol = FTP;
	m_type = DEFAULT;
	m_logonType = ANONYMOUS;
	m_timezoneOffset = 0;
	m_allowMultipleConnections = true;
	m_pasvMode = MODE_DEFAULT;
}

bool CServer::ParseUrl(wxString host, unsigned int port, wxString user, wxString pass, wxString &error, CServerPath &path)
{
	m_type = DEFAULT;

	if (host == _T(""))
	{
		error = _("No host given, please enter a host.");
		return false;
	}

	int pos = host.Find(_T("://"));
	if (pos != -1)
	{
		wxString protocol = host.Left(pos).Lower();
		host = host.Mid(pos + 3);
		if (protocol.Left(3) == _T("fz_"))
			protocol = protocol.Mid(3);
		if (protocol == _T("ftp"))
			m_protocol = FTP;
		else
		{
			error = _("Invalid protocol specified. Only valid protocol is ftp:// at the moment.");
			return false;
		}
	}
	else
		m_protocol = FTP;

	pos = host.Find('@');
	if (pos != -1)
	{
		user = host.Left(pos);
		host = host.Mid(pos + 1);

		pos = user.Find(':');
		if (pos != -1)
		{
			pass = user.Mid(pos + 1);
			user = user.Left(pos);
		}

		if (user == _T(""))
		{
			error = _("Invalid username given.");
			return false;
		}
	}
	else
	{
		if (user == _T(""))
		{
			user = _T("anonymous");
			pass = _T("anonymous@example.com");
		}
	}

	pos = host.Find('/');
	if (pos != -1)
	{
		path = CServerPath(host.Mid(pos));
		host = host.Left(pos);
	}

	pos = host.Find(':');
	if (pos != -1)
	{
		if (!pos)
		{
			error = _("No host given, please enter a host.");
			return false;
		}

		long tmp;
		if (!host.Mid(pos + 1).ToLong(&tmp) || tmp < 1 || tmp > 65535)
		{
			error = _("Invalid port given. The port has to be a value from 1 to 65535.");
			return false;
		}
		port = tmp;

		host = host.Left(pos);
	}
	else
	{
		if (!port)
		{
			if (m_protocol == FTP)
				port = 21;
			else
				port = 21;
		}
		else if (port > 65535)
		{
			error = _("Invalid port given. The port has to be a value from 1 to 65535.");
			return false;
		}
	}

	if (host == _T(""))
	{
		error = _("No host given, please enter a host.");
		return false;
	}

	m_host = host;
	m_port = port;
	m_user = user;
	m_pass = pass;
	if (m_user == _T("") || m_user == _T("anonymous"))
		m_logonType = ANONYMOUS;
	else
		m_logonType = NORMAL;

	return true;
}

ServerProtocol CServer::GetProtocol() const
{
	return m_protocol;
}

ServerType CServer::GetType() const
{
	return m_type;
}

wxString CServer::GetHost() const
{
	return m_host;
}

unsigned int CServer::GetPort() const
{
	return m_port;
}

wxString CServer::GetUser() const
{
	if (m_logonType == ANONYMOUS)
		return _T("anonymous");
	
	return m_user;
}

wxString CServer::GetPass() const
{
	if (m_logonType == ANONYMOUS)
		return _T("anon@localhost");
	
	return m_pass;
}

CServer& CServer::operator=(const CServer &op)
{
	m_protocol = op.m_protocol;
	m_type = op.m_type;
	m_host = op.m_host;
	m_port = op.m_port;
	m_logonType = op.m_logonType;
	m_user = op.m_user;
	m_pass = op.m_pass;
	m_timezoneOffset = op.m_timezoneOffset;
	m_pasvMode = op.m_pasvMode;
	m_allowMultipleConnections = op.m_allowMultipleConnections;

	return *this;
}

bool CServer::operator==(const CServer &op) const
{
	if (m_protocol != op.m_protocol)
		return false;
	else if (m_type != op.m_type)
		return false;
	else if (m_host != op.m_host)
		return false;
	else if (m_port != op.m_port)
		return false;
	else if (m_logonType != op.m_logonType)
		return false;
	else if (m_logonType != ANONYMOUS)
	{
		if (m_user != op.m_user)
			return false;

		if (m_logonType == NORMAL)
		{
			if (m_pass != op.m_pass)
				return false;
		}
	}
	else if (m_timezoneOffset != op.m_timezoneOffset)
		return false;
	else if (m_pasvMode != op.m_pasvMode)
		return false;
	else if (m_allowMultipleConnections != op.m_allowMultipleConnections)
		return false;

	return true;
}

bool CServer::operator!=(const CServer &op) const
{
	return !(*this == op);
}

CServer::CServer(enum ServerProtocol protocol, enum ServerType type, wxString host, unsigned int port, wxString user, wxString pass /*=_T("")*/)
{
	m_protocol = protocol;
	m_type = type;
	m_host = host;
	m_port = port;
	m_logonType = NORMAL;
	m_user = user;
	m_pass = pass;
}

CServer::CServer(enum ServerProtocol protocol, enum ServerType type, wxString host, unsigned int port)
{
	m_protocol = protocol;
	m_type = type;
	m_host = host;
	m_port = port;
	m_logonType = ANONYMOUS;
}

void CServer::SetType(enum ServerType type)
{
	m_type = type;
}

enum LogonType CServer::GetLogonType() const
{
	return m_logonType;
}

void CServer::SetLogonType(enum LogonType logonType)
{
	m_logonType = logonType;
}

void CServer::SetProtocol(enum ServerProtocol serverProtocol)
{
	m_protocol = serverProtocol;
}

bool CServer::SetHost(wxString host, unsigned int port)
{
	if (host == _T(""))
		return false;

	if (port < 1 || port > 65535)
		return false;

	m_host = host;
	m_port = port;
	
	return true;
}

bool CServer::SetUser(wxString user, wxString pass /*=_T("")*/)
{
	if (m_logonType == ANONYMOUS)
		return true;

	if (user == _T(""))
		return false;

	m_user = user;
	m_pass = pass;
	
	return true;
}

bool CServer::SetTimezoneOffset(int minutes)
{
	if (minutes > (60 * 13) || minutes < (-60 * 30))
		return false;

	m_timezoneOffset = minutes;

	return true;
}

int CServer::GetTimezoneOffset() const
{
	return m_timezoneOffset;
}

enum PasvMode CServer::GetPasvMode() const
{
	return m_pasvMode;
}

void CServer::SetPasvMode(enum PasvMode pasvMode)
{
	m_pasvMode = pasvMode;
}

void CServer::AllowMultipleConnections(bool allow)
{
	m_allowMultipleConnections = allow;
}

bool CServer::AllowMultipleConnections() const
{
	return m_allowMultipleConnections;
}

wxString CServer::FormatHost() const
{
	wxString host = m_host;
	if (m_port != 21)
		host += wxString::Format(_T(":%d"), m_port);

	return host;
}
