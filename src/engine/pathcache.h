#ifndef __PATHCACHE_H__
#define __PATHCACHE_H__

class CPathCache
{
public:
	CPathCache();
	virtual ~CPathCache();

	// The source argument should be a canonicalized path already if subdir is non-empty
	static void Store(const CServer& server, const CServerPath& target, const CServerPath& source, const wxString subdir = _T(""));

	// The source argument should be a canonicalized path already if subdir is non-empty happen
	static CServerPath Lookup(const CServer& server, const CServerPath& source, const wxString subdir = _T(""));

	static void InvalidateServer(const CServer& server);

	// Invalidate path
	static void InvalidatePath(const CServer& server, const CServerPath& path, const wxString& subdir = _T(""));

	static void Clear();

protected:
	class CSourcePath
	{
	public:
		CServerPath source;
		wxString subdir;

		bool operator<(const CSourcePath& op) const
		{
			const int cmp = subdir.Cmp(op.subdir);
			if (cmp < 0)
				return true;
			if (cmp > 0)
				return false;

			return source < op.source;
		}
	};
	
	typedef std::map<CSourcePath, CServerPath> tServerCache;
	typedef tServerCache::iterator tServerCacheIterator;
	typedef tServerCache::const_iterator tServerCacheConstIterator;
	typedef std::map<CServer, tServerCache*> tCache;
	static tCache m_cache;
	typedef tCache::iterator tCacheIterator;
	typedef tCache::const_iterator tCacheConstIterator;

	static CServerPath Lookup(const tServerCache &serverCache, const CServerPath& source, const wxString subdir);

	static int m_hits;
	static int m_misses;

	static int m_instance_count;
};

#endif //__PATHCACHE_H__
