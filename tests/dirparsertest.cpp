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

void CDirectoryListingParserTest::InitEntries()
{
	const int year = wxDateTime::Now().GetYear();
	const int month = wxDateTime::Now().GetMonth();

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
				{1994, 4, 8},
				{0, 0},
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
				{(3 > month) ? year - 1 : year, 3, 29},
				{3, 26},
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
				{1994, 4, 8},
				{0, 0},
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
				{year, 1, 25},
				{0, 17},
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
				{2000, 9, 26},
				{0, 0},
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
				{(9 > month) ? year - 1 : year, 9, 26},
				{13, 45},
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
				{2005, 6, 7},
				{21, 22},
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
				{(10 > month) ? year - 1 : year, 10, 5},
				{21, 22},
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
				{(1 > month) ? year - 1 : year, 1, 16},
				{18, 53},
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
				{(10 > month) ? year - 1 : year, 10, 20},
				{15, 27},
				false
			}
		});
		
	// NetPresenz for the Mac
	// ----------------------

	// Actually this one isn't parse properly:
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
				{1995, 11, 22},
				{0, 0},
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
				{1996, 5, 10},
				{0, 0},
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
				{(1 > month) ? year - 1 : year, 1, 29},
				{3, 26},
				false
			}
		});

	// EPLF directory listings
	// -----------------------

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
				{1996, 2, 1},
				{23, 15},
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
				{1996, 1, 14},
				{0, 58},
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
				{2000, 4, 27},
				{12, 9},
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
				{2000, 4, 6},
				{15, 47},
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
				{2002, 9, 2},
				{18, 48},
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
				{2002, 9, 2},
				{19, 6},
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
				{1973, 10, 29},
				{22, 33},
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
				{2000, 4, 4},
				{21, 6},
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
				{2002, 12, 12},
				{2, 13},
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
				{2002, 10, 8},
				{9, 47},
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
				{2003, 4, 23},
				{10, 57},
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
				{1999, 7, 14},
				{12, 37},
				false
			}
		});

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
				{2003, 2, 11},
				{16, 15},
				false
			}
		});

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
				{2000, 10, 5},
				{23, 38},
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
				{(7 > month) ? year - 1 : year, 7, 26},
				{20, 10},
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
				{2003, 10, 2},
				{0, 0},
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
				{1999, 10, 12},
				{17, 12},
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
				{2003, 4, 24},
				{17, 12},
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
				{2003, 4, 18},
				{0, 0},
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

	struct t_date
	{
		int year;
		int month;
		int day;
	} date;

	struct t_time
	{
		int hour;
		int minute;
	} time;

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

	wxString msg = wxString::Format(_T("Data: %s, count: %d"), wxString(entry.data.c_str(), wxConvUTF8).c_str(), pListing->m_entryCount);
	msg.Replace(_T("\r"), _T(""));
	msg.Replace(_T("\n"), _T(""));
	CPPUNIT_ASSERT_MESSAGE((const char*)msg.mb_str(), pListing->m_entryCount == 1);

	msg = wxString::Format(_T("Data: %s  Expected:\n%s\n  Got:\n%s"), wxString(entry.data.c_str(), wxConvUTF8).c_str(), entry.reference.dump().c_str(), pListing->m_pEntries[0].dump().c_str());

	CPPUNIT_ASSERT_MESSAGE((const char*)msg.mb_str(), pListing->m_pEntries[0] == entry.reference);

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

	CPPUNIT_ASSERT(pListing->m_entryCount == m_entries.size());

	unsigned int i = 0;
	for (std::vector<t_entry>::const_iterator iter = m_entries.begin(); iter != m_entries.end(); iter++, i++)
	{
		wxString msg = wxString::Format(_T("Data: %s  Expected:\n%s\n  Got:\n%s"), wxString(iter->data.c_str(), wxConvUTF8).c_str(), iter->reference.dump().c_str(), pListing->m_pEntries[i].dump().c_str());

		CPPUNIT_ASSERT_MESSAGE((const char*)msg.mb_str(), pListing->m_pEntries[i] == iter->reference);
	}


	delete pListing;
}

void CDirectoryListingParserTest::setUp()
{
}
