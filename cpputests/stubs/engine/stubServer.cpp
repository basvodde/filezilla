
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "server.h"

CServer::CServer()
{
	FAIL("CServer::CServer");
}

CServer::CServer(enum ServerProtocol protocol, enum ServerType type, wxString host, unsigned int)
{
	FAIL("CServer::~CServer");
}

bool CServer::EqualsNoPass(const CServer &op) const
{
	FAIL("CServer::EqualsNoPass");
	return true;
}

wxString CServer::FormatHost(bool always_omit_port) const
{
	FAIL("CServer::FormatHost");
	return L"";
}

wxString CServer::FormatServer(const bool always_include_prefix) const
{
	FAIL("CServer::FormatServer");
	return L"";
}

wxString CServer::GetAccount() const
{
	FAIL("CServer::GetAccount");
	return L"";
}

bool CServer::GetBypassProxy() const
{
	FAIL("CServer::GetBypassProxy");
	return true;
}

wxString CServer::GetCustomEncoding() const
{
	FAIL("CServer::GetCustomEncoding");
	return L"";
}

unsigned int CServer::GetDefaultPort(enum ServerProtocol protocol)
{
	FAIL("CServer::GetDefaultPort");
	return 0;
}

enum CharsetEncoding CServer::GetEncodingType() const
{
	FAIL("CServer::GetEncodingType");
	return ENCODING_AUTO;
}

wxString CServer::GetHost() const
{
	FAIL("CServer::GetHost");
	return L"";
}

enum LogonType CServer::GetLogonType() const
{
	FAIL("CServer::GetLogonType");
	return NORMAL;
}

enum LogonType CServer::GetLogonTypeFromName(const wxString& name)
{
	FAIL("CServer::GetLogonTypeFromName");
	return NORMAL;
}

wxString CServer::GetNameFromLogonType(enum LogonType type)
{
	FAIL("CServer::GetNameFromLogonType");
	return L"";
}

wxString CServer::GetNameFromServerType(enum ServerType type)
{
	FAIL("CServer::GetNameFromServerType");
	return L"";
}

wxString CServer::GetPass() const
{
	FAIL("CServer::GetPass");
	return L"";
}

enum PasvMode CServer::GetPasvMode() const
{
	FAIL("CServer::GetPasvMode");
	return MODE_DEFAULT;
}

unsigned int CServer::GetPort() const
{
	FAIL("CServer::GetPort");
	return 0;
}

wxString CServer::GetPrefixFromProtocol(const enum ServerProtocol protocol)
{
	FAIL("CServer::GetPrefixFromProtocol");
	return L"";
}

enum ServerProtocol CServer::GetProtocol() const
{
	FAIL("CServer::GetProtocol");
	return FTP;
}

enum ServerProtocol CServer::GetProtocolFromName(const wxString& name)
{
	FAIL("CServer::GetProtocolFromName");
	return FTP;
}

enum ServerProtocol CServer::GetProtocolFromPort(unsigned int port, bool defaultOnly)
{
	FAIL("CServer::GetProtocolFromPort");
	return FTP;
}

wxString CServer::GetProtocolName(enum ServerProtocol protocol)
{
	FAIL("CServer::GetProtocolName");
	return L"";
}

enum ServerType CServer::GetServerTypeFromName(const wxString& name)
{
	FAIL("CServer::GetServerTypeFromName");
	return DEFAULT;
}

int CServer::GetTimezoneOffset() const
{
	FAIL("CServer::GetTimezoneOffset");
	return 0;
}

enum ServerType CServer::GetType() const
{
	FAIL("CServer::GetType");
	return DEFAULT;
}

wxString CServer::GetUser() const
{
	FAIL("CServer::GetUser");
	return L"";
}

int CServer::MaximumMultipleConnections() const
{
	FAIL("CServer::MaximumMultipleConnections");
	return 0;
}

void CServer::MaximumMultipleConnections(int maximum)
{
	FAIL("CServer::MaximumMultipleConnections");
}

bool CServer::ParseUrl(wxString host, unsigned int port, wxString user, wxString pass, wxString &error, CServerPath &path)
{
	FAIL("CServer::ParseUrl");
	return true;
}

bool CServer::ParseUrl(wxString host, const wxString& port, wxString user, wxString pass, wxString &error, CServerPath &path)
{
	FAIL("CServer::ParseUrl");
	return true;
}

bool CServer::ProtocolHasDataTypeConcept(const enum ServerProtocol protocol)
{
	FAIL("CServer::ProtocolHasDataTypeConcept");
	return true;
}

bool CServer::SetAccount(const wxString& account)
{
	FAIL("CServer::SetAccount");
	return true;
}

void CServer::SetBypassProxy(bool val)
{
	FAIL("CServer::SetBypassProxy");
}

bool CServer::SetEncodingType(enum CharsetEncoding type, const wxString& encoding)
{
	FAIL("CServer::SetEncodingType");
	return true;
}

bool CServer::SetHost(wxString Host, unsigned int port)
{
	FAIL("CServer::SetHost");
	return true;
}

void CServer::SetLogonType(enum LogonType logonType)
{
	FAIL("CServer::SetLogonType");
}

void CServer::SetPasvMode(enum PasvMode pasvMode)
{
	FAIL("CServer::SetPasvMode");
}

bool CServer::SetPostLoginCommands(const std::vector<wxString>& postLoginCommands)
{
	FAIL("CServer::SetPostLoginCommands");
	return true;
}

void CServer::SetProtocol(enum ServerProtocol serverProtocol)
{
	FAIL("CServer::SetProtocol");
}

bool CServer::SetTimezoneOffset(int minutes)
{
	FAIL("CServer::SetTimezoneOffset");
	return true;
}

void CServer::SetType(enum ServerType type)
{
	FAIL("CServer::SetType");
}

bool CServer::SetUser(const wxString& user, const wxString& pass)
{
	FAIL("CServer::SetUser");
	return true;
}

CServer& CServer::operator=(const CServer &op)
{
	FAIL("CServer::operator=");
	return *this;
}

bool CServer::operator==(const CServer &op) const
{
	FAIL("CServer::operator==");
	return true;
}

bool CServer::operator!=(const CServer &op) const
{
	FAIL("CServer::operator!=");
	return true;
}

