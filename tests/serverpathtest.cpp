#include "FileZilla.h"
#include "directorylistingparser.h"
#include <cppunit/extensions/HelperMacros.h>
#include <list>

/*
 * This testsuite asserts the correctness of the CServerPath class.
 */

class CServerPathTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(CServerPathTest);
	CPPUNIT_TEST(testGetPath);
	CPPUNIT_TEST(testHasParent);
	CPPUNIT_TEST(testGetParent);
	CPPUNIT_TEST(testFormatSubdir);
	CPPUNIT_TEST(testGetCommonParent);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {}
	void tearDown() {}

	void testGetPath();
	void testHasParent();
	void testGetParent();
	void testFormatSubdir();
	void testGetCommonParent();

protected:
};

CPPUNIT_TEST_SUITE_REGISTRATION(CServerPathTest);

void CServerPathTest::testGetPath()
{
	CServerPath unix1(_T("/"));
	CPPUNIT_ASSERT(unix1.GetPath() == _T("/"));

	CServerPath unix2(_T("/foo"));
	CPPUNIT_ASSERT(unix2.GetPath() == _T("/foo"));

	CServerPath unix3(_T("/foo/bar"));
	CPPUNIT_ASSERT(unix3.GetPath() == _T("/foo/bar"));

	CServerPath vms1(_T("FOO:[BAR]"));
	CPPUNIT_ASSERT(vms1.GetPath() == _T("FOO:[BAR]"));

	CServerPath vms2(_T("FOO:[BAR.TEST]"));
	CPPUNIT_ASSERT(vms2.GetPath() == _T("FOO:[BAR.TEST]"));

	CServerPath vms3(_T("FOO:[BAR^.TEST]"));
	CPPUNIT_ASSERT(vms3.GetPath() == _T("FOO:[BAR^.TEST]"));

	CServerPath vms4(_T("FOO:[BAR^.TEST.SOMETHING]"));
	CPPUNIT_ASSERT(vms4.GetPath() == _T("FOO:[BAR^.TEST.SOMETHING]"));

	CServerPath dos1(_T("C:\\"));
	CPPUNIT_ASSERT(dos1.GetPath() == _T("C:\\"));

	CServerPath dos2(_T("C:\\FOO"));
	CPPUNIT_ASSERT(dos2.GetPath() == _T("C:\\FOO"));

	CServerPath mvs1(_T("'FOO'"), MVS);
	CPPUNIT_ASSERT(mvs1.GetPath() == _T("'FOO'"));

	CServerPath mvs2(_T("'FOO.'"), MVS);
	CPPUNIT_ASSERT(mvs2.GetPath() == _T("'FOO.'"));

	CServerPath mvs3(_T("'FOO.BAR'"), MVS);
	CPPUNIT_ASSERT(mvs3.GetPath() == _T("'FOO.BAR'"));

	CServerPath mvs4(_T("'FOO.BAR.'"), MVS);
	CPPUNIT_ASSERT(mvs4.GetPath() == _T("'FOO.BAR.'"));

	CServerPath vxworks1(_T(":foo:"));
	CPPUNIT_ASSERT(vxworks1.GetPath() == _T(":foo:"));

	CServerPath vxworks2(_T(":foo:bar"));
	CPPUNIT_ASSERT(vxworks2.GetPath() == _T(":foo:bar"));

	CServerPath vxworks3(_T(":foo:bar/test"));
	CPPUNIT_ASSERT(vxworks3.GetPath() == _T(":foo:bar/test"));

	// ZVM is same as Unix, only makes difference in directory
	// listing parser.
	CServerPath zvm1(_T("/"), ZVM);
	CPPUNIT_ASSERT(zvm1.GetPath() == _T("/"));

	CServerPath zvm2(_T("/foo"), ZVM);
	CPPUNIT_ASSERT(zvm2.GetPath() == _T("/foo"));

	CServerPath zvm3(_T("/foo/bar"), ZVM);
	CPPUNIT_ASSERT(zvm3.GetPath() == _T("/foo/bar"));

	CServerPath hpnonstop1(_T("\\mysys"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop1.GetPath() == _T("\\mysys"));

	CServerPath hpnonstop2(_T("\\mysys.$myvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop2.GetPath() == _T("\\mysys.$myvol"));

	CServerPath hpnonstop3(_T("\\mysys.$myvol.mysubvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop3.GetPath() == _T("\\mysys.$myvol.mysubvol"));

	CServerPath dos_virtual1(_T("\\"));
	CPPUNIT_ASSERT(dos_virtual1.GetPath() == _T("\\"));

	CServerPath dos_virtual2(_T("\\foo"));
	CPPUNIT_ASSERT(dos_virtual2.GetPath() == _T("\\foo"));

	CServerPath dos_virtual3(_T("\\foo\\bar"));
	CPPUNIT_ASSERT(dos_virtual3.GetPath() == _T("\\foo\\bar"));
}

void CServerPathTest::testHasParent()
{
	CServerPath unix1(_T("/"));
	CPPUNIT_ASSERT(!unix1.HasParent());

	CServerPath unix2(_T("/foo"));
	CPPUNIT_ASSERT(unix2.HasParent());

	CServerPath unix3(_T("/foo/bar"));
	CPPUNIT_ASSERT(unix3.HasParent());

	CServerPath vms1(_T("FOO:[BAR]"));
	CPPUNIT_ASSERT(!vms1.HasParent());

	CServerPath vms2(_T("FOO:[BAR.TEST]"));
	CPPUNIT_ASSERT(vms2.HasParent());

	CServerPath vms3(_T("FOO:[BAR^.TEST]"));
	CPPUNIT_ASSERT(!vms3.HasParent());

	CServerPath vms4(_T("FOO:[BAR^.TEST.SOMETHING]"));
	CPPUNIT_ASSERT(vms4.HasParent());

	CServerPath dos1(_T("C:\\"));
	CPPUNIT_ASSERT(!dos1.HasParent());

	CServerPath dos2(_T("C:\\FOO"));
	CPPUNIT_ASSERT(dos2.HasParent());

	CServerPath mvs1(_T("'FOO'"), MVS);
	CPPUNIT_ASSERT(!mvs1.HasParent());

	CServerPath mvs2(_T("'FOO.'"), MVS);
	CPPUNIT_ASSERT(!mvs2.HasParent());

	CServerPath mvs3(_T("'FOO.BAR'"), MVS);
	CPPUNIT_ASSERT(mvs3.HasParent());

	CServerPath mvs4(_T("'FOO.BAR.'"), MVS);
	CPPUNIT_ASSERT(mvs4.HasParent());

	CServerPath vxworks1(_T(":foo:"));
	CPPUNIT_ASSERT(!vxworks1.HasParent());

	CServerPath vxworks2(_T(":foo:bar"));
	CPPUNIT_ASSERT(vxworks2.HasParent());

	CServerPath vxworks3(_T(":foo:bar/test"));
	CPPUNIT_ASSERT(vxworks3.HasParent());

	// ZVM is same as Unix, only makes difference in directory
	// listing parser.
	CServerPath zvm1(_T("/"), ZVM);
	CPPUNIT_ASSERT(!zvm1.HasParent());

	CServerPath zvm2(_T("/foo"), ZVM);
	CPPUNIT_ASSERT(zvm2.HasParent());

	CServerPath zvm3(_T("/foo/bar"), ZVM);
	CPPUNIT_ASSERT(zvm3.HasParent());

	CServerPath hpnonstop1(_T("\\mysys"), HPNONSTOP);
	CPPUNIT_ASSERT(!hpnonstop1.HasParent());

	CServerPath hpnonstop2(_T("\\mysys.$myvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop2.HasParent());

	CServerPath hpnonstop3(_T("\\mysys.$myvol.mysubvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop3.HasParent());

	CServerPath dos_virtual1(_T("\\"));
	CPPUNIT_ASSERT(!dos_virtual1.HasParent());

	CServerPath dos_virtual2(_T("\\foo"));
	CPPUNIT_ASSERT(dos_virtual2.HasParent());

	CServerPath dos_virtual3(_T("\\foo\\bar"));
	CPPUNIT_ASSERT(dos_virtual3.HasParent());
}

void CServerPathTest::testGetParent()
{
	CServerPath unix1(_T("/"));
	CServerPath unix2(_T("/foo"));
	CServerPath unix3(_T("/foo/bar"));
	CPPUNIT_ASSERT(unix2.GetParent() == unix1);
	CPPUNIT_ASSERT(unix3.GetParent() == unix2);

	CServerPath vms1(_T("FOO:[BAR]"));
	CServerPath vms2(_T("FOO:[BAR.TEST]"));
	CServerPath vms3(_T("FOO:[BAR^.TEST]"));
	CServerPath vms4(_T("FOO:[BAR^.TEST.SOMETHING]"));
	CPPUNIT_ASSERT(vms2.GetParent() == vms1);
	CPPUNIT_ASSERT(vms4.GetParent() == vms3);

	CServerPath dos1(_T("C:\\"));
	CServerPath dos2(_T("C:\\FOO"));
	CPPUNIT_ASSERT(dos2.GetParent() == dos1);

	CServerPath mvs1(_T("'FOO'"), MVS);
	CServerPath mvs2(_T("'FOO.'"), MVS);
	CServerPath mvs3(_T("'FOO.BAR'"), MVS);
	CServerPath mvs4(_T("'FOO.BAR.'"), MVS);
	CPPUNIT_ASSERT(mvs3.GetParent() == mvs2);
	CPPUNIT_ASSERT(mvs4.GetParent() == mvs2);

	CServerPath vxworks1(_T(":foo:"));
	CServerPath vxworks2(_T(":foo:bar"));
	CServerPath vxworks3(_T(":foo:bar/test"));
	CPPUNIT_ASSERT(vxworks2.GetParent() == vxworks1);
	CPPUNIT_ASSERT(vxworks3.GetParent() == vxworks2);

	// ZVM is same as Unix, only makes difference in directory
	// listing parser.
	CServerPath zvm1(_T("/"), ZVM);
	CServerPath zvm2(_T("/foo"), ZVM);
	CServerPath zvm3(_T("/foo/bar"), ZVM);
	CPPUNIT_ASSERT(zvm2.GetParent() == zvm1);
	CPPUNIT_ASSERT(zvm3.GetParent() == zvm2);

	CServerPath hpnonstop1(_T("\\mysys"), HPNONSTOP);
	CServerPath hpnonstop2(_T("\\mysys.$myvol"), HPNONSTOP);
	CServerPath hpnonstop3(_T("\\mysys.$myvol.mysubvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop2.GetParent() == hpnonstop1);
	CPPUNIT_ASSERT(hpnonstop3.GetParent() == hpnonstop2);

	CServerPath dos_virtual1(_T("\\"));
	CServerPath dos_virtual2(_T("\\foo"));
	CServerPath dos_virtual3(_T("\\foo\\bar"));
	CPPUNIT_ASSERT(dos_virtual2.GetParent() == dos_virtual1);
	CPPUNIT_ASSERT(dos_virtual3.GetParent() == dos_virtual2);
}

void CServerPathTest::testFormatSubdir()
{
	CServerPath path;
	path.SetType(VMS);

	CPPUNIT_ASSERT(path.FormatSubdir(_T("FOO.BAR")) == _T("FOO^.BAR"));
}


void CServerPathTest::testGetCommonParent()
{
	const CServerPath unix1(_T("/foo"));
	const CServerPath unix2(_T("/foo/baz"));
	const CServerPath unix3(_T("/foo/bar"));
	CPPUNIT_ASSERT(unix2.GetCommonParent(unix3) == unix1);
	CPPUNIT_ASSERT(unix1.GetCommonParent(unix1) == unix1);

	const CServerPath vms1(_T("FOO:[BAR]"));
	const CServerPath vms2(_T("FOO:[BAR.TEST]"));
	const CServerPath vms3(_T("GOO:[BAR"));
	const CServerPath vms4(_T("GOO:[BAR^.TEST.SOMETHING]"));
	CPPUNIT_ASSERT(vms2.GetCommonParent(vms1) == vms1);
	CPPUNIT_ASSERT(vms3.GetCommonParent(vms1) == CServerPath());

	const CServerPath dos1(_T("C:\\"));
	const CServerPath dos2(_T("C:\\FOO"));
	const CServerPath dos3(_T("D:\\FOO"));
	CPPUNIT_ASSERT(dos1.GetCommonParent(dos2) == dos1);
	CPPUNIT_ASSERT(dos2.GetCommonParent(dos3) == CServerPath());

	const CServerPath mvs1(_T("'FOO'"), MVS);
	const CServerPath mvs2(_T("'FOO.'"), MVS);
	const CServerPath mvs3(_T("'FOO.BAR'"), MVS);
	const CServerPath mvs4(_T("'FOO.BAR.'"), MVS);
	const CServerPath mvs5(_T("'BAR.'"), MVS);
	CPPUNIT_ASSERT(mvs1.GetCommonParent(mvs2) == CServerPath());
	CPPUNIT_ASSERT(mvs3.GetCommonParent(mvs1) == CServerPath());
	CPPUNIT_ASSERT(mvs4.GetCommonParent(mvs3) == mvs2);
	CPPUNIT_ASSERT(mvs3.GetCommonParent(mvs2) == mvs2);
	CPPUNIT_ASSERT(mvs4.GetCommonParent(mvs2) == mvs2);
	CPPUNIT_ASSERT(mvs5.GetCommonParent(mvs2) == CServerPath());

	const CServerPath vxworks1(_T(":foo:"));
	const CServerPath vxworks2(_T(":foo:bar"));
	const CServerPath vxworks3(_T(":foo:baz"));
	const CServerPath vxworks4(_T(":baz:bar"));
	CPPUNIT_ASSERT(vxworks1.GetCommonParent(vxworks2) == vxworks1);
	CPPUNIT_ASSERT(vxworks2.GetCommonParent(vxworks3) == vxworks1);
	CPPUNIT_ASSERT(vxworks4.GetCommonParent(vxworks2) == CServerPath());

	// ZVM is same as Unix, only makes difference in directory
	// listing parser.
	const CServerPath zvm1(_T("/foo"), ZVM);
	const CServerPath zvm2(_T("/foo/baz"), ZVM);
	const CServerPath zvm3(_T("/foo/bar"), ZVM);
	CPPUNIT_ASSERT(zvm2.GetCommonParent(zvm3) == zvm1);
	CPPUNIT_ASSERT(zvm1.GetCommonParent(zvm1) == zvm1);

	CServerPath hpnonstop1(_T("\\mysys"), HPNONSTOP);
	CServerPath hpnonstop2(_T("\\mysys.$myvol"), HPNONSTOP);
	CServerPath hpnonstop3(_T("\\mysys.$theirvol"), HPNONSTOP);
	CServerPath hpnonstop4(_T("\\theirsys.$myvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop2.GetCommonParent(hpnonstop3) == hpnonstop1);
	CPPUNIT_ASSERT(hpnonstop1.GetCommonParent(hpnonstop1) == hpnonstop1);
	CPPUNIT_ASSERT(hpnonstop2.GetCommonParent(hpnonstop4) == CServerPath());

	const CServerPath dos_virtual1(_T("\\foo"));
	const CServerPath dos_virtual2(_T("\\foo\\baz"));
	const CServerPath dos_virtual3(_T("\\foo\\bar"));
	CPPUNIT_ASSERT(dos_virtual2.GetCommonParent(dos_virtual3) == dos_virtual1);
	CPPUNIT_ASSERT(dos_virtual1.GetCommonParent(dos_virtual1) == dos_virtual1);
}
