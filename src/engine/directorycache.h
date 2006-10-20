#ifndef __DIRECTORYCACHE_H__
#define __DIRECTORYCACHE_H__

/*
This class is the directory cache used to store retrieved directory listings 
for further use.
Directory get either purged from the cache if the maximum cache time exceeds,
or on possible data inconsistencies.
For example since some servers are case sensitive and others aren't, a 
directory is removed from cache once an operation effects a file wich matches
multiple entries in a cache directory using a case insensitive search
On other operations, the directory ist markes as unsure. It may still be valid,
but for some operations the engine/interface prefers to retrieve a clean 
version.
*/

class CDirectoryCache
{
public:
	enum Filetype
	{
		unknown,
		file,
		dir
	};

	CDirectoryCache();
	~CDirectoryCache();

	void Store(const CDirectoryListing &listing, const CServer &server, CServerPath parentPath = CServerPath(), wxString subDir = _T(""));
	bool HasChanged(CTimeEx since, const CServer &server, const CServerPath &path) const;
	bool GetChangeTime(CTimeEx& time, const CServer &server, const CServerPath &path) const;
	bool Lookup(CDirectoryListing &listing, const CServer &server, const CServerPath &path, bool allowUnsureEntries);
	bool Lookup(CDirectoryListing &listing, const CServer &server, const CServerPath &path, wxString subDir, bool allowUnsureEntries);
	bool DoesExist(const CServer &server, const CServerPath &path, wxString subDir, int &hasUnsureEntries);
	bool LookupFile(CDirentry &entry, const CServer &server, const CServerPath &path, const wxString& file, bool &dirDidExist, bool &matchedCase);
	bool InvalidateFile(const CServer &server, const CServerPath &path, const wxString& filename);
	bool UpdateFile(const CServer &server, const CServerPath &path, const wxString& filename, bool mayCreate, enum Filetype type = file, int size = -1);
	bool RemoveFile(const CServer &server, const CServerPath &path, const wxString& filename);
	void InvalidateServer(const CServer& server);
	void RemoveDir(const CServer& server, const CServerPath& path, const wxString& filename);
	void Rename(const CServer& server, const CServerPath& pathFrom, const wxString& fileFrom, const CServerPath& pathTo, const wxString& fileTo);
	void AddParent(const CServer& server, const CServerPath& path, const CServerPath& parentPath, const wxString subDir);

protected:

	class CCacheEntry
	{
	public:
		CCacheEntry() { };
		CCacheEntry(const CCacheEntry &entry);
		~CCacheEntry() { };
		CDirectoryListing listing;
		CServer server;
		wxDateTime createTime;
		CTimeEx modificationTime;
		typedef struct
		{
			CServerPath path;
			wxString subDir;
		} t_parent;
		std::list<t_parent> parents;

		CCacheEntry& operator=(const CCacheEntry &a);
	};

	typedef std::list<CCacheEntry::t_parent>::iterator tParentsIter;
	typedef std::list<CCacheEntry::t_parent>::const_iterator tParentsConstIter;
	typedef std::list<CCacheEntry>::iterator tCacheIter;
	typedef std::list<CCacheEntry>::const_iterator tCacheConstIter;

	static std::list<CCacheEntry> m_CacheList;
	
	static int m_nRefCount;
};

#endif

