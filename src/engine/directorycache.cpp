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

bool CDirectoryCache::Lookup(CDirectoryListing &listing, const CServer &server, const CServerPath &path, bool allowUnsureEntries)
{
	return Lookup(listing, server, path, _T(""), allowUnsureEntries);
}

bool CDirectoryCache::Lookup(CDirectoryListing &listing, const CServer &server, const CServerPath &path, wxString subDir, bool allowUnsureEntries)
{
	if (subDir != _T(".."))
	{
		for (tCacheConstIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
		{
			const CCacheEntry &entry = *iter;
			if (entry.server != server)
				continue;

			if (subDir == _T(""))
			{
				if (entry.listing.path == path)
				{
					if (!allowUnsureEntries && entry.listing.m_hasUnsureEntries)
						return false;

					listing = entry.listing;
					return true;
				}
			}
			else
			{
				for (tParentsConstIter parentsIter = entry.parents.begin(); parentsIter != entry.parents.end(); parentsIter++)
				{
					const CCacheEntry::t_parent &parent = *parentsIter;
					if (parent.path != path || parent.subDir != subDir)
						continue;

					if (!allowUnsureEntries && entry.listing.m_hasUnsureEntries)
						return false;

					listing = entry.listing;
					return true;
				}
			}
		}
	}
	else
	{
		// Search own entry
		tCacheConstIter iter;
		for (iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
		{
			if (iter->server != server)
				continue;

			const CCacheEntry &entry = *iter;

			tParentsConstIter parentsIter;
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

		bool res = Lookup(listing, server, iter->listing.path, _T(""), allowUnsureEntries);
		return res;
	}
	return false;
}

bool CDirectoryCache::DoesExist(const CServer &server, const CServerPath &path, wxString subDir, bool &hasUnsureEntries)
{
	if (subDir != _T(".."))
	{
		for (tCacheConstIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
		{
			const CCacheEntry &entry = *iter;
			if (entry.server != server)
				continue;

			if (subDir == _T(""))
			{
				if (entry.listing.path == path)
				{
					hasUnsureEntries = entry.listing.m_hasUnsureEntries;
					return true;
				}
			}
			else
			{
				for (tParentsConstIter parentsIter = entry.parents.begin(); parentsIter != entry.parents.end(); parentsIter++)
				{
					const CCacheEntry::t_parent &parent = *parentsIter;
					if (parent.path != path || parent.subDir != subDir)
						continue;

					hasUnsureEntries = entry.listing.m_hasUnsureEntries;
					return true;
				}
			}
		}
	}
	else
	{
		// Search own entry
		tCacheConstIter iter;
		for (iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
		{
			if (iter->server != server)
				continue;

			const CCacheEntry &entry = *iter;

			tParentsConstIter parentsIter;
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

		bool res = DoesExist(server, iter->listing.path, _T(""), hasUnsureEntries);
		return res;
	}
	return false;
}

bool CDirectoryCache::LookupFile(CDirentry &entry, const CServer &server, const CServerPath &path, const wxString& file, bool &dirDidExist, bool &matchedCase)
{
	for (tCacheConstIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		const CCacheEntry &cacheEntry = *iter;
		if (cacheEntry.server != server)
			continue;

		if (cacheEntry.listing.path == path)
		{
			dirDidExist = true;

			const CDirectoryListing &listing = cacheEntry.listing;

			bool found = false;
			for (unsigned int i = 0; i < listing.m_entryCount; i++)
			{
				if (!listing.m_pEntries[i].name.CmpNoCase(file))
				{
					if (listing.m_pEntries[i].name == file)
					{
						entry = listing.m_pEntries[i];
						matchedCase = true;
						return true;
					}
					if (!found)
					{
						found = true;
						entry = listing.m_pEntries[i];
					}
				}
			}
			if (found)
			{
				matchedCase = false;
				return true;
			}

			return false;
		}
	}

	dirDidExist = false;
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

bool CDirectoryCache::InvalidateFile(const CServer &server, const CServerPath &path, const wxString& filename, bool mayCreate, enum Filetype type /*=file*/, int size /*=-1*/)
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
		if (!matchCase && type != unknown && mayCreate)
		{
			CDirentry *listing = new CDirentry[entry.listing.m_entryCount + 1];
			for (unsigned int i = 0; i < entry.listing.m_entryCount; i++)
				listing[i] = entry.listing.m_pEntries[i];
			listing[entry.listing.m_entryCount].name = filename;
			listing[entry.listing.m_entryCount].hasDate = false;
			listing[entry.listing.m_entryCount].hasTime = false;
			listing[entry.listing.m_entryCount].size = size;
			listing[entry.listing.m_entryCount].dir = (type == dir);
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

bool CDirectoryCache::GetChangeTime(CTimeEx& time, const CServer &server, const CServerPath &path) const
{
	for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		CCacheEntry &entry = *iter;
		if (entry.server != server)
			continue;
		
		if (entry.listing.path != path)
			continue;

		time = entry.modificationTime;
		return true;
	}

	return false;
}

void CDirectoryCache::RemoveDir(const CServer& server, const CServerPath& path, const wxString& filename)
{
	// TODO: This is not 100% foolproof and may not work properly
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
			// Don't delete items from different server
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

void CDirectoryCache::Rename(const CServer& server, const CServerPath& pathFrom, const wxString& fileFrom, const CServerPath& pathTo, const wxString& fileTo)
{
	CDirectoryListing listing;
	bool found = Lookup(listing, server, pathFrom, true);
	if (found)
	{
		unsigned int i;
		for (i = 0; i < listing.m_entryCount; i++)
		{
			if (listing.m_pEntries[i].name == fileFrom)
				break;
		}
		if (i != listing.m_entryCount)
		{
			if (listing.m_pEntries[i].dir)
			{
				RemoveDir(server, pathFrom, fileFrom);
				InvalidateFile(server, pathTo, fileTo, true, dir);
			}
			else
			{
				RemoveFile(server, pathFrom, fileFrom);
				InvalidateFile(server, pathTo, fileTo, true, file);
			}
		}
		else
		{
			// Be on the safe side, invalidate everything.
			InvalidateServer(server);
		}
	}
	else
	{
		// We know nothing, invalidate everything.
		InvalidateServer(server);
	}
}

void CDirectoryCache::AddParent(const CServer& server, const CServerPath& path, const CServerPath& parentPath, const wxString subDir)
{
	for (tCacheIter iter = m_CacheList.begin(); iter != m_CacheList.end(); iter++)
	{
		CCacheEntry &entry = *iter;
		if (entry.server != server)
			continue;

		if (entry.listing.path != path)
			continue;

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
}
