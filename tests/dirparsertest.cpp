#include "FileZilla.h"
#include "directorylistingparser.h"
#include <cppunit/extensions/HelperMacros.h>
#include <list>

typedef struct
{
	std::string data;
	CDirentry reference;
} t_entry;

class CDirectoryListingParserTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(CDirectoryListingParserTest);
	CPPUNIT_TEST(testIndividual);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

	void testIndividual();

	std::vector<t_entry> m_entries;
};

CPPUNIT_TEST_SUITE_REGISTRATION(CDirectoryListingParserTest);

void CDirectoryListingParserTest::setUp()
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

void CDirectoryListingParserTest::tearDown()
{
	m_entries.clear();
}

void CDirectoryListingParserTest::testIndividual()
{
	const CServer server;
	for (std::vector<t_entry>::const_iterator iter = m_entries.begin(); iter != m_entries.end(); iter++)
	{
		CDirectoryListingParser parser(0, server);

		const char* str = iter->data.c_str();
		const int len = strlen(str);
		char* data = new char[len];
		strcpy(data, str);
		parser.AddData(data, len);

		CDirectoryListing* pListing = parser.Parse(CServerPath());

		wxString msg = wxString::Format(_T("Data: %s, count: %d"), wxString(iter->data.c_str(), wxConvUTF8).c_str(), pListing->m_entryCount);
		msg.Replace(_T("\n"), _T(""));
		CPPUNIT_ASSERT_MESSAGE((const char*)msg.mb_str(), pListing->m_entryCount == 1);

		msg = wxString::Format(_T("Data: %s  Expected:\n%s\n  Got:\n%s"), wxString(iter->data.c_str(), wxConvUTF8).c_str(), iter->reference.dump().c_str(), pListing->m_pEntries[0].dump().c_str());

		CPPUNIT_ASSERT_MESSAGE((const char*)msg.mb_str(), pListing->m_pEntries[0] == iter->reference);
	}
}
