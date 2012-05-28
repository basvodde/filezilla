
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "directorylisting.h"

CDirectoryListing::CDirectoryListing()
{
	FAIL("CDirectoryListing::CDirectoryListing");
}

CDirentry gEntry;
const CDirentry& CDirectoryListing::operator[](unsigned int index) const
{
	FAIL("CDirectoryListing::operator[]");
	return gEntry;
}

CDirentry& CDirectoryListing::operator[](unsigned int index)
{
	FAIL("CDirectoryListing::operator[]");
	return gEntry;
}
