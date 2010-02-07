#include <filezilla.h>
#include <cppunit/extensions/HelperMacros.h>
#include "local_path.h"

/*
 * This testsuite asserts the correctness of the CLocalPathTest class.
 */

class CLocalPathTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(CLocalPathTest);
	CPPUNIT_TEST(testSetPath);
	CPPUNIT_TEST(testChangePath);
	CPPUNIT_TEST(testHasParent);
#ifdef __WXMSW__
	CPPUNIT_TEST(testHasLogicalParent);
#endif
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {}
	void tearDown() {}

	void testSetPath();
	void testChangePath();
	void testHasParent();
#ifdef __WXMSW__
	void testHasLogicalParent();
#endif

protected:
};

CPPUNIT_TEST_SUITE_REGISTRATION(CLocalPathTest);

void CLocalPathTest::testSetPath()
{
#ifdef __WXMSW__
	CPPUNIT_ASSERT(CLocalPath(_T("\\")).GetPath() == _T("\\"));

	CPPUNIT_ASSERT(CLocalPath(_T("C:")).GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\")).GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\.")).GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\.\\")).GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\.")).GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\..")).GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\..\\")).GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\foo")).GetPath() == _T("C:\\foo\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\..\\foo\\")).GetPath() == _T("C:\\foo\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\foo\\..\\bar")).GetPath() == _T("C:\\bar\\"));

	CPPUNIT_ASSERT(CLocalPath(_T("\\\\foo")).GetPath() == _T("\\\\foo\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("\\\\foo\\")).GetPath() == _T("\\\\foo\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("\\\\foo/")).GetPath() == _T("\\\\foo\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("\\\\foo/..")).GetPath() == _T("\\\\foo\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("\\\\foo\\.")).GetPath() == _T("\\\\foo\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("\\\\foo\\.\\")).GetPath() == _T("\\\\foo\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("\\\\foo\\bar\\")).GetPath() == _T("\\\\foo\\bar\\"));
	CPPUNIT_ASSERT(CLocalPath(_T("\\\\foo\\bar\\.\\..")).GetPath() == _T("\\\\foo\\"));
#else
	CPPUNIT_ASSERT(CLocalPath(_T("/")).GetPath() == _T("/"));
	CPPUNIT_ASSERT(CLocalPath(_T("/foo")).GetPath() == _T("/foo/"));
	CPPUNIT_ASSERT(CLocalPath(_T("//foo//")).GetPath() == _T("/foo/"));
	CPPUNIT_ASSERT(CLocalPath(_T("/foo/../foo")).GetPath() == _T("/foo/"));
	CPPUNIT_ASSERT(CLocalPath(_T("/foo/..")).GetPath() == _T("/"));
	CPPUNIT_ASSERT(CLocalPath(_T("/..")).GetPath() == _T("/"));
	CPPUNIT_ASSERT(CLocalPath(_T("/foo/.")).GetPath() == _T("/foo/"));
	CPPUNIT_ASSERT(CLocalPath(_T("/foo/./")).GetPath() == _T("/foo/"));
	CPPUNIT_ASSERT(CLocalPath(_T("/foo/bar/")).GetPath() == _T("/foo/bar/"));
	CPPUNIT_ASSERT(CLocalPath(_T("/foo/bar/./..")).GetPath() == _T("/foo/"));
#endif
}

void CLocalPathTest::testChangePath()
{
#ifdef __WXMSW__
	CLocalPath p1(_T("C:\\"));
	CPPUNIT_ASSERT(p1.ChangePath(_T("\\")) && p1.GetPath() == _T("\\"));
	CPPUNIT_ASSERT(p1.ChangePath(_T("C:")) && p1.GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(p1.ChangePath(_T("C:\\.")) && p1.GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(p1.ChangePath(_T("C:\\..")) && p1.GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(p1.ChangePath(_T("foo")) && p1.GetPath() == _T("C:\\foo\\"));
	CPPUNIT_ASSERT(p1.ChangePath(_T("..")) && p1.GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(p1.ChangePath(_T("..")) && p1.GetPath() == _T("C:\\"));
	CPPUNIT_ASSERT(p1.ChangePath(_T("C:\\foo")) && p1.GetPath() == _T("C:\\foo\\"));
	CPPUNIT_ASSERT(p1.ChangePath(_T(".")) && p1.GetPath() == _T("C:\\foo\\"));
	CPPUNIT_ASSERT(p1.ChangePath(_T("..\\bar")) && p1.GetPath() == _T("C:\\bar\\"));

	CLocalPath p2;
	CPPUNIT_ASSERT(p2.ChangePath(_T("\\\\foo")) && p2.GetPath() == _T("\\\\foo\\"));
	CPPUNIT_ASSERT(p2.ChangePath(_T(".")) && p2.GetPath() == _T("\\\\foo\\"));
	CPPUNIT_ASSERT(p2.ChangePath(_T("..")) && p2.GetPath() == _T("\\\\foo\\"));
	CPPUNIT_ASSERT(p2.ChangePath(_T("..\\bar\\.\\baz\\..")) && p2.GetPath() == _T("\\\\foo\\bar\\"));
#else
#endif
}

void CLocalPathTest::testHasParent()
{
#ifdef __WXMSW__
	CPPUNIT_ASSERT(!CLocalPath(_T("\\")).HasParent());

	CPPUNIT_ASSERT(!CLocalPath(_T("C:\\")).HasParent());
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\foo")).HasParent());
	CPPUNIT_ASSERT(CLocalPath(_T("c:\\foo\\bar\\")).HasParent());

	CPPUNIT_ASSERT(!CLocalPath(_T("\\\\foo")).HasParent());
	CPPUNIT_ASSERT(CLocalPath(_T("\\\\foo\\bar")).HasParent());
#else
	CPPUNIT_ASSERT(!CLocalPath(_T("/")).HasParent());
	CPPUNIT_ASSERT(CLocalPath(_T("/foo")).HasParent());
	CPPUNIT_ASSERT(CLocalPath(_T("/foo/bar")).HasParent());
#endif
}

#ifdef __WXMSW__
void CLocalPathTest::testHasLogicalParent()
{
	CPPUNIT_ASSERT(!CLocalPath(_T("\\")).HasLogicalParent());

	CPPUNIT_ASSERT(CLocalPath(_T("C:\\")).HasLogicalParent()); // This one's only difference
	CPPUNIT_ASSERT(CLocalPath(_T("C:\\foo")).HasLogicalParent());
	CPPUNIT_ASSERT(CLocalPath(_T("c:\\foo\\bar\\")).HasLogicalParent());

	CPPUNIT_ASSERT(!CLocalPath(_T("\\\\foo")).HasLogicalParent());
	CPPUNIT_ASSERT(CLocalPath(_T("\\\\foo\\bar")).HasLogicalParent());
}
#endif
