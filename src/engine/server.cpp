#include "FileZilla.h"

CServer::CServer()
{
	Initialize();
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
		else if (protocol == _T("sftp"))
			m_protocol = SFTP;
		else
		{
			error = _("Invalid protocol specified. Valid protocols are ftp and sftp:// at the moment.");
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
			switch (m_protocol)
			{
			case FTP:
			default:
				port = 21;
				break;
			case SFTP:
				port = 22;
				break;
			}
		}
		else if (port > 65535)
		{
			error = _("Invalid port given. The port has to be a value from 1 to 65535.");
			return false;
		}
	}

	host.Trim(true);
	host.Trim(false);

	if (host == _T(""))
	{
		error = _("No host given, please enter a host.");
		return false;
	}

	m_host = host;
	m_port = port;
	m_user = user;
	m_pass = pass;
	m_account = _T("");
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

wxString CServer::GetAccount() const
{
	if (m_logonType != ACCOUNT)
		return _T("");

	return m_account;
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
	m_account = op.m_account;
	m_timezoneOffset = op.m_timezoneOffset;
	m_pasvMode = op.m_pasvMode;
	m_maximumMultipleConnections = op.m_maximumMultipleConnections;
	m_encodingType = op.m_encodingType;
	m_customEncoding = op.m_customEncoding;

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
		else if (m_logonType == NORMAL)
		{
			if (m_pass != op.m_pass)
				return false;
			if (m_account != op.m_account)
				return false;
		}
	}
	else if (m_timezoneOffset != op.m_timezoneOffset)
		return false;
	else if (m_pasvMode != op.m_pasvMode)
		return false;
	else if (m_encodingType != op.m_encodingType)
		return false;
	else if (m_encodingType == ENCODING_CUSTOM)
	{
		if (m_customEncoding != op.m_customEncoding)
			return false;
	}

	// Do not compare number of allowed multiple connections

	return true;
}

bool CServer::operator<(const CServer &op) const
{
	if (m_protocol < op.m_protocol)
		return true;
	else if (m_type < op.m_type)
		return true;
	else if (m_host < op.m_host)
		return true;
	else if (m_port < op.m_port)
		return true;
	else if (m_logonType < op.m_logonType)
		return true;
	else if (m_logonType != ANONYMOUS)
	{
		if (m_user < op.m_user)
			return true;

		if (m_logonType == NORMAL)
		{
			if (m_pass < op.m_pass)
				return true;
		}
		else if (m_logonType == NORMAL)
		{
			if (m_pass < op.m_pass)
				return true;
			if (m_account < op.m_account)
				return true;
		}
	}
	else if (m_timezoneOffset < op.m_timezoneOffset)
		return true;
	else if (m_pasvMode < op.m_pasvMode)
		return true;
	else if (m_encodingType < op.m_encodingType)
		return true;
	else if (m_encodingType == ENCODING_CUSTOM)
	{
		if (m_customEncoding < op.m_customEncoding)
			return true;
	}

	// Do not compare number of allowed multiple connections

	return false;
}

bool CServer::operator!=(const CServer &op) const
{
	return !(*this == op);
}

CServer::CServer(enum ServerProtocol protocol, enum ServerType type, wxString host, unsigned int port, wxString user, wxString pass /*=_T("")*/, wxString account /*=_T("")*/)
{
	Initialize();
	m_protocol = protocol;
	m_type = type;
	m_host = host;
	m_port = port;
	m_logonType = NORMAL;
	m_user = user;
	m_pass = pass;
	m_account = account;
}

CServer::CServer(enum ServerProtocol protocol, enum ServerType type, wxString host, unsigned int port)
{
	Initialize();
	m_protocol = protocol;
	m_type = type;
	m_host = host;
	m_port = port;
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

bool CServer::SetUser(const wxString& user, const wxString& pass /*=_T("")*/)
{
	if (m_logonType == ANONYMOUS)
		return true;

	if (user == _T(""))
		return false;

	m_user = user;
	m_pass = pass;
	
	return true;
}

bool CServer::SetAccount(const wxString& account)
{
	if (m_logonType != ACCOUNT)
		return false;

	m_account = account;

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

void CServer::MaximumMultipleConnections(int maximumMultipleConnections)
{
	m_maximumMultipleConnections = maximumMultipleConnections;
}

int CServer::MaximumMultipleConnections() const
{
	return m_maximumMultipleConnections;
}

wxString CServer::FormatHost() const
{
	wxString host = m_host;
	if (m_port != 21)
		host += wxString::Format(_T(":%d"), m_port);

	return host;
}

wxString CServer::FormatServer() const
{
	wxString server = m_host;
	if (m_port != 21)
		server += wxString::Format(_T(":%d"), m_port);

	if (m_logonType != ANONYMOUS)
		server = GetUser() + _T("@") + server;

	return server;
}

void CServer::Initialize()
{
	m_protocol = FTP;
	m_type = DEFAULT;
	m_host = _T("");
	m_port = 21;
	m_logonType = ANONYMOUS;
	m_user = _T("");
	m_pass = _T("");
	m_account = _T("");
	m_timezoneOffset = 0;
	m_pasvMode = MODE_DEFAULT;
	m_maximumMultipleConnections = 0;
	m_encodingType = ENCODING_AUTO;
	m_customEncoding = _T("");
}

bool CServer::SetEncodingType(enum CharsetEncoding type, const wxString& encoding /*=_T("")*/)
{
	if (type == ENCODING_CUSTOM && encoding == _T(""))
		return false;

	m_encodingType = type;
	m_customEncoding = encoding;

	return true;
}

bool CServer::SetCustomEncoding(const wxString& encoding)
{
	if (encoding == _T(""))
		return false;

	m_encodingType = ENCODING_CUSTOM;
	m_customEncoding = encoding;
	
	return true;
}

enum CharsetEncoding CServer::GetEncodingType() const
{
	return m_encodingType;
}

wxString CServer::GetCustomEncoding() const
{
	return m_customEncoding;
}
