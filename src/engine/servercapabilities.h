#ifndef __SERVERCAPABILITIES_H__
#define __SERVERCAPABILITIES_H__

enum capabilities
{
	unknown,
	yes,
	no
};

enum capabilityNames
{
	resume2GBbug,
	resume4GBbug
};

class CCapabilities
{
public:
	enum capabilities GetCapability(enum capabilityNames name) const;
	void SetCapability(enum capabilityNames name, enum capabilities cap);

protected:
	std::map<enum capabilityNames, enum capabilities> m_capabilityMap;
};

class CServerCapabilities
{
public:
	static enum capabilities GetCapability(const CServer& server, enum capabilityNames name);
	static void SetCapability(const CServer& server, enum capabilityNames name, enum capabilities cap);

protected:
	static std::map<CServer, CCapabilities> m_serverMap;
};

#endif //__SERVERCAPABILITIES_H__
