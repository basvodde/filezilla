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
	// First check for existing entry in cache and update if it neccessary
	for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		CCacheEntry &entry = *iter;
		if (entry.server != server || entry.listing.path != listing.path)
			continue;
		
		entry.modificationTime = CTimeEx::Now();
		entry.createTime = entry.modificationTime.GetTime();

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

	// Create new entry and store listing in cache
	CCacheEntry entry;
	entry.modificationTime = CTimeEx::Now();
	entry.createTime = entry.modificationTime.GetTime();
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
			if (entry.server != server)
				continue;

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
					if (parent.path != path || parent.subDir != subDir)
						continue;

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
	modificationTime = a.modificationTime;
	parents = a.parents;

	return *this;
}

CDirectoryCache::CCacheEntry::CCacheEntry(const CDirectoryCache::CCacheEntry &entry)
{
	listing = entry.listing;
	server = entry.server;
	createTime = entry.createTime;
	modificationTime = entry.modificationTime;
	parents = entry.parents;
}

bool CDirectoryCache::InvalidateFile(const CServer &server, const CServerPath &path, const wxString& filename, bool dir /*=false*/, int size /*=-1*/)
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
			listing[entry.listing.m_entryCount].size = size;
			listing[entry.listing.m_entryCount].dir = dir;
			listing[entry.listing.m_entryCount].link = 0;
			listing[entry.listing.m_entryCount].unsure = true;
			entry.listing.m_entryCount++;
			delete [] entry.listing.m_pEntries;
			entry.listing.m_pEntries = listing;
		}
		entry.listing.m_hasUnsureEntries = true;
		entry.modificationTime = CTimeEx::Now();
	}

	return true;
}

bool CDirectoryCache::RemoveFile(const CServer &server, const CServerPath &path, const wxString& filename)
{
	for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		CCacheEntry &entry = *iter;
		if (entry.server != server || path.CmpNoCase(entry.listing.path))
			continue;

		bool matchCase = false;
		for (unsigned int i = 0; i < entry.listing.m_entryCount; i++)
		{
			if (entry.listing.m_pEntries[i].name == filename)
				matchCase = true;
		}
		
		if (matchCase)
		{
			unsigned int i;
			for (i = 0; i < entry.listing.m_entryCount; i++)
				if (entry.listing.m_pEntries[i].name == filename)
					break;
			wxASSERT(i != entry.listing.m_entryCount);
			for (i++; i < entry.listing.m_entryCount; i++)
				entry.listing.m_pEntries[i - 1] = entry.listing.m_pEntries[i];
			entry.listing.m_entryCount--;
		}
		else
		{
			for (unsigned int i = 0; i < entry.listing.m_entryCount; i++)
			{
				if (!filename.CmpNoCase(entry.listing.m_pEntries[i].name))
				{
					entry.listing.m_pEntries[i].unsure = true;
				}				
			}
		}
		entry.listing.m_hasUnsureEntries = true;
		entry.modificationTime = CTimeEx::Now();
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

bool CDirectoryCache::HasChanged(CTimeEx since, const CServer &server, const CServerPath &path) const
{
	for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		CCacheEntry &entry = *iter;
		if (entry.server != server)
			continue;
		
		if (entry.listing.path != path)
			continue;

		if (entry.modificationTime > since)
			return true;
		else
			return false;
	}

	return false;
}

void CDirectoryCache::RemoveDir(const CServer& server, const CServerPath& path, const wxString& filename)
{
	// TODO: This is not 100% foolproof and my not work properly
	// Perhaps just throw away the complete cache?

	CServerPath absolutePath = path;
	if (!absolutePath.AddSegment(filename))
		absolutePath.Clear();

	std::list<CCacheEntry> newList;
	for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		CCacheEntry &entry = *iter;
		if (entry.server != server)
		{
			// Don't delet eitems from different server
			newList.push_back(*iter);
			continue;
		}

		// Delete exact matches and subdirs
		if (!absolutePath.IsEmpty() && (entry.listing.path == absolutePath || absolutePath.IsParentOf(entry.listing.path, true)))
			continue;

		tParentsIter parentsIter;
		for (parentsIter = entry.parents.begin(); parentsIter != entry.parents.end(); parentsIter++)
		{
			const CCacheEntry::t_parent &parent = *parentsIter;
			if (parent.path == path && parent.subDir == filename)
				break;
		}
		if (parentsIter != entry.parents.end())
			continue;

		newList.push_back(*iter);
	}
	
	m_CacheList = newList;

	RemoveFile(server, path, filename);
}
