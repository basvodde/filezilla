#include "FileZilla.h"
#include "directorycache.h"

int CDirectoryCache::m_nRefCount = 0;
std::list<CDirectoryCache::CCacheEntry> CDirectoryCache::m_CacheList;

CDirectoryCache::CDirectoryCache()
{
	m_nRefCount++;
}

CDirectoryCache::~CDirectoryCache()
{
	m_nRefCount--;
}

void CDirectoryCache::Store(const CDirectoryListing &listing, const CServer &server, CServerPath parentPath /*=CServerPath()*/, wxString subDir /*=_T("")*/)
{
	for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		CCacheEntry &entry = *iter;
		if (entry.server == server && entry.listing.path == listing.path)
		{
			entry.createTime = wxDateTime::Now();
			entry.listing = listing;

			if (!parentPath.IsEmpty() && subDir != _T(""))
			{
				// Search for parents
				for (tParentsIter parentsIter = entry.parents.begin(); parentsIter != entry.parents.end(); parentsIter++)
				{
					const CCacheEntry::t_parent &parent = *parentsIter;
					if (parent.path == parentPath && parent.subDir == subDir)
						return;
				}
	
				// Store parent
				CCacheEntry::t_parent parent;
				parent.path = parentPath;
				parent.subDir = subDir;
				entry.parents.push_front(parent);
			}

			return;
		}
	}

	// Store listing in cache
	CCacheEntry entry;
	entry.createTime = wxDateTime::Now();
	entry.listing = listing;
	entry.server = server;

	if (!parentPath.IsEmpty() && subDir != _T(""))
	{
		// Store parent
		CCacheEntry::t_parent parent;
		parent.path = parentPath;
		parent.subDir = subDir;
		entry.parents.push_front(parent);
	}

	m_CacheList.push_front(entry);
}

bool CDirectoryCache::Lookup(CDirectoryListing &listing, const CServer &server, const CServerPath &path)
{
	return Lookup(listing, server, path, _T(""));
}

bool CDirectoryCache::Lookup(CDirectoryListing &listing, const CServer &server, const CServerPath &path, wxString subDir)
{
	if (subDir != _T(".."))
	{
		for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
		{
			CCacheEntry &entry = *iter;
			if (entry.server == server)
			{
				if (subDir == _T(""))
				{
					if (entry.listing.path == path)
					{
						listing = entry.listing;
						return true;
					}
				}
				else
				{
					for (tParentsIter parentsIter = entry.parents.begin(); parentsIter != entry.parents.end(); parentsIter++)
					{
						const CCacheEntry::t_parent &parent = *parentsIter;
						if (parent.path == path && parent.subDir == subDir)
						{
							// TODO: Cache timeout
							if (false)
							{
								m_CacheList.erase(iter);
								return false;
							}
							listing = entry.listing;
							return true;
						}
					}
				}
			}
		}
	}
	else
	{
		// Search own entry
		tCacheIter iter;
		for (iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
		{
			if (iter->server != server)
				continue;

			CCacheEntry &entry = *iter;

			tParentsIter parentsIter;
			for (parentsIter = entry.parents.begin(); parentsIter != entry.parents.end(); parentsIter++)
			{
				if (parentsIter->subDir == _T("..") && parentsIter->path == path)
					break;
			}
			if (parentsIter != entry.parents.end())
				break;
		}
		if (iter == m_CacheList.end())
			return false;

		bool res = Lookup(listing, server, iter->listing.path);
		return res;
	}
	return false;
}

CDirectoryCache::CCacheEntry& CDirectoryCache::CCacheEntry::operator=(const CDirectoryCache::CCacheEntry &a)
{
	listing = a.listing;
	server = a.server;
	createTime = a.createTime;
	parents = a.parents;

	return *this;
}

CDirectoryCache::CCacheEntry::CCacheEntry(const CDirectoryCache::CCacheEntry &entry)
{
	listing = entry.listing;
	server = entry.server;
	createTime = entry.createTime;
	parents = entry.parents;
}

bool CDirectoryCache::InvalidateFile(wxString filename, const CServer &server, const CServerPath &path)
{
	for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		CCacheEntry &entry = *iter;
		if (entry.server != server || path.CmpNoCase(entry.listing.path))
			continue;

		bool matchCase = false;
		for (unsigned int i = 0; i < entry.listing.m_entryCount; i++)
		{
			if (!filename.CmpNoCase(entry.listing.m_pEntries[i].name))
			{
				entry.listing.m_pEntries[i].unsure = true;
				if (entry.listing.m_pEntries[i].name == filename)
					matchCase = true;
			}				
		}
		if (!matchCase)
		{
			CDirentry *listing = new CDirentry[entry.listing.m_entryCount + 1];
			for (unsigned int i = 0; i < entry.listing.m_entryCount; i++)
				listing[i] = entry.listing.m_pEntries[i];
			listing[entry.listing.m_entryCount].name = filename;
			listing[entry.listing.m_entryCount].hasDate = false;
			listing[entry.listing.m_entryCount].hasTime = false;
			listing[entry.listing.m_entryCount].size = -1;
			listing[entry.listing.m_entryCount].dir = 0;
			listing[entry.listing.m_entryCount].link = 0;
			listing[entry.listing.m_entryCount].unsure = true;
			entry.listing.m_entryCount++;
			delete [] entry.listing.m_pEntries;
			entry.listing.m_pEntries = listing;
		}
	}

	return true;
}

void CDirectoryCache::InvalidateServer(const CServer& server)
{
	std::list<CCacheEntry> newCache;
	for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		const CCacheEntry &entry = *iter;
		if (entry.server != server)
			newCache.push_back(entry);
	}
	m_CacheList = newCache;
}
