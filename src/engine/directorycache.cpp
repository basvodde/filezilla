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
	for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		const CCacheEntry &entry = *iter;
		if (entry.server == server && entry.listing.path == path)
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
	return false;
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
	else
	{
		// Search own entry
		tCacheIter iter;
		for (iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
		{
			if (iter->server == server && iter->listing.path == path)
				break;
		}
		if (iter == m_CacheList.end())
			return false;
		CCacheEntry &entry = *iter;

		tParentsIter parentsIter;
		for (parentsIter = entry.parents.begin(); parentsIter != entry.parents.end(); parentsIter++)
		{
			if (parentsIter->subDir == _T(".."))
				break;
		}
		if (parentsIter == entry.parents.end())
			return false;

		bool res = Lookup(listing, server, parentsIter->path);
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