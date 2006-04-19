#ifndef __RECENTSERVERLIST_H__
#define __RECENTSERVERLIST_H__

#include "xmlfunctions.h"

class CRecentServerList
{
public:
	static void SetMostRecentServer(const CServer& server);
	static const std::list<CServer> GetMostRecentServers(bool lockMutex = true);

protected:

	static CXmlFile m_XmlFile;
	static std::list<CServer> m_mostRecentServers;
};

#endif //__RECENTSERVERLIST_H__
