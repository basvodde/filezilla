#include "FileZilla.h"
#include "servercapabilities.h"

std::map<CServer, CCapabilities> CServerCapabilities::m_serverMap;

enum capabilities CCapabilities::GetCapability(enum capabilityNames name, wxString* pOption /*=0*/) const
{
	const std::map<enum capabilityNames, struct CCapabilities::t_cap>::const_iterator iter = m_capabilityMap.find(name);
	if (iter == m_capabilityMap.end())
		return unknown;

	if (iter->second.cap == yes && pOption)
		*pOption = iter->second.option;
	return iter->second.cap;
}

enum capabilities CCapabilities::GetCapability(enum capabilityNames name, int* pOption) const
{
	const std::map<enum capabilityNames, struct CCapabilities::t_cap>::const_iterator iter = m_capabilityMap.find(name);
	if (iter == m_capabilityMap.end())
		return unknown;

	if (iter->second.cap == yes && pOption)
		*pOption = iter->second.number;
	return iter->second.cap;
}

void CCapabilities::SetCapability(enum capabilityNames name, enum capabilities cap, const wxString& option /*=_T("")*/)
{
	wxASSERT(cap == yes || option == _T(""));
	struct CCapabilities::t_cap tcap;
	tcap.cap = cap;
	tcap.option = option;
	tcap.number = 0;

	m_capabilityMap[name] = tcap;
}

void CCapabilities::SetCapability(enum capabilityNames name, enum capabilities cap, int option)
{
	wxASSERT(cap == yes || option == 0);
	struct CCapabilities::t_cap tcap;
	tcap.cap = cap;
	tcap.number = option;

	m_capabilityMap[name] = tcap;
}

enum capabilities CServerCapabilities::GetCapability(const CServer& server, enum capabilityNames name, wxString* pOption /*=0*/)
{
	const std::map<CServer, CCapabilities>::const_iterator iter = m_serverMap.find(server);
	if (iter == m_serverMap.end())
		return unknown;

	return iter->second.GetCapability(name, pOption);
}

enum capabilities CServerCapabilities::GetCapability(const CServer& server, enum capabilityNames name, int* pOption)
{
	const std::map<CServer, CCapabilities>::const_iterator iter = m_serverMap.find(server);
	if (iter == m_serverMap.end())
		return unknown;

	return iter->second.GetCapability(name, pOption);
}

void CServerCapabilities::SetCapability(const CServer& server, enum capabilityNames name, enum capabilities cap, const wxString& option /*=_T("")*/)
{
	const std::map<CServer, CCapabilities>::iterator iter = m_serverMap.find(server);
	if (iter == m_serverMap.end())
	{
		CCapabilities capabilities;
		capabilities.SetCapability(name, cap, option);
		m_serverMap[server] = capabilities;
		return;
	}

	iter->second.SetCapability(name, cap, option);
}

void CServerCapabilities::SetCapability(const CServer& server, enum capabilityNames name, enum capabilities cap, int option)
{
	const std::map<CServer, CCapabilities>::iterator iter = m_serverMap.find(server);
	if (iter == m_serverMap.end())
	{
		CCapabilities capabilities;
		capabilities.SetCapability(name, cap, option);
		m_serverMap[server] = capabilities;
		return;
	}

	iter->second.SetCapability(name, cap, option);
}
