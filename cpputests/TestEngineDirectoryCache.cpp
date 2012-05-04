
#include "CppUTest/TestHarness.h"
#include <filezilla.h>
#include <directorycache.h>

TEST_GROUP(DirectoryCache)
{
};

TEST(DirectoryCache, fail)
{
	CDirectoryCache * cache = new CDirectoryCache();
	FAIL("Failed");
}

