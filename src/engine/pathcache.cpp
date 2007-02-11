#include "FileZilla.h"
#include "pathcache.h"

CPathCache::tCache CPathCache::m_cache;

int CPathCache::m_hits = 0;
int CPathCache::m_misses = 0;

void CPathCache::Store(const CServer& server, const CServerPath& target, const CServerPath& source, const wxString subdir/*=_T("")*/)
{
	wxASSERT(!target.IsEmpty() && !source.IsEmpty());

	tServerCache &serverCache = m_cache[server];

	CSourcePath sourcePath;
	
	sourcePath.source = Lookup(serverCache, source, _T(""));
	if (sourcePath.source.IsEmpty())
		sourcePath.source = source;

	sourcePath.subdir = subdir;

	serverCache[sourcePath] = target;

	if (subdir != _T(""))
	{
		if (subdir == _T(".."))
		{
			if (!sourcePath.source.HasParent())
				return;
			sourcePath.source = sourcePath.source.GetParent();
		}
		else
			sourcePath.source.AddSegment(subdir);

		if (sourcePath.source != target)
			return;
		sourcePath.subdir = _T("");

		serverCache[sourcePath] = target;
	}
}

CServerPath CPathCache::Lookup(const CServer& server, const CServerPath& source, const wxString subdir /*=_T("")*/)
{
	const tCacheConstIterator iter = m_cache.find(server);
	if (iter == m_cache.end())
		return CServerPath();

	const CServerPath& result = Lookup(iter->second, source, subdir);

	if (result.IsEmpty())
		m_misses++;
	else
		m_hits++;

	return result;
}

CServerPath CPathCache::Lookup(const tServerCache &serverCache, const CServerPath& source, const wxString subdir) const
{
	CSourcePath sourcePath;
	
	if (subdir != _T(""))
	{
		sourcePath.source = Lookup(serverCache, source, _T(""));
		if (sourcePath.source.IsEmpty())
			sourcePath.source = source;
	}
	else
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

	m_cache.erase(iter);
}

void CPathCache::InvalidatePath(const CServer& server, const CServerPath& path, const wxString& subdir /*=_T("")*/)
{
	// It should suffice to just remove the path/subdir pair and the constructed path
	// Since directory removal is done recursively, the childs will be deleted prior to this one
	// Even if not (for whatever reason), removing this entry should suffice in case its target changes
	// after MKDing it again. (e.g. RMD a link, MKD a regular directory)

	tCacheIterator iter = m_cache.find(server);
	if (iter == m_cache.end())
		return;

	CSourcePath sourcePath;
	
	sourcePath.source = Lookup(iter->second, path, _T(""));
	if (sourcePath.source.IsEmpty())
		sourcePath.source = path;

	sourcePath.subdir = subdir;

	tServerCacheIterator serverIter = iter->second.find(sourcePath);
	if (serverIter != iter->second.end())
		iter->second.erase(serverIter);

	sourcePath.source.AddSegment(subdir);
	sourcePath.subdir = _T("");
	serverIter = iter->second.find(sourcePath);
	if (serverIter != iter->second.end())
		iter->second.erase(serverIter);
}
