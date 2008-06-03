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
	CPPUNIT_TEST(testFormatFilename);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {}
	void tearDown() {}

	void testGetPath();
	void testHasParent();
	void testGetParent();
	void testFormatSubdir();
	void testGetCommonParent();
	void testFormatFilename();

protected:
};

CPPUNIT_TEST_SUITE_REGISTRATION(CServerPathTest);

void CServerPathTest::testGetPath()
{
	const CServerPath unix1(_T("/"));
	CPPUNIT_ASSERT(unix1.GetPath() == _T("/"));

	const CServerPath unix2(_T("/foo"));
	CPPUNIT_ASSERT(unix2.GetPath() == _T("/foo"));

	const CServerPath unix3(_T("/foo/bar"));
	CPPUNIT_ASSERT(unix3.GetPath() == _T("/foo/bar"));

	const CServerPath vms1(_T("FOO:[BAR]"));
	CPPUNIT_ASSERT(vms1.GetPath() == _T("FOO:[BAR]"));

	const CServerPath vms2(_T("FOO:[BAR.TEST]"));
	CPPUNIT_ASSERT(vms2.GetPath() == _T("FOO:[BAR.TEST]"));

	const CServerPath vms3(_T("FOO:[BAR^.TEST]"));
	CPPUNIT_ASSERT(vms3.GetPath() == _T("FOO:[BAR^.TEST]"));

	const CServerPath vms4(_T("FOO:[BAR^.TEST.SOMETHING]"));
	CPPUNIT_ASSERT(vms4.GetPath() == _T("FOO:[BAR^.TEST.SOMETHING]"));

	const CServerPath dos1(_T("C:\\"));
	CPPUNIT_ASSERT(dos1.GetPath() == _T("C:\\"));

	const CServerPath dos2(_T("C:\\FOO"));
	CPPUNIT_ASSERT(dos2.GetPath() == _T("C:\\FOO"));

	const CServerPath mvs1(_T("'FOO'"), MVS);
	CPPUNIT_ASSERT(mvs1.GetPath() == _T("'FOO'"));

	const CServerPath mvs2(_T("'FOO.'"), MVS);
	CPPUNIT_ASSERT(mvs2.GetPath() == _T("'FOO.'"));

	const CServerPath mvs3(_T("'FOO.BAR'"), MVS);
	CPPUNIT_ASSERT(mvs3.GetPath() == _T("'FOO.BAR'"));

	const CServerPath mvs4(_T("'FOO.BAR.'"), MVS);
	CPPUNIT_ASSERT(mvs4.GetPath() == _T("'FOO.BAR.'"));

	const CServerPath vxworks1(_T(":foo:"));
	CPPUNIT_ASSERT(vxworks1.GetPath() == _T(":foo:"));

	const CServerPath vxworks2(_T(":foo:bar"));
	CPPUNIT_ASSERT(vxworks2.GetPath() == _T(":foo:bar"));

	const CServerPath vxworks3(_T(":foo:bar/test"));
	CPPUNIT_ASSERT(vxworks3.GetPath() == _T(":foo:bar/test"));

	// ZVM is same as Unix, only makes difference in directory
	// listing parser.
	const CServerPath zvm1(_T("/"), ZVM);
	CPPUNIT_ASSERT(zvm1.GetPath() == _T("/"));

	const CServerPath zvm2(_T("/foo"), ZVM);
	CPPUNIT_ASSERT(zvm2.GetPath() == _T("/foo"));

	const CServerPath zvm3(_T("/foo/bar"), ZVM);
	CPPUNIT_ASSERT(zvm3.GetPath() == _T("/foo/bar"));

	const CServerPath hpnonstop1(_T("\\mysys"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop1.GetPath() == _T("\\mysys"));

	const CServerPath hpnonstop2(_T("\\mysys.$myvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop2.GetPath() == _T("\\mysys.$myvol"));

	const CServerPath hpnonstop3(_T("\\mysys.$myvol.mysubvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop3.GetPath() == _T("\\mysys.$myvol.mysubvol"));

	const CServerPath dos_virtual1(_T("\\"));
	CPPUNIT_ASSERT(dos_virtual1.GetPath() == _T("\\"));

	const CServerPath dos_virtual2(_T("\\foo"));
	CPPUNIT_ASSERT(dos_virtual2.GetPath() == _T("\\foo"));

	const CServerPath dos_virtual3(_T("\\foo\\bar"));
	CPPUNIT_ASSERT(dos_virtual3.GetPath() == _T("\\foo\\bar"));
}

void CServerPathTest::testHasParent()
{
	const CServerPath unix1(_T("/"));
	CPPUNIT_ASSERT(!unix1.HasParent());

	const CServerPath unix2(_T("/foo"));
	CPPUNIT_ASSERT(unix2.HasParent());

	const CServerPath unix3(_T("/foo/bar"));
	CPPUNIT_ASSERT(unix3.HasParent());

	const CServerPath vms1(_T("FOO:[BAR]"));
	CPPUNIT_ASSERT(!vms1.HasParent());

	const CServerPath vms2(_T("FOO:[BAR.TEST]"));
	CPPUNIT_ASSERT(vms2.HasParent());

	const CServerPath vms3(_T("FOO:[BAR^.TEST]"));
	CPPUNIT_ASSERT(!vms3.HasParent());

	const CServerPath vms4(_T("FOO:[BAR^.TEST.SOMETHING]"));
	CPPUNIT_ASSERT(vms4.HasParent());

	const CServerPath dos1(_T("C:\\"));
	CPPUNIT_ASSERT(!dos1.HasParent());

	const CServerPath dos2(_T("C:\\FOO"));
	CPPUNIT_ASSERT(dos2.HasParent());

	const CServerPath mvs1(_T("'FOO'"), MVS);
	CPPUNIT_ASSERT(!mvs1.HasParent());

	const CServerPath mvs2(_T("'FOO.'"), MVS);
	CPPUNIT_ASSERT(!mvs2.HasParent());

	const CServerPath mvs3(_T("'FOO.BAR'"), MVS);
	CPPUNIT_ASSERT(mvs3.HasParent());

	const CServerPath mvs4(_T("'FOO.BAR.'"), MVS);
	CPPUNIT_ASSERT(mvs4.HasParent());

	const CServerPath vxworks1(_T(":foo:"));
	CPPUNIT_ASSERT(!vxworks1.HasParent());

	const CServerPath vxworks2(_T(":foo:bar"));
	CPPUNIT_ASSERT(vxworks2.HasParent());

	const CServerPath vxworks3(_T(":foo:bar/test"));
	CPPUNIT_ASSERT(vxworks3.HasParent());

	// ZVM is same as Unix, only makes difference in directory
	// listing parser.
	const CServerPath zvm1(_T("/"), ZVM);
	CPPUNIT_ASSERT(!zvm1.HasParent());

	const CServerPath zvm2(_T("/foo"), ZVM);
	CPPUNIT_ASSERT(zvm2.HasParent());

	const CServerPath zvm3(_T("/foo/bar"), ZVM);
	CPPUNIT_ASSERT(zvm3.HasParent());

	const CServerPath hpnonstop1(_T("\\mysys"), HPNONSTOP);
	CPPUNIT_ASSERT(!hpnonstop1.HasParent());

	const CServerPath hpnonstop2(_T("\\mysys.$myvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop2.HasParent());

	const CServerPath hpnonstop3(_T("\\mysys.$myvol.mysubvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop3.HasParent());

	const CServerPath dos_virtual1(_T("\\"));
	CPPUNIT_ASSERT(!dos_virtual1.HasParent());

	const CServerPath dos_virtual2(_T("\\foo"));
	CPPUNIT_ASSERT(dos_virtual2.HasParent());

	const CServerPath dos_virtual3(_T("\\foo\\bar"));
	CPPUNIT_ASSERT(dos_virtual3.HasParent());
}

void CServerPathTest::testGetParent()
{
	const CServerPath unix1(_T("/"));
	const CServerPath unix2(_T("/foo"));
	const CServerPath unix3(_T("/foo/bar"));
	CPPUNIT_ASSERT(unix2.GetParent() == unix1);
	CPPUNIT_ASSERT(unix3.GetParent() == unix2);

	const CServerPath vms1(_T("FOO:[BAR]"));
	const CServerPath vms2(_T("FOO:[BAR.TEST]"));
	const CServerPath vms3(_T("FOO:[BAR^.TEST]"));
	const CServerPath vms4(_T("FOO:[BAR^.TEST.SOMETHING]"));
	CPPUNIT_ASSERT(vms2.GetParent() == vms1);
	CPPUNIT_ASSERT(vms4.GetParent() == vms3);

	const CServerPath dos1(_T("C:\\"));
	const CServerPath dos2(_T("C:\\FOO"));
	CPPUNIT_ASSERT(dos2.GetParent() == dos1);

	const CServerPath mvs1(_T("'FOO'"), MVS);
	const CServerPath mvs2(_T("'FOO.'"), MVS);
	const CServerPath mvs3(_T("'FOO.BAR'"), MVS);
	const CServerPath mvs4(_T("'FOO.BAR.'"), MVS);
	CPPUNIT_ASSERT(mvs3.GetParent() == mvs2);
	CPPUNIT_ASSERT(mvs4.GetParent() == mvs2);

	const CServerPath vxworks1(_T(":foo:"));
	const CServerPath vxworks2(_T(":foo:bar"));
	const CServerPath vxworks3(_T(":foo:bar/test"));
	CPPUNIT_ASSERT(vxworks2.GetParent() == vxworks1);
	CPPUNIT_ASSERT(vxworks3.GetParent() == vxworks2);

	// ZVM is same as Unix, only makes difference in directory
	// listing parser.
	const CServerPath zvm1(_T("/"), ZVM);
	const CServerPath zvm2(_T("/foo"), ZVM);
	const CServerPath zvm3(_T("/foo/bar"), ZVM);
	CPPUNIT_ASSERT(zvm2.GetParent() == zvm1);
	CPPUNIT_ASSERT(zvm3.GetParent() == zvm2);

	const CServerPath hpnonstop1(_T("\\mysys"), HPNONSTOP);
	const CServerPath hpnonstop2(_T("\\mysys.$myvol"), HPNONSTOP);
	const CServerPath hpnonstop3(_T("\\mysys.$myvol.mysubvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop2.GetParent() == hpnonstop1);
	CPPUNIT_ASSERT(hpnonstop3.GetParent() == hpnonstop2);

	const CServerPath dos_virtual1(_T("\\"));
	const CServerPath dos_virtual2(_T("\\foo"));
	const CServerPath dos_virtual3(_T("\\foo\\bar"));
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
	const CServerPath mvs6(_T("'FOO.BAR.BAZ'"), MVS);
	CPPUNIT_ASSERT(mvs1.GetCommonParent(mvs2) == CServerPath());
	CPPUNIT_ASSERT(mvs3.GetCommonParent(mvs1) == CServerPath());
	CPPUNIT_ASSERT(mvs4.GetCommonParent(mvs3) == mvs2);
	CPPUNIT_ASSERT(mvs3.GetCommonParent(mvs4) == mvs2);
	CPPUNIT_ASSERT(mvs3.GetCommonParent(mvs2) == mvs2);
	CPPUNIT_ASSERT(mvs4.GetCommonParent(mvs2) == mvs2);
	CPPUNIT_ASSERT(mvs5.GetCommonParent(mvs2) == CServerPath());
	CPPUNIT_ASSERT(mvs6.GetCommonParent(mvs4) == mvs4);
	CPPUNIT_ASSERT(mvs6.GetCommonParent(mvs3) == mvs2);

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

void CServerPathTest::testFormatFilename()
{
	const CServerPath unix1(_T("/"));
	const CServerPath unix2(_T("/foo"));
	CPPUNIT_ASSERT(unix1.FormatFilename(_T("bar"), false) == _T("/bar"));
	CPPUNIT_ASSERT(unix2.FormatFilename(_T("bar"), false) == _T("/foo/bar"));

	const CServerPath vms1(_T("FOO:[BAR]"));
	CPPUNIT_ASSERT(vms1.FormatFilename(_T("BAZ"), false) == _T("FOO:[BAR]BAZ"));

	const CServerPath dos1(_T("C:\\"));
	const CServerPath dos2(_T("C:\\foo"));
	CPPUNIT_ASSERT(dos1.FormatFilename(_T("bar"), false) == _T("C:\\bar"));
	CPPUNIT_ASSERT(dos2.FormatFilename(_T("bar"), false) == _T("C:\\foo\\bar"));

	const CServerPath mvs1(_T("'FOO.BAR'"), MVS);
	const CServerPath mvs2(_T("'FOO.BAR.'"), MVS);
	CPPUNIT_ASSERT(mvs1.FormatFilename(_T("BAZ"), false) == _T("'FOO.BAR(BAZ)'"));
	CPPUNIT_ASSERT(mvs2.FormatFilename(_T("BAZ"), false) == _T("'FOO.BAR.BAZ'"));

	const CServerPath vxworks1(_T(":foo:"));
	const CServerPath vxworks2(_T(":foo:bar"));
	const CServerPath vxworks3(_T(":foo:bar/test"));
	CPPUNIT_ASSERT(vxworks1.FormatFilename(_T("baz"), false) == _T(":foo:baz"));
	CPPUNIT_ASSERT(vxworks2.FormatFilename(_T("baz"), false) == _T(":foo:bar/baz"));
	CPPUNIT_ASSERT(vxworks3.FormatFilename(_T("baz"), false) == _T(":foo:bar/test/baz"));

	// ZVM is same as Unix, only makes difference in directory
	// listing parser.
	const CServerPath zvm1(_T("/"));
	const CServerPath zvm2(_T("/foo"));
	CPPUNIT_ASSERT(zvm1.FormatFilename(_T("bar"), false) == _T("/bar"));
	CPPUNIT_ASSERT(zvm2.FormatFilename(_T("bar"), false) == _T("/foo/bar"));

	const CServerPath hpnonstop1(_T("\\mysys"), HPNONSTOP);
	const CServerPath hpnonstop2(_T("\\mysys.$myvol"), HPNONSTOP);
	const CServerPath hpnonstop3(_T("\\mysys.$myvol.mysubvol"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop1.FormatFilename(_T("foo"), false) == _T("\\mysys.foo"));
	CPPUNIT_ASSERT(hpnonstop2.FormatFilename(_T("foo"), false) == _T("\\mysys.$myvol.foo"));
	CPPUNIT_ASSERT(hpnonstop3.FormatFilename(_T("foo"), false) == _T("\\mysys.$myvol.mysubvol.foo"));

	const CServerPath dos_virtual1(_T("/"));
	const CServerPath dos_virtual2(_T("/foo"));
	CPPUNIT_ASSERT(dos_virtual1.FormatFilename(_T("bar"), false) == _T("/bar"));
	CPPUNIT_ASSERT(dos_virtual2.FormatFilename(_T("bar"), false) == _T("/foo/bar"));
}
