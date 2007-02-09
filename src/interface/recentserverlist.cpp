#include "FileZilla.h"
#include "recentserverlist.h"
#include "ipcmutex.h"
#include "filezillaapp.h"
#include "xmlfunctions.h"

CXmlFile CRecentServerList::m_XmlFile;
std::list<CServer> CRecentServerList::m_mostRecentServers;

const std::list<CServer> CRecentServerList::GetMostRecentServers(bool lockMutex /*=true*/)
{
	CInterProcessMutex mutex(MUTEX_MOSTRECENTSERVERS, false);
	if (lockMutex)
		mutex.Lock();

	if (!m_XmlFile.HasFileName() || m_XmlFile.Modified())
		m_XmlFile.Load(_T("recentservers"));
	else
		return m_mostRecentServers;

	TiXmlElement* pElement = m_XmlFile.GetElement();
	if (!pElement || !(pElement = pElement->FirstChildElement("RecentServers")))
		return m_mostRecentServers;

	m_mostRecentServers.clear();
    
	bool modified = false;
	TiXmlElement* pServer = pElement->FirstChildElement("Server");
	while (pServer)
	{
		CServer server;
		if (!GetServer(pServer, server) || m_mostRecentServers.size() >= 10)
		{
			TiXmlElement* pRemove = pServer;
			pServer = pServer->NextSiblingElement("Server");
			pElement->RemoveChild(pRemove);
			modified = true;
		}
		else
		{
			std::list<CServer>::const_iterator iter;
			for (iter = m_mostRecentServers.begin(); iter != m_mostRecentServers.end(); iter++)
			{
				if (*iter == server)
					break;
			}
			if (iter == m_mostRecentServers.end())
				m_mostRecentServers.push_back(server);
			pServer = pServer->NextSiblingElement("Server");
		}
	}

	if (modified)
	{
		wxString error;
		m_XmlFile.Save(&error);
	}

	return m_mostRecentServers;
}

void CRecentServerList::SetMostRecentServer(const CServer& server)
{
	CInterProcessMutex mutex(MUTEX_MOSTRECENTSERVERS);

	// Make sure list is initialized
	GetMostRecentServers(false);

	bool relocated = false;
	for (std::list<CServer>::iterator iter = m_mostRecentServers.begin(); iter != m_mostRecentServers.end(); iter++)
	{
		if (*iter == server)
		{
			m_mostRecentServers.erase(iter);
			m_mostRecentServers.push_front(server);
			relocated = true;
			break;
		}
	}
	if (!relocated)
	{
		m_mostRecentServers.push_front(server);
		if (m_mostRecentServers.size() > 10)
			m_mostRecentServers.pop_back();
	}

	TiXmlElement* pDocument = m_XmlFile.GetElement();
	if (!pDocument)
		return;
	
	TiXmlElement* pElement = pDocument->FirstChildElement("RecentServers");
	if (!pElement)
		pElement = pDocument->InsertEndChild(TiXmlElement("RecentServers"))->ToElement();

	pElement->Clear();
	for (std::list<CServer>::const_iterator iter = m_mostRecentServers.begin(); iter != m_mostRecentServers.end(); iter++)
	{
		TiXmlElement* pServer = pElement->InsertEndChild(TiXmlElement("Server"))->ToElement();
		SetServer(pServer, *iter);
	}

	wxString error;
	m_XmlFile.Save(&error);
}

void CRecentServerList::Clear()
{
	m_mostRecentServers.clear();

	CInterProcessMutex mutex(MUTEX_MOSTRECENTSERVERS);
	if (!m_XmlFile.HasFileName())
		m_XmlFile.SetFileName(_T("recentservers"));

	m_XmlFile.CreateEmpty();

	wxString error;
	m_XmlFile.Save(&error);
}
