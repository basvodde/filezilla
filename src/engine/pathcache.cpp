#include "filezilla.h"
#include "pathcache.h"

CPathCache::tCache CPathCache::m_cache;

int CPathCache::m_hits = 0;
int CPathCache::m_misses = 0;

int CPathCache::m_instance_count = 0;

static CPathCache cache;

CPathCache::CPathCache()
{
	m_instance_count++;
}

CPathCache::~CPathCache()
{
	m_instance_count--;
	if (!m_instance_count)
		Clear();
}

void CPathCache::Store(const CServer& server, const CServerPath& target, const CServerPath& source, const wxString subdir/*=_T("")*/)
{
	wxASSERT(!target.IsEmpty() && !source.IsEmpty());

	tServerCache *pServerCache;
	tCacheIterator iter = m_cache.find(server);
	if (iter != m_cache.end())
		pServerCache = iter->second;
	else
	{
		pServerCache = new tServerCache;
		m_cache[server] = pServerCache;
	}
	tServerCache &serverCache = *pServerCache;

	CSourcePath sourcePath;
	
	sourcePath.source = source;
	sourcePath.subdir = subdir;

	serverCache[sourcePath] = target;
}

CServerPath CPathCache::Lookup(const CServer& server, const CServerPath& source, const wxString subdir /*=_T("")*/)
{
	const tCacheConstIterator iter = m_cache.find(server);
	if (iter == m_cache.end())
		return CServerPath();

	CServerPath result = Lookup(*iter->second, source, subdir);

	if (result.IsEmpty())
		m_misses++;
	else
		m_hits++;

	return result;
}

CServerPath CPathCache::Lookup(const tServerCache &serverCache, const CServerPath& source, const wxString subdir)
{
	CSourcePath sourcePath;
	sourcePath.source = source;
	sourcePath.subdir = subdir;

	tServerCacheConstIterator serverIter = serverCache.find(sourcePath);
	if (serverIter == serverCache.end())
		return CServerPath();

	return serverIter->second;
}

void CPathCache::InvalidateServer(const CServer& server)
{
	tCacheIterator iter = m_cache.find(server);
	if (iter == m_cache.end())
		return;

	delete iter->second;
	m_cache.erase(iter);
}

void CPathCache::InvalidatePath(const CServer& server, const CServerPath& path, const wxString& subdir /*=_T("")*/)
{
	tCacheIterator iter = m_cache.find(server);
	if (iter == m_cache.end())
		return;

	CSourcePath sourcePath;
	
	sourcePath.source = path;
	sourcePath.subdir = subdir;

	CServerPath target;
	tServerCacheIterator serverIter = iter->second->find(sourcePath);
	if (serverIter != iter->second->end())
	{
		target = serverIter->second;
		iter->second->erase(serverIter);
	}
	
	if (target.IsEmpty() && subdir != _T(""))
	{
		target = path;
		if (!target.AddSegment(subdir))
			return;
	}

	if (!target.IsEmpty())
	{
		// Unfortunately O(n), don't know of a faster way.
		for (std::map<CSourcePath, CServerPath>::iterator serverIter = iter->second->begin(); serverIter != iter->second->end();)
		{
			if (serverIter->second == target || target.IsParentOf(serverIter->second, false))
				iter->second->erase(serverIter++);
			else if (serverIter->first.source == target || target.IsParentOf(serverIter->first.source, false))
				iter->second->erase(serverIter++);
			else
				++serverIter;
		}
	}
}

void CPathCache::Clear()
{
	for (tCacheIterator iter = m_cache.begin(); iter != m_cache.end(); iter++)
		delete iter->second;

	m_cache.clear();
}
