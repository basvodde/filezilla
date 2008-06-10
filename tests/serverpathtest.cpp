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
	CPPUNIT_TEST(testChangePath);
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
	void testChangePath();

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

	const CServerPath cygwin1(_T("/"), CYGWIN);
	CPPUNIT_ASSERT(cygwin1.GetPath() == _T("/"));

	const CServerPath cygwin2(_T("/foo"), CYGWIN);
	CPPUNIT_ASSERT(cygwin2.GetPath() == _T("/foo"));

	const CServerPath cygwin3(_T("//"), CYGWIN);
	CPPUNIT_ASSERT(cygwin3.GetPath() == _T("//"));

	const CServerPath cygwin4(_T("//foo"), CYGWIN);
	CPPUNIT_ASSERT(cygwin4.GetPath() == _T("//foo"));
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

	const CServerPath cygwin1(_T("/"), CYGWIN);
	CPPUNIT_ASSERT(!cygwin1.HasParent());

	const CServerPath cygwin2(_T("/foo"), CYGWIN);
	CPPUNIT_ASSERT(cygwin2.HasParent());

	const CServerPath cygwin3(_T("/foo/bar"), CYGWIN);
	CPPUNIT_ASSERT(cygwin3.HasParent());

	const CServerPath cygwin4(_T("//"), CYGWIN);
	CPPUNIT_ASSERT(!cygwin4.HasParent());

	const CServerPath cygwin5(_T("//foo"), CYGWIN);
	CPPUNIT_ASSERT(cygwin5.HasParent());

	const CServerPath cygwin6(_T("//foo/bar"), CYGWIN);
	CPPUNIT_ASSERT(cygwin6.HasParent());
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

	const CServerPath cygwin1(_T("/"), CYGWIN);
	const CServerPath cygwin2(_T("/foo"), CYGWIN);
	const CServerPath cygwin3(_T("/foo/bar"), CYGWIN);
	const CServerPath cygwin4(_T("//"), CYGWIN);
	const CServerPath cygwin5(_T("//foo"), CYGWIN);
	const CServerPath cygwin6(_T("//foo/bar"), CYGWIN);
	CPPUNIT_ASSERT(cygwin2.GetParent() == cygwin1);
	CPPUNIT_ASSERT(cygwin3.GetParent() == cygwin2);
	CPPUNIT_ASSERT(cygwin5.GetParent() == cygwin4);
	CPPUNIT_ASSERT(cygwin6.GetParent() == cygwin5);
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
	CPPUNIT_ASSERT(unix1.GetCommonParent(unix3) == unix1);

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

	const CServerPath cygwin1(_T("/foo"), CYGWIN);
	const CServerPath cygwin2(_T("/foo/baz"), CYGWIN);
	const CServerPath cygwin3(_T("/foo/bar"), CYGWIN);
	const CServerPath cygwin4(_T("//foo"), CYGWIN);
	const CServerPath cygwin5(_T("//foo/baz"), CYGWIN);
	const CServerPath cygwin6(_T("//foo/bar"), CYGWIN);
	CPPUNIT_ASSERT(cygwin2.GetCommonParent(cygwin3) == cygwin1);
	CPPUNIT_ASSERT(cygwin1.GetCommonParent(cygwin1) == cygwin1);
	CPPUNIT_ASSERT(cygwin1.GetCommonParent(cygwin3) == cygwin1);
	CPPUNIT_ASSERT(cygwin5.GetCommonParent(cygwin6) == cygwin4);
	CPPUNIT_ASSERT(cygwin4.GetCommonParent(cygwin4) == cygwin4);
	CPPUNIT_ASSERT(cygwin4.GetCommonParent(cygwin6) == cygwin4);
	CPPUNIT_ASSERT(cygwin4.GetCommonParent(cygwin1) == CServerPath());
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
	const CServerPath zvm1(_T("/"), ZVM);
	const CServerPath zvm2(_T("/foo"), ZVM);
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

	const CServerPath cygwin1(_T("/"), CYGWIN);
	const CServerPath cygwin2(_T("/foo"), CYGWIN);
	const CServerPath cygwin3(_T("//"), CYGWIN);
	const CServerPath cygwin4(_T("//foo"), CYGWIN);
	CPPUNIT_ASSERT(cygwin1.FormatFilename(_T("bar"), false) == _T("/bar"));
	CPPUNIT_ASSERT(cygwin2.FormatFilename(_T("bar"), false) == _T("/foo/bar"));
	CPPUNIT_ASSERT(cygwin3.FormatFilename(_T("bar"), false) == _T("//bar"));
	CPPUNIT_ASSERT(cygwin4.FormatFilename(_T("bar"), false) == _T("//foo/bar"));
}

void CServerPathTest::testChangePath()
{
	CServerPath unix1(_T("/foo/bar"));
	CServerPath unix2(_T("/foo/bar/baz"));
	CServerPath unix3(_T("/foo/baz"));
	CServerPath unix4(_T("/foo/bar/baz"));
	CServerPath unix5(_T("/foo"));
	CPPUNIT_ASSERT(unix1.ChangePath(_T("baz")) && unix1 == unix2);
	CPPUNIT_ASSERT(unix2.ChangePath(_T("../../baz")) && unix2 == unix3);
	CPPUNIT_ASSERT(unix3.ChangePath(_T("/foo/bar/baz")) && unix3 == unix4);
	CPPUNIT_ASSERT(unix3.ChangePath(_T(".")) && unix3 == unix4);
	wxString sub = _T("../../bar");
	CPPUNIT_ASSERT(unix4.ChangePath(sub, true) && unix4 == unix5 && sub == _T("bar"));
	sub = _T("bar/");
	CPPUNIT_ASSERT(!unix4.ChangePath(sub, true));

	const CServerPath vms1(_T("FOO:[BAR]"));
	CServerPath vms2(_T("FOO:[BAR]"));
	CServerPath vms3(_T("FOO:[BAR.BAZ]"));
	CServerPath vms4(_T("FOO:[BAR]"));
	CServerPath vms5(_T("FOO:[BAR.BAZ.BAR]"));
	CServerPath vms6(_T("FOO:[BAR]"));
	CServerPath vms7(_T("FOO:[BAR]"));
	CServerPath vms8(_T("FOO:[BAR]"));
	CServerPath vms9(_T("FOO:[BAZ.BAR]"));
	CServerPath vms10(_T("DOO:[BAZ.BAR]"));
	CPPUNIT_ASSERT(vms2.ChangePath(_T("BAZ")) && vms2 == vms3);
	CPPUNIT_ASSERT(vms4.ChangePath(_T("BAZ.BAR")) && vms4 == vms5);
	sub = _T("BAZ.BAR");
	CPPUNIT_ASSERT(vms6.ChangePath(sub, true) && vms6 == vms1 && sub == _T("BAZ.BAR"));
	sub = _T("[BAZ.BAR]FOO");
	CPPUNIT_ASSERT(!vms7.ChangePath(sub, false));
	CPPUNIT_ASSERT(vms8.ChangePath(sub, true) && vms8 == vms9 && sub == _T("FOO"));
	CPPUNIT_ASSERT(vms10.ChangePath(_T("FOO:[BAR]")) && vms10 == vms1);

	const CServerPath dos1(_T("c:\\bar"));
	CServerPath dos2(_T("c:\\bar"));
	CServerPath dos3(_T("c:\\bar\\baz"));
	CServerPath dos4(_T("c:\\bar"));
	CServerPath dos5(_T("c:\\bar\\baz\\bar"));
	CServerPath dos6(_T("c:\\bar\\baz\\"));
	CServerPath dos7(_T("d:\\bar"));
	CServerPath dos8(_T("c:\\bar\\baz"));
	CServerPath dos9(_T("c:\\bar\\"));
	CPPUNIT_ASSERT(dos2.ChangePath(_T("baz")) && dos2 == dos3);
	CPPUNIT_ASSERT(dos4.ChangePath(_T("baz\\bar")) && dos4 == dos5);
	CPPUNIT_ASSERT(dos5.ChangePath(_T("\\bar\\")) && dos5 == dos1);
	CPPUNIT_ASSERT(dos6.ChangePath(_T("..\\..\\.\\foo\\..\\bar")) && dos6 == dos1);
	CPPUNIT_ASSERT(dos7.ChangePath(_T("c:\\bar")) && dos7 == dos1);
	sub = _T("\\bar\\foo");
	CPPUNIT_ASSERT(dos8.ChangePath(sub, true) && dos8 == dos1 && sub == _T("foo"));
	sub = _T("baz\\foo");
	CPPUNIT_ASSERT(dos9.ChangePath(sub, true) && dos9 == dos3 && sub == _T("foo"));

	const CServerPath mvs1(_T("'BAR.'"), MVS);
	CServerPath mvs2(_T("'BAR.'"), MVS);
	CServerPath mvs3(_T("'BAR.BAZ.'"), MVS);
	CServerPath mvs4(_T("'BAR.'"), MVS);
	CServerPath mvs5(_T("'BAR.BAZ.BAR'"), MVS);
	CServerPath mvs6(_T("'BAR.'"), MVS);
	CServerPath mvs7(_T("'BAR.'"), MVS);
	CServerPath mvs8(_T("'BAR.'"), MVS);
	CServerPath mvs9(_T("'BAR.BAZ'"), MVS);
	CServerPath mvs10(_T("'BAR.BAZ.'"), MVS);
	CServerPath mvs11(_T("'BAR.BAZ'"), MVS);
	CServerPath mvs12(_T("'BAR.BAZ'"), MVS);
	CPPUNIT_ASSERT(mvs2.ChangePath(_T("BAZ.")) && mvs2 == mvs3);
	CPPUNIT_ASSERT(mvs4.ChangePath(_T("BAZ.BAR")) && mvs4 == mvs5);
	sub = _T("BAZ.BAR");
	CPPUNIT_ASSERT(mvs6.ChangePath(sub, true) && mvs6 == mvs3 && sub == _T("BAR"));
	sub = _T("BAZ.BAR.");
	CPPUNIT_ASSERT(!mvs7.ChangePath(sub, true));
	sub = _T("BAZ(BAR)");
	CPPUNIT_ASSERT(mvs8.ChangePath(sub, true) && mvs8 == mvs9 && sub == _T("BAR"));
	CPPUNIT_ASSERT(!mvs5.ChangePath(_T("(FOO)")));
	CPPUNIT_ASSERT(!mvs9.ChangePath(_T("FOO")));
	sub = _T("(FOO)");
	CPPUNIT_ASSERT(!mvs10.ChangePath(sub));
	sub = _T("(FOO)");
	CPPUNIT_ASSERT(mvs11.ChangePath(sub, true) && mvs11 == mvs12 && sub == _T("FOO"));

	const CServerPath vxworks1(_T(":foo:bar"));
	CServerPath vxworks2(_T(":foo:bar"));
	CServerPath vxworks3(_T(":foo:bar/baz"));
	CServerPath vxworks4(_T(":foo:bar"));
	CServerPath vxworks5(_T(":foo:bar/baz/bar"));
	CServerPath vxworks6(_T(":foo:bar/baz/"));
	CServerPath vxworks7(_T(":bar:bar"));
	CServerPath vxworks8(_T(":foo:bar/baz"));
	CServerPath vxworks9(_T(":foo:bar/baz/bar"));
	CPPUNIT_ASSERT(vxworks2.ChangePath(_T("baz")) && vxworks2 == vxworks3);
	CPPUNIT_ASSERT(vxworks4.ChangePath(_T("baz/bar")) && vxworks4 == vxworks5);
	CPPUNIT_ASSERT(vxworks6.ChangePath(_T("../.././foo/../bar")) && vxworks6 == vxworks1);
	CPPUNIT_ASSERT(vxworks7.ChangePath(_T(":foo:bar")) && vxworks7 == vxworks1);
	sub = _T("bar/foo");
	CPPUNIT_ASSERT(vxworks8.ChangePath(sub, true) && vxworks8 == vxworks9 && sub == _T("foo"));

	// ZVM is same as Unix, only makes difference in directory
	// listing parser.
	CServerPath zvm1(_T("/foo/bar"), ZVM);
	CServerPath zvm2(_T("/foo/bar/baz"), ZVM);
	CServerPath zvm3(_T("/foo/baz"), ZVM);
	CServerPath zvm4(_T("/foo/bar/baz"), ZVM);
	CServerPath zvm5(_T("/foo"), ZVM);
	CPPUNIT_ASSERT(zvm1.ChangePath(_T("baz")) && zvm1 == zvm2);
	CPPUNIT_ASSERT(zvm2.ChangePath(_T("../../baz")) && zvm2 == zvm3);
	CPPUNIT_ASSERT(zvm3.ChangePath(_T("/foo/bar/baz")) && zvm3 == zvm4);
	CPPUNIT_ASSERT(zvm3.ChangePath(_T(".")) && zvm3 == zvm4);
	sub = _T("../../bar");
	CPPUNIT_ASSERT(zvm4.ChangePath(sub, true) && zvm4 == zvm5 && sub == _T("bar"));
	sub = _T("bar/");
	CPPUNIT_ASSERT(!zvm4.ChangePath(sub, true));

	CServerPath hpnonstop1(_T("\\mysys.$myvol"), HPNONSTOP);
	CServerPath hpnonstop2(_T("\\mysys.$myvol.mysubvol"), HPNONSTOP);
	CServerPath hpnonstop3(_T("\\mysys.$myvol2"), HPNONSTOP);
	CServerPath hpnonstop4(_T("\\mysys.$myvol.mysubvol"), HPNONSTOP);
	CServerPath hpnonstop5(_T("\\mysys"), HPNONSTOP);
	CPPUNIT_ASSERT(hpnonstop1.ChangePath(_T("mysubvol")) && hpnonstop1 == hpnonstop2);
	CPPUNIT_ASSERT(hpnonstop3.ChangePath(_T("\\mysys.$myvol.mysubvol")) && hpnonstop3 == hpnonstop4);
	sub = _T("bar");
	CPPUNIT_ASSERT(hpnonstop2.ChangePath(sub, true) && hpnonstop2 == hpnonstop4 && sub == _T("bar"));
	sub = _T("$myvol.mysubvol.bar");
	CPPUNIT_ASSERT(hpnonstop5.ChangePath(sub, true) && hpnonstop5 == hpnonstop4 && sub == _T("bar"));
	sub = _T("bar.");
	CPPUNIT_ASSERT(!hpnonstop4.ChangePath(sub, true));

	CServerPath dos_virtual1(_T("\\foo\\bar"));
	CServerPath dos_virtual2(_T("\\foo\\bar\\baz"));
	CServerPath dos_virtual3(_T("\\foo\\baz"));
	CServerPath dos_virtual4(_T("\\foo\\bar\\baz"));
	CServerPath dos_virtual5(_T("\\foo"));
	CPPUNIT_ASSERT(dos_virtual1.ChangePath(_T("baz")) && dos_virtual1 == dos_virtual2);
	CPPUNIT_ASSERT(dos_virtual2.ChangePath(_T("..\\..\\baz")) && dos_virtual2 == dos_virtual3);
	CPPUNIT_ASSERT(dos_virtual3.ChangePath(_T("\\foo\\bar\\baz")) && dos_virtual3 == dos_virtual4);
	CPPUNIT_ASSERT(dos_virtual3.ChangePath(_T(".")) && dos_virtual3 == dos_virtual4);
	sub = _T("..\\..\\bar");
	CPPUNIT_ASSERT(dos_virtual4.ChangePath(sub, true) && dos_virtual4 == dos_virtual5 && sub == _T("bar"));
	sub = _T("bar\\");
	CPPUNIT_ASSERT(!dos_virtual4.ChangePath(sub, true));

	CServerPath cygwin1(_T("/foo/bar"), CYGWIN);
	CServerPath cygwin2(_T("/foo/bar/baz"), CYGWIN);
	CServerPath cygwin3(_T("/foo/baz"), CYGWIN);
	CServerPath cygwin4(_T("/foo/bar/baz"), CYGWIN);
	CServerPath cygwin5(_T("/foo"), CYGWIN);
	CServerPath cygwin6(_T("//foo"), CYGWIN);
	CServerPath cygwin7(_T("//"), CYGWIN);
	CPPUNIT_ASSERT(cygwin1.ChangePath(_T("baz")) && cygwin1 == cygwin2);
	CPPUNIT_ASSERT(cygwin2.ChangePath(_T("../../baz")) && cygwin2 == cygwin3);
	CPPUNIT_ASSERT(cygwin3.ChangePath(_T("/foo/bar/baz")) && cygwin3 == cygwin4);
	CPPUNIT_ASSERT(cygwin3.ChangePath(_T(".")) && cygwin3 == cygwin4);
	sub = _T("../../bar");
	CPPUNIT_ASSERT(cygwin4.ChangePath(sub, true) && cygwin4 == cygwin5 && sub == _T("bar"));
	sub = _T("bar/");
	CPPUNIT_ASSERT(!cygwin4.ChangePath(sub, true));
	CPPUNIT_ASSERT(cygwin5.ChangePath(_T("//foo")) && cygwin5 == cygwin6);
	sub = _T("//foo");
	CPPUNIT_ASSERT(cygwin1.ChangePath(sub, true) && cygwin1 == cygwin7 && sub == _T("foo"));
}
