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
	void tearDown() {}

	void testIndividual();

	std::vector<t_entry> m_entries;
};

CPPUNIT_TEST_SUITE_REGISTRATION(CDirectoryListingParserTest);

void CDirectoryListingParserTest::setUp()
{
	const int year = wxDateTime::Now().GetYear();
	const int month = wxDateTime::Now().GetYear();
	m_entries.push_back((t_entry){
			"-rw-r--r--   1 root     other        531 3 29 03:26 01-unix-std file",
			{
				_T("01-unix-std file"),
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

		CPPUNIT_ASSERT(pListing->m_entryCount == 1);

		CPPUNIT_ASSERT(pListing->m_pEntries[0] == iter->reference);
	}
}
