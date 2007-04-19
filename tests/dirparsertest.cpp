#include "FileZilla.h"
#include "directorylistingparser.h"
#include <cppunit/extensions/HelperMacros.h>
#include <list>

/*
 * This testsuite asserts the correctness of the directory listing parser.
 * It's main purpose is to ensure that all known formats are recognized and
 * parsed as expected. Due to the high amount of variety and unfortunately
 * also ambiguity, the parser is very fragile.
 */

typedef struct
{
	std::string data;
	CDirentry reference;
} t_entry;

class CDirectoryListingParserTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(CDirectoryListingParserTest);
	InitEntries();
	for (unsigned int i = 0; i < m_entries.size(); i++)
		CPPUNIT_TEST(testIndividual);
	CPPUNIT_TEST(testAll);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown() {}

	void testIndividual();
	void testAll();

	static std::vector<t_entry> m_entries;

	static wxCriticalSection m_sync;

protected:
	static void InitEntries();

	t_entry m_entry;
};

wxCriticalSection CDirectoryListingParserTest::m_sync;
std::vector<t_entry> CDirectoryListingParserTest::m_entries;

CPPUNIT_TEST_SUITE_REGISTRATION(CDirectoryListingParserTest);

static int calcYear(int month, int day)
{
	const int cur_year = wxDateTime::GetCurrentYear();
	const int cur_month = wxDateTime::GetCurrentMonth() + 1;
	const int cur_day = wxDateTime::Now().GetDay();

	// Not exact but good enough for our purpose
	const int day_of_year = month * 31 + day;
	const int cur_day_of_year = cur_month * 31 + cur_day;
	if (day_of_year > cur_day_of_year)
		return cur_year - 1;
	else
		return cur_year;
}

void CDirectoryListingParserTest::InitEntries()
{
	// Unix-style listings
        // -------------------

	// We start with a perfect example of a unix style directory listing without anomalies.
	m_entries.push_back((t_entry){
			"dr-xr-xr-x   2 root     other        512 Apr  8  1994 01-unix-std dir",
			{
				_T("01-unix-std dir"),
				512,
				_T("dr-xr-xr-x"),
				_T("root other"),
				true,
				false,
				_T(""),
				true,
				false,
				wxDateTime(8, wxDateTime::Apr, 1994),
				false
			}
		});

	// This one is a recent file with a time instead of the year.
	m_entries.push_back((t_entry){
			"-rw-r--r--   1 root     other        531 3 29 03:26 02-unix-std file",
			{
				_T("02-unix-std file"),
				531,
				_T("-rw-r--r--"),
				_T("root other"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(29, wxDateTime::Mar, calcYear(3, 29), 3, 26),
				false
			}
		});

	// Group omitted
	m_entries.push_back((t_entry){
			"dr-xr-xr-x   2 root                  512 Apr  8  1994 03-unix-nogroup dir",
			{
				_T("03-unix-nogroup dir"),
				512,
				_T("dr-xr-xr-x"),
				_T("root"),
				true,
				false,
				_T(""),
				true,
				false,
				wxDateTime(8, wxDateTime::Apr, 1994),
				false
			}
		});

	// Symbolic link
	m_entries.push_back((t_entry){
			"lrwxrwxrwx   1 root     other          7 Jan 25 00:17 04-unix-std link -> usr/bin",
			{
				_T("04-unix-std link"),
				7,
				_T("lrwxrwxrwx"),
				_T("root other"),
				true,
				true,
				_T("usr/bin"),
				true,
				true,
				wxDateTime(25, wxDateTime::Jan, calcYear(1, 25), 0, 17),
				false
			}
		});

	// Some listings with uncommon date/time format
	// --------------------------------------------

	m_entries.push_back((t_entry){
			"-rw-r--r--   1 root     other        531 09-26 2000 05-unix-date file",
			{
				_T("05-unix-date file"),
				531,
				_T("-rw-r--r--"),
				_T("root other"),
				false,
				false,
				_T(""),
				true,
				false,
				wxDateTime(26, wxDateTime::Sep, 2000),
				false
			}
		});

	m_entries.push_back((t_entry){
			"-rw-r--r--   1 root     other        531 09-26 13:45 06-unix-date file",
			{
				_T("06-unix-date file"),
				531,
				_T("-rw-r--r--"),
				_T("root other"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(26, wxDateTime::Sep, calcYear(9, 26), 13, 45),
				false
			}
		});

	m_entries.push_back((t_entry){
			"-rw-r--r--   1 root     other        531 2005-06-07 21:22 07-unix-date file",
			{
				_T("07-unix-date file"),
				531,
				_T("-rw-r--r--"),
				_T("root other"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(7, wxDateTime::Jun, 2005, 21, 22),
				false
			}
		});


	// Unix style with size information in kilobytes
	m_entries.push_back((t_entry){
			"-rw-r--r--   1 root     other  33.5k Oct 5 21:22 08-unix-namedsize file",
			{
				_T("08-unix-namedsize file"),
				335 * 1024 / 10,
				_T("-rw-r--r--"),
				_T("root other"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(5, wxDateTime::Oct, calcYear(10, 5), 21, 22),
				false
			}
		});

	// NetWare style listings
	// ----------------------

	m_entries.push_back((t_entry){
			"d [R----F--] supervisor            512       Jan 16 18:53    09-netware dir",
			{
				_T("09-netware dir"),
				512,
				_T("d [R----F--]"),
				_T("supervisor"),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(16, wxDateTime::Jan, calcYear(1, 16), 18, 53),
				false
			}
		});

	m_entries.push_back((t_entry){
			"- [R----F--] rhesus             214059       Oct 20 15:27    10-netware file",
			{
				_T("10-netware file"),
				214059,
				_T("- [R----F--]"),
				_T("rhesus"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(20, wxDateTime::Oct, calcYear(10, 20), 15, 27),
				false
			}
		});
		
	// NetPresenz for the Mac
	// ----------------------

	// Actually this one isn't parsed properly:
	// The numerical username is mistaken as size. However,
	// this is ambiguous to the normal unix style listing.
	// It's not possible to recognize both formats the right way.
	m_entries.push_back((t_entry){
			"-------r--         326  1391972  1392298 Nov 22  1995 11-netpresenz file",
			{
				_T("11-netpresenz file"),
				1392298,
				_T("-------r--"),
				_T("1391972"),
				false,
				false,
				_T(""),
				true,
				false,
				wxDateTime(22, wxDateTime::Nov, 1995),
				false
			}
		});

	m_entries.push_back((t_entry){
			"drwxrwxr-x               folder        2 May 10  1996 12-netpresenz dir",
			{
				_T("12-netpresenz dir"),
				2,
				_T("drwxrwxr-x"),
				_T("folder"),
				true,
				false,
				_T(""),
				true,
				false,
				wxDateTime(10, wxDateTime::May, 1996),
				false
			}
		});

	// A format with domain field some windows servers send
	m_entries.push_back((t_entry){
			"-rw-r--r--   1 group domain user 531 Jan 29 03:26 13-unix-domain file",
			{
				_T("13-unix-domain file"),
				531,
				_T("-rw-r--r--"),
				_T("group domain user"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(29, wxDateTime::Jan, calcYear(1, 29), 3, 26),
				false
			}
		});

	// EPLF directory listings
	// -----------------------

	// See http://cr.yp.to/ftp/list/eplf.html (mirrored at http://filezilla-project.org/specs/eplf.html)

	m_entries.push_back((t_entry){
			"+i8388621.48594,m825718503,r,s280,up755\t14-eplf file",
			{
				_T("14-eplf file"),
				280,
				_T("755"),
				_T(""),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(1, wxDateTime::Mar, 1996, 23, 15, 3),
				false
			}
		});

	m_entries.push_back((t_entry){
        	"+i8388621.50690,m824255907,/,\t15-eplf dir",
			{
				_T("15-eplf dir"),
				-1,
				_T(""),
				_T(""),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(14, wxDateTime::Feb, 1996, 0, 58, 27),
				false
			}
		});

	// MSDOS type listing used by old IIS
	// ----------------------------------

	m_entries.push_back((t_entry){
			"04-27-00  12:09PM       <DIR>          16-dos-dateambiguous dir",
			{
				_T("16-dos-dateambiguous dir"),
				-1,
				_T(""),
				_T(""),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(27, wxDateTime::Apr, 2000, 12, 9),
				false
			}
		});

	// Ambiguous date and AM/PM crap. Some evil manager must have forced the poor devs to implement this
	m_entries.push_back((t_entry){
			"04-06-00  03:47PM                  589 17-dos-dateambiguous file",
			{
				_T("17-dos-dateambiguous file"),
				589,
				_T(""),
				_T(""),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(6, wxDateTime::Apr, 2000, 15, 47),
				false
			}
		});

	m_entries.push_back((t_entry){
			"2002-09-02  18:48       <DIR>          18-dos-longyear dir",
			{
				_T("18-dos-longyear dir"),
				-1,
				_T(""),
				_T(""),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(2, wxDateTime::Sep, 2002, 18, 48),
				false
			}
		});

	m_entries.push_back((t_entry){
			"2002-09-02  19:06                9,730 19-dos-longyear file",
			{
				_T("19-dos-longyear file"),
				9730,
				_T(""),
				_T(""),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(2, wxDateTime::Sep, 2002, 19, 6),
				false
			}
		});

	// Numerical unix style listing
	m_entries.push_back((t_entry){
			"0100644   500  101   12345    123456789       20-unix-numerical file",
			{
				_T("20-unix-numerical file"),
				12345,
				_T("0100644"),
				_T("500 101"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(29, wxDateTime::Nov, 1973, 22, 33, 9),
				false
			}
		});

	// VShell servers
	// --------------

	m_entries.push_back((t_entry){
			"206876  Apr 04, 2000 21:06 21-vshell-old file",
			{
				_T("21-vshell-old file"),
				206876,
				_T(""),
				_T(""),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(4, wxDateTime::Apr, 2000, 21, 6),
				false
			}
		});
	
	m_entries.push_back((t_entry){
			"0  Dec 12, 2002 02:13 22-vshell-old dir/",
			{
				_T("22-vshell-old dir"),
				0,
				_T(""),
				_T(""),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(12, wxDateTime::Dec, 2002, 2, 13),
				false
			}
		});
	
	/* This type of directory listings is sent by some newer versions of VShell
	 * both year and time in one line is uncommon. */
	m_entries.push_back((t_entry){
			"-rwxr-xr-x    1 user group        9 Oct 08, 2002 09:47 23-vshell-new file",
			{
				_T("23-vshell-new file"),
				9,
				_T("-rwxr-xr-x"),
				_T("user group"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(8, wxDateTime::Oct, 2002, 9, 47),
				false
			}
		});

	// OS/2 server format
	// ------------------
	
	// This server obviously isn't Y2K aware
	m_entries.push_back((t_entry){
			"36611      A    04-23-103  10:57  24-os2 file",
			{
				_T("24-os2 file"),
				36611,
				_T(""),
				_T(""),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(23, wxDateTime::Apr, 2003, 10, 57),
				false
			}
		});

	m_entries.push_back((t_entry){
			" 1123      A    07-14-99   12:37  25-os2 file",
			{
				_T("25-os2 file"),
				1123,
				_T(""),
				_T(""),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(14, wxDateTime::Jul, 1999, 12, 37),
				false
			}
		});

	// Another server not aware of Y2K
	m_entries.push_back((t_entry){
			"    0 DIR       02-11-103  16:15  26-os2 dir",
			{
				_T("26-os2 dir"),
				0,
				_T(""),
				_T(""),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(11, wxDateTime::Feb, 2003, 16, 15),
				false
			}
		});

	// Again Y2K
	m_entries.push_back((t_entry){
			" 1123 DIR  A    10-05-100  23:38  27-os2 dir",
			{
				_T("27-os2 dir"),
				1123,
				_T(""),
				_T(""),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(5, wxDateTime::Oct, 2000, 23, 38),
				false
			}
		});

	// Localized date formats
	// ----------------------

	m_entries.push_back((t_entry){
			"dr-xr-xr-x   2 root     other      2235 26. Juli, 20:10 28-datetest-ger dir",
			{
				_T("28-datetest-ger dir"),
				2235,
				_T("dr-xr-xr-x"),
				_T("root other"),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(26, wxDateTime::Jul, calcYear(7, 26), 20, 10),
				false
			}
		});

	m_entries.push_back((t_entry){
			"-r-xr-xr-x   2 root     other      2235 2.   Okt.  2003 29-datetest-ger file",
			{
				_T("29-datetest-ger file"),
				2235,
				_T("-r-xr-xr-x"),
				_T("root other"),
				false,
				false,
				_T(""),
				true,
				false,
				wxDateTime(2, wxDateTime::Oct, 2003),
				false
			}
		});

	m_entries.push_back((t_entry){
			"-r-xr-xr-x   2 root     other      2235 1999/10/12 17:12 30-datetest file",
			{
				_T("30-datetest file"),
				2235,
				_T("-r-xr-xr-x"),
				_T("root other"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(12, wxDateTime::Oct, 1999, 17, 12),
				false
			}
		});

	m_entries.push_back((t_entry){
			"-r-xr-xr-x   2 root     other      2235 24-04-2003 17:12 31-datetest file",
			{
				_T("31-datetest file"),
				2235,
				_T("-r-xr-xr-x"),
				_T("root other"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(24, wxDateTime::Apr, 2003, 17, 12),
				false
			}
		});

	// Japanese listing
	// Remark: I'v no idea in which encoding the foreign characters are, but
	// it's not valid UTF-8. Parser has to be able to cope with it somehow.
	m_entries.push_back((t_entry){
			"-rw-r--r--   1 root       sys           8473  4\x8c\x8e 18\x93\xfa 2003\x94\x4e 32-datatest-japanese file",
			{
				_T("32-datatest-japanese file"),
				8473,
				_T("-rw-r--r--"),
				_T("root sys"),
				false,
				false,
				_T(""),
				true,
				false,
				wxDateTime(18, wxDateTime::Apr, 2003),
				false
			}
		});

	// Some other asian listing format. Those >127 chars are just examples

	m_entries.push_back((t_entry){
			"-rwxrwxrwx   1 root     staff          0 2003   3\xed\xef 20 33-asian date file",
			{
				_T("33-asian date file"),
				0,
				_T("-rwxrwxrwx"),
				_T("root staff"),
				false,
				false,
				_T(""),
				true,
				false,
				wxDateTime(20, wxDateTime::Mar, 2003),
				false
			}
		});

	m_entries.push_back((t_entry){
			"-r--r--r-- 1 root root 2096 8\xed 17 08:52 34-asian date file",
			{
				_T("34-asian date file"),
				2096,
				_T("-r--r--r--"),
				_T("root root"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(17, wxDateTime::Aug, calcYear(8, 17), 8, 52),
				false
			}
		});

	m_entries.push_back((t_entry){
			"-r-xr-xr-x   2 root  root  96 2004.07.15   35-dotted-date file",
			{
				_T("35-dotted-date file"),
				96,
				_T("-r-xr-xr-x"),
				_T("root root"),
				false,
				false,
				_T(""),
				true,
				false,
				wxDateTime(15, wxDateTime::Jul, 2004),
				false
			}
		});

	// VMS listings
	// ------------

	m_entries.push_back((t_entry){
			"36-vms-dir.DIR;1  1 19-NOV-2001 21:41 [root,root] (RWE,RWE,RE,RE)",
			{
				_T("36-vms-dir;1"),
				1,
				_T("RWE,RWE,RE,RE"),
				_T("root,root"),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(19, wxDateTime::Nov, 2001, 21, 41),
				false
			}
		});


	m_entries.push_back((t_entry){
			"37-vms-file;1       155   2-JUL-2003 10:30:13.64",
			{
				_T("37-vms-file;1"),
				155,
				_T(""),
				_T(""),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(2, wxDateTime::Jul, 2003, 10, 30),
				false
			}
		});

	/* VMS style listing without time */
	m_entries.push_back((t_entry){
			"38-vms-notime-file;1    2/8    7-JAN-2000    [IV2_XXX]   (RWED,RWED,RE,)",
			{
				_T("38-vms-notime-file;1"),
				2,
				_T("RWED,RWED,RE,"),
				_T("IV2_XXX"),
				false,
				false,
				_T(""),
				true,
				false,
				wxDateTime(7, wxDateTime::Jan, 2000),
				false
			}
		});

	/* Localized month */
	m_entries.push_back((t_entry){
			"39-vms-notime-file;1    6/8    15-JUI-2002    PRONAS   (RWED,RWED,RE,)",
			{
				_T("39-vms-notime-file;1"),
				6,
				_T("RWED,RWED,RE,"),
				_T("PRONAS"),
				false,
				false,
				_T(""),
				true,
				false,
				wxDateTime(15, wxDateTime::Jul, 2002),
				false
			}
		});

	m_entries.push_back((t_entry){
			"40-vms-multiline-file;1\r\n170774/170775     24-APR-2003 08:16:15  [FTP_CLIENT,SCOT]      (RWED,RWED,RE,)",
			{
				_T("40-vms-multiline-file;1"),
				170774,
				_T("RWED,RWED,RE,"),
				_T("FTP_CLIENT,SCOT"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(24, wxDateTime::Apr, 2003, 8, 16),
				false
			}
		});

	m_entries.push_back((t_entry){
			"41-vms-multiline-file;1\r\n10     2-JUL-2003 10:30:08.59  [FTP_CLIENT,SCOT]      (RWED,RWED,RE,)",
			{
				_T("41-vms-multiline-file;1"),
				10,
				_T("RWED,RWED,RE,"),
				_T("FTP_CLIENT,SCOT"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(2, wxDateTime::Jul, 2003, 10, 30),
				false
			}
		});

	// VMS style listings with a different field order
	m_entries.push_back((t_entry){
			"42-vms-alternate-field-order-file;1   [SUMMARY]    1/3     2-AUG-2006 13:05  (RWE,RWE,RE,)",
			{
				_T("42-vms-alternate-field-order-file;1"),
				1,
				_T("RWE,RWE,RE,"),
				_T("SUMMARY"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(2, wxDateTime::Aug, 2006, 13, 5),
				false
			}
		});

	m_entries.push_back((t_entry){
			"43-vms-alternate-field-order-file;1       17-JUN-1994 17:25:37     6308/13     (RWED,RWED,R,)",
			{
				_T("43-vms-alternate-field-order-file;1"),
				6308,
				_T("RWED,RWED,R,"),
				_T(""),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(17, wxDateTime::Jun, 1994, 17, 25),
				false
			}
		});

	// Miscellaneous listings
	// ----------------------

	/* IBM AS/400 style listing */
	m_entries.push_back((t_entry){
			"QSYS            77824 02/23/00 15:09:55 *DIR 44-ibm-as400 dir/",
			{
				_T("44-ibm-as400 dir"),
				77824,
				_T(""),
				_T("QSYS"),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(23, wxDateTime::Feb, 2000, 15, 9),
				false
			}
		});

	m_entries.push_back((t_entry){
			"QSYS            77824 23/02/00 15:09:55 *FILE 45-ibm-as400-date file",
			{
				_T("45-ibm-as400-date file"),
				77824,
				_T(""),
				_T("QSYS"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(23, wxDateTime::Feb, 2000, 15, 9),
				false
			}
		});

	/* aligned directory listing with too long size */
	m_entries.push_back((t_entry){
			"-r-xr-xr-x longowner longgroup123456 Feb 12 17:20 46-unix-concatsize file",
			{
				_T("46-unix-concatsize file"),
				123456,
				_T("-r-xr-xr-x"),
				_T("longowner longgroup"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(12, wxDateTime::Feb, calcYear(2, 12), 17, 20),
				false
			}
		});

	/* short directory listing with month name */
	m_entries.push_back((t_entry){
			"-r-xr-xr-x 2 owner group 4512 01-jun-99 47_unix_shortdatemonth file",
			{
				_T("47_unix_shortdatemonth file"),
				4512,
				_T("-r-xr-xr-x"),
				_T("owner group"),
				false,
				false,
				_T(""),
				true,
				false,
				wxDateTime(1, wxDateTime::Jun, 1999),
				false
			}
		});

	/* Nortel wfFtp router */
	m_entries.push_back((t_entry){
			"48-nortel-wfftp-file       1014196  06/03/04  Thur.   10:20:03",
			{
				_T("48-nortel-wfftp-file"),
				1014196,
				_T(""),
				_T(""),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(3, wxDateTime::Jun, 2004, 10, 20),
				false
			}
		});

	/* VxWorks based server used in Nortel routers */
	m_entries.push_back((t_entry){
			"2048    Feb-28-1998  05:23:30   49-nortel-vxworks dir <DIR>",
			{
				_T("49-nortel-vxworks dir"),
				2048,
				_T(""),
				_T(""),
				true,
				false,
				_T(""),
				true,
				true,
				wxDateTime(28, wxDateTime::Feb, 1998, 5, 23),
				false
			}
		});

	/* the following format is sent by the Connect:Enterprise server by Sterling Commerce */
	m_entries.push_back((t_entry){
			"-C--E-----FTP B BCC3I1       7670  1294495 Jan 13 07:42 50-conent file",
			{
				_T("50-conent file"),
				1294495,
				_T("-C--E-----FTP"),
				_T("B BCC3I1 7670"),
				false,
				false,
				_T(""),
				true,
				true,
				wxDateTime(13, wxDateTime::Jan, calcYear(1, 13), 7, 42),
				false
			}
		});

	/* Microware OS-9
	 * Notice the yy/mm/dd date format */
	m_entries.push_back((t_entry){
			"20.20 07/03/29 1026 d-ewrewr 2650 85920 51-OS-9 dir",
			{
				_T("51-OS-9 dir"),
				85920,
				_T("d-ewrewr"),
				_T("20.20"),
				true,
				false,
				_T(""),
				true,
				false,
				wxDateTime(29, wxDateTime::Mar, 2007),
				false
			}
		});

/*
	wxString name;
	wxLongLong size;
	wxString permissions;
	wxString ownerGroup;
	bool dir;
	bool link;
	wxString target; // Set to linktarget it link is true

	bool hasDate;
	bool hasTime;
	wxDateTime time;

	bool unsure;
*/

	// Fix line endings
	for (std::vector<t_entry>::iterator iter = m_entries.begin(); iter != m_entries.end(); iter++)
		iter->data += "\r\n";
}

void CDirectoryListingParserTest::testIndividual()
{
	m_sync.Enter();

	static int index = 0;
	const t_entry &entry = m_entries[index++];

	m_sync.Leave();

	const CServer server;

	CDirectoryListingParser parser(0, server);

	const char* str = entry.data.c_str();
	const int len = strlen(str);
	char* data = new char[len];
	memcpy(data, str, len);
	parser.AddData(data, len);

	CDirectoryListing* pListing = parser.Parse(CServerPath());

	wxString msg = wxString::Format(_T("Data: %s, count: %d"), wxString(entry.data.c_str(), wxConvUTF8).c_str(), pListing->GetCount());
	msg.Replace(_T("\r"), _T(""));
	msg.Replace(_T("\n"), _T(""));
	CPPUNIT_ASSERT_MESSAGE((const char*)msg.mb_str(), pListing->GetCount() == 1);

	msg = wxString::Format(_T("Data: %s  Expected:\n%s\n  Got:\n%s"), wxString(entry.data.c_str(), wxConvUTF8).c_str(), entry.reference.dump().c_str(), (*pListing)[0].dump().c_str());

	CPPUNIT_ASSERT_MESSAGE((const char*)msg.mb_str(), (*pListing)[0] == entry.reference);

	delete pListing;
}

void CDirectoryListingParserTest::testAll()
{
	const CServer server;
	CDirectoryListingParser parser(0, server);
	for (std::vector<t_entry>::const_iterator iter = m_entries.begin(); iter != m_entries.end(); iter++)
	{
		const char* str = iter->data.c_str();
		const int len = strlen(str);
		char* data = new char[len];
		memcpy(data, str, len);
		parser.AddData(data, len);
	}
	CDirectoryListing* pListing = parser.Parse(CServerPath());

	CPPUNIT_ASSERT(pListing->GetCount() == m_entries.size());

	unsigned int i = 0;
	for (std::vector<t_entry>::const_iterator iter = m_entries.begin(); iter != m_entries.end(); iter++, i++)
	{
		wxString msg = wxString::Format(_T("Data: %s  Expected:\n%s\n  Got:\n%s"), wxString(iter->data.c_str(), wxConvUTF8).c_str(), iter->reference.dump().c_str(), (*pListing)[i].dump().c_str());

		CPPUNIT_ASSERT_MESSAGE((const char*)msg.mb_str(), (*pListing)[i] == iter->reference);
	}


	delete pListing;
}

void CDirectoryListingParserTest::setUp()
{
}
