#include "FileZilla.h"
#include "servercapabilities.h"

std::map<CServer, CCapabilities> CServerCapabilities::m_serverMap;

enum capabilities CCapabilities::GetCapability(enum capabilityNames name) const
{
	const std::map<enum capabilityNames, enum capabilities>::const_iterator iter = m_capabilityMap.find(name);
	if (iter == m_capabilityMap.end())
		return unknown;

	return iter->second;
}

void CCapabilities::SetCapability(enum capabilityNames name, enum capabilities cap)
{
	m_capabilityMap[name] = cap;
}

enum capabilities CServerCapabilities::GetCapability(const CServer& server, enum capabilityNames name)
{
	const std::map<CServer, CCapabilities>::const_iterator iter = m_serverMap.find(server);
	if (iter == m_serverMap.end())
		return unknown;

	return iter->second.GetCapability(name);
}

void CServerCapabilities::SetCapability(const CServer& server, enum capabilityNames name, enum capabilities cap)
{
	const std::map<CServer, CCapabilities>::iterator iter = m_serverMap.find(server);
	if (iter == m_serverMap.end())
	{
		CCapabilities capabilities;
		capabilities.SetCapability(name, cap);
		m_serverMap[server] = capabilities;
		return;
	}

	iter->second.SetCapability(name, cap);
}
