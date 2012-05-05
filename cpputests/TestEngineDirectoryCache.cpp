
#include "CppUTest/TestHarness.h"
#include <filezilla.h>
#include <directorycache.h>

TEST_GROUP(DirectoryCache)
{
	CServer server;
	CServerPath serverPath;
	CDirectoryListing listing;
	std::list<CDirentry> direntries;
	CDirectoryCache cache;

	void teardown()
	{
		cache.InvalidateServer(server);
	}
};

CDirentry createDirEntry(const wchar_t* filename)
{
	CDirentry entry;
	entry.name = filename;
	return entry;
}

TEST(DirectoryCache, WhenStoringAFileInTheCacheWeCanLookItUp)
{
	bool is_outdated;
	direntries.push_back(createDirEntry(L"filename"));
	listing.Assign(direntries);

	cache.Store(listing, server);
	CHECK(cache.Lookup(listing, server, serverPath, true, is_outdated));
}

