#include "FileZilla.h"
#include "directorylistingparser.h"

class CToken
{
protected:
	enum TokenInformation
	{
		Unknown,
		Yes,
		No
	};

public:
	CToken()
	{
		m_pToken = 0;
		m_len = 0;
	}

	CToken(const char *p, int len)
	{
		m_pToken = p;
		m_len = len;
		m_numeric = Unknown;
		m_leftNumeric = Unknown;
		m_rightNumeric = Unknown;
		m_number = -1;
	}

	const char *GetToken() const
	{
		return m_pToken;
	}

	int GetLength() const
	{
		return m_len;
	}

	wxString GetString(unsigned int type = 0)
	{
		if (!m_pToken)
			return _T("");

		if (type > 2)
			return _T("");

		if (m_str[type] != _T(""))
			return m_str[type];

		if (!type)
		{
#ifdef __UNICODE__
			char *buffer = new char[m_len + 1];
			buffer[m_len] = 0;
			memcpy(buffer, m_pToken, m_len);
			wxString str = wxConvCurrent->wxMB2WX(buffer);
			delete [] buffer;
#else
			wxString str(m_pToken, m_len);
#endif
			m_str[type] = str;
			return str;
		}
		else if (type == 1)
		{
			if (!IsRightNumeric() || IsNumeric())
				return _T("");

			int pos = m_len - 1;
			while (m_pToken[pos] >= '0' && m_pToken[pos] <= '9')
				pos--;

#ifdef __UNICODE__
			char *buffer = new char[pos + 2];
			buffer[pos + 1] = 0;
			memcpy(buffer, m_pToken, pos + 1);
			wxString str = wxConvCurrent->wxMB2WX(buffer);
			delete [] buffer;
#else
			wxString str(m_pToken, pos + 1);
#endif
			m_str[type] = str;
			return str;
		}
		else if (type == 2)
		{
			if (!IsLeftNumeric() || IsNumeric())
				return _T("");

			int len = 0;
			while (m_pToken[len] >= '0' && m_pToken[len] <= '9')
				len++;

#ifdef __UNICODE__
			char *buffer = new char[len + 1];
			buffer[len] = 0;
			memcpy(buffer, m_pToken, len);
			wxString str = wxConvCurrent->wxMB2WX(buffer);
			delete [] buffer;
#else
			wxString str(m_pToken, len);
#endif
			m_str[type] = str;
			return str;
		}
	}

	bool IsNumeric()
	{
		if (m_numeric == Unknown)
		{
			m_numeric = Yes;
			for (int i = 0; i < m_len; i++)
				if (m_pToken[i] < '0' || m_pToken[i] > '9')
				{
					m_numeric = No;
					break;
				}
		}
		return m_numeric == Yes;
	}

	bool IsLeftNumeric()
	{
		if (m_leftNumeric == Unknown)
		{
			if (m_len < 2)
				m_leftNumeric = No;
			else if (m_pToken[0] < '0' || m_pToken[0] > '9')
				m_leftNumeric = No;
			else
				m_leftNumeric = Yes;
		}
		return m_leftNumeric == Yes;
	}

	bool IsRightNumeric()
	{
		if (m_rightNumeric == Unknown)
		{
			if (m_len < 2)
				m_rightNumeric = No;
			else if (m_pToken[m_len - 1] < '0' || m_pToken[m_len - 1] > '9')
				m_rightNumeric = No;
			else
				m_rightNumeric = Yes;
		}
		return m_rightNumeric == Yes;
	}

	int Find(char chr) const
	{
		if (!m_pToken)
			return -1;

		for (int i = 0; i < m_len; i++)
			if (m_pToken[i] == chr)
				return i;
		
		return -1;
	}

	wxLongLong GetNumber()
	{
		if (m_number == -1)
		{
			if (IsNumeric() || IsLeftNumeric())
			{
				m_number = 0;
				for (int i = 0; i < m_len; i++)
				{
					if (m_pToken[i] < '0' || m_pToken[i] > '9')
						break;
					m_number *= 10;
					m_number += m_pToken[i] - '0';
				}
			}
			else if (IsRightNumeric())
			{
				m_number = 0;
				int start = m_len - 1;
				while (m_pToken[start - 1] >= '0' && m_pToken[start - 1] <= '9')
					start--;
				for (int i = start; i < m_len; i++)
				{
					m_number *= 10;
					m_number += m_pToken[i] - '0';
				}
			}
		}
		return m_number;;
	}

	char operator[](unsigned int n) const
	{
		if (n >= m_len)
			return 0;

		return m_pToken[n];
	}

protected:
	const char *m_pToken;
	int m_len;

	TokenInformation m_numeric;
	TokenInformation m_leftNumeric;
	TokenInformation m_rightNumeric;
	wxLongLong m_number;
	wxString m_str[3];
};

class CLine
{
public:
	CLine(const char *p, int len)
	{
		m_pLine = p;
		m_len = len;

		m_parsePos = 0;
	}
	
	bool GetToken(unsigned int n, CToken &token, int type = 0)
	{
		if (!type)
		{
			if (m_Tokens.size() > n)
			{
				token = *(m_Tokens[n]);
				return true;
			}

			int start = m_parsePos;
			while (m_parsePos < m_len)
			{
				if (m_pLine[m_parsePos] == ' ')
				{
					CToken *pToken = new CToken(m_pLine + start, m_parsePos - start);
					m_Tokens.push_back(pToken);
					
					while (m_pLine[m_parsePos] == ' ' && m_parsePos < m_len)
						m_parsePos++;

					if (m_Tokens.size() > n)
					{
						token = *(m_Tokens[n]); 
						return true;
					}

					start = m_parsePos;
				}
				m_parsePos++;
			}
			if (m_parsePos != start)
			{
				CToken *pToken = new CToken(m_pLine + start, m_parsePos - start);
					m_Tokens.push_back(pToken);
			}

			if (m_Tokens.size() > n)
			{
				token = *(m_Tokens[n]); 
				return true;
			}

			return false;
		}
		else
		{
			if (m_LineEndTokens.size() > n)
			{
				token = *(m_Tokens[n]);
				return true;
			}
			
			if (m_Tokens.size() <= n)
				if (!GetToken(n, token))
					return false;

			for (unsigned int i = m_LineEndTokens.size(); i <= n; i++)
			{
				const CToken *refToken = m_Tokens[i];
				const char *p = refToken->GetToken();
				CToken *pToken = new CToken(p, m_len - (p - m_pLine));
				m_LineEndTokens.push_back(pToken);
			}
			token = *(m_LineEndTokens[n]);
			return true;
		}
	};

protected:
	std::vector<CToken *> m_Tokens;
	std::vector<CToken *> m_LineEndTokens;
	int m_parsePos;
	const char *m_pLine;
	int m_len;
};

static char data[][100]={
	"-rw-r--r--   1 root     other        531 Jan 29 03:26 unix time",
	"dr-xr-xr-x   2 root     other        512 Apr  8  1994 d unix year",
	"-r-xr-xr-x   2 root                  512 Apr  8  1994 unix nogroup",
	"lrwxrwxrwx   1 root     other          7 Jan 25 00:17 unix link -> usr/bin",
	
	/* Some listings with uncommon date/time format: */
	"-rw-r--r--   1 root     other        531 09-26 2000 README2",
	"-rw-r--r--   1 root     other        531 09-26 13:45 README3",
	"-rw-r--r--   1 root     other        531 2005-06-07 21:22 README4",
	
	/* Also produced by Microsoft's FTP servers for Windows: */
	"----------   1 owner    group         1803128 Jul 10 10:18 ls-lR.Z",
	"d---------   1 owner    group               0 May  9 19:45 Softlib",
	
	/* Also WFTPD for MSDOS: */
	"-rwxrwxrwx   1 noone    nogroup      322 Aug 19  1996 message.ftp",
	
	/* Also NetWare: */
	"d [R----F--] supervisor            512       Jan 16 18:53    login",
	"- [R----F--] rhesus             214059       Oct 20 15:27    cx.exe",
	
	/* Also NetPresenz for the Mac: */
	"-------r--         326  1391972  1392298 Nov 22  1995 MegaPhone.sit",
	"drwxrwxr-x               folder        2 May 10  1996 network",

	/* Some other formats some windows servers send */
	"-rw-r--r--   1 root 531 Jan 29 03:26 README5",
	"-rw-r--r--   1 group domain user 531 Jan 29 03:26 README6",

	/* EPLF directory listings */
	"+i8388621.48594,m825718503,r,s280,\teplf test 1.file",
	"+i8388621.50690,m824255907,/,\teplf test 2.dir",
	"+i8388621.48598,m824253270,r,s612,\teplf test 3.file",

	/* MSDOS type listing used by IIS */
	"04-27-00  12:09PM       <DIR>          DOS dir 1",
	"04-14-00  03:47PM                  589 DOS file 1",

	/* Another type of MSDOS style listings */
	"2002-09-02  18:48       <DIR>          DOS dir 2",
	"2002-09-02  19:06                9,730 DOS file 2",

	/* Numerical Unix style format */
	"0100644   500  101   12345    123456789       filename",

	/* This one is used by SSH-2.0-VShell_2_1_2_143, this is the old VShell format */
	"206876  Apr 04, 2000 21:06 VShell (old)",
	"0  Dec 12, 2002 02:13 VShell (old) Dir/",

	/* This type of directory listings is sent by some newer versions of VShell
	 * both year and time in one line is uncommon.
	 */
	"-rwxr-xr-x    1 user group        9 Oct 08, 2002 09:47 VShell (new)",

	/* Next ones come from an OS/2 server. The server obviously isn't Y2K aware */
	"36611      A    04-23-103   10:57  OS2 test1.file",
	" 1123      A    07-14-99   12:37  OS2 test2.file",
	"    0 DIR       02-11-103   16:15  OS2 test1.dir",
	" 1123 DIR  A    10-05-100   23:38  OS2 test2.dir",

	/* Some servers send localized date formats, here the German one: */
	"dr-xr-xr-x   2 root     other      2235 26. Juli, 20:10 datetest1 (ger)",
	"-r-xr-xr-x   2 root     other      2235 2.   Okt.  2003 datetest2 (ger)",
	"-r-xr-xr-x   2 root     other      2235 1999/10/12 17:12 datetest3",
	"-r-xr-xr-x   2 root     other      2235 24-04-2003 17:12 datetest4",

	/* Here a Japanese one: */
	"-rw-r--r--   1 root       sys           8473  4\x8c\x8e 18\x93\xfa 2003\x94\x4e datatest1 (jap)",

	/* two different VMS style listings */
	"vms_dir_1.DIR;1  1 19-NOV-2001 21:41 [root,root] (RWE,RWE,RE,RE)",
	"vms_file_3;1       155   2-JUL-2003 10:30:13.64",

	/* VMS multiline */
	"VMS_file_1;1\r\n170774/170775     24-APR-2003 08:16:15  [FTP_CLIENT,SCOT]      (RWED,RWED,RE,)",
	"VMS_file_2;1\r\n10			     2-JUL-2003 10:30:08.59  [FTP_CLIENT,SCOT]      (RWED,RWED,RE,)",

	/* IBM AS/400 style listing */
	"QSYS            77824 02/23/00 15:09:55 *DIR IBM Dir1/",

	/* aligned directory listing with too long size */
	"-r-xr-xr-x longowner longgroup123456 Feb 12 17:20 long size test1",

	/* short directory listing with month name */
	"-r-xr-xr-x 2 owner group 4512 01-jun-99 shortdate with monthname",

	/* the following format is sent by the Connect:Enterprise server by Sterling Commerce */
	"-C--E-----FTP B BCC3I1       7670  1294495 Jan 13 07:42 ConEnt file",
	"-C--E-----FTS B BCC3I1       7670  1294495 Jan 13 07:42 ConEnt file2",

	"-AR--M----TCP B ceunix      17570  2313708 Mar 29 08:56 ALL_SHORT1.zip",
	""};

CDirectoryListingParser::CDirectoryListingParser()
{
	//Fill the month names map

	//English month names
	m_MonthNamesMap[_T("jan")] = 1;
	m_MonthNamesMap[_T("feb")] = 2;
	m_MonthNamesMap[_T("mar")] = 3;
	m_MonthNamesMap[_T("apr")] = 4;
	m_MonthNamesMap[_T("may")] = 5;
	m_MonthNamesMap[_T("jun")] = 6;
	m_MonthNamesMap[_T("june")] = 6;
	m_MonthNamesMap[_T("jul")] = 7;
	m_MonthNamesMap[_T("july")] = 7;
	m_MonthNamesMap[_T("aug")] = 8;
	m_MonthNamesMap[_T("sep")] = 9;
	m_MonthNamesMap[_T("sept")] = 9;
	m_MonthNamesMap[_T("oct")] = 10;
	m_MonthNamesMap[_T("nov")] = 11;
	m_MonthNamesMap[_T("dec")] = 12;

	//Numerical values for the month
	m_MonthNamesMap[_T("1")] = 1;
	m_MonthNamesMap[_T("01")] = 1;
	m_MonthNamesMap[_T("2")] = 2;
	m_MonthNamesMap[_T("02")] = 2;
	m_MonthNamesMap[_T("3")] = 3;
	m_MonthNamesMap[_T("03")] = 3;
	m_MonthNamesMap[_T("4")] = 4;
	m_MonthNamesMap[_T("04")] = 4;
	m_MonthNamesMap[_T("5")] = 5;
	m_MonthNamesMap[_T("05")] = 5;
	m_MonthNamesMap[_T("6")] = 6;
	m_MonthNamesMap[_T("06")] = 6;
	m_MonthNamesMap[_T("7")] = 7;
	m_MonthNamesMap[_T("07")] = 7;
	m_MonthNamesMap[_T("8")] = 8;
	m_MonthNamesMap[_T("08")] = 8;
	m_MonthNamesMap[_T("9")] = 9;
	m_MonthNamesMap[_T("09")] = 9;
	m_MonthNamesMap[_T("10")] = 10;
	m_MonthNamesMap[_T("11")] = 11;
	m_MonthNamesMap[_T("12")] = 12;
	
	//German month names
	m_MonthNamesMap[_T("mrz")] = 3;
	m_MonthNamesMap[_T("mär")] = 3;
	m_MonthNamesMap[_T("märz")] = 3;
	m_MonthNamesMap[_T("mai")] = 5;
	m_MonthNamesMap[_T("juni")] = 5;
	m_MonthNamesMap[_T("juli")] = 6;
	m_MonthNamesMap[_T("okt")] = 10;
	m_MonthNamesMap[_T("dez")] = 12;
	
	//French month names
	m_MonthNamesMap[_T("janv")] = 1;
	m_MonthNamesMap[_T("féb")] = 1;
	m_MonthNamesMap[_T("fév")] = 2;
	m_MonthNamesMap[_T("fev")] = 2;
	m_MonthNamesMap[_T("févr")] = 2;
	m_MonthNamesMap[_T("fevr")] = 2;
	m_MonthNamesMap[_T("mars")] = 3;
	m_MonthNamesMap[_T("avr")] = 4;
	m_MonthNamesMap[_T("juin")] = 7;
	m_MonthNamesMap[_T("juil")] = 7;
	m_MonthNamesMap[_T("aoû")] = 8;
	m_MonthNamesMap[_T("août")] = 8;
	m_MonthNamesMap[_T("aout")] = 8;
	m_MonthNamesMap[_T("déc")] = 12;
	m_MonthNamesMap[_T("dec")] = 12;
	
	//Italian month names
	m_MonthNamesMap[_T("gen")] = 1;
	m_MonthNamesMap[_T("mag")] = 5;
	m_MonthNamesMap[_T("giu")] = 6;
	m_MonthNamesMap[_T("lug")] = 7;
	m_MonthNamesMap[_T("ago")] = 8;
	m_MonthNamesMap[_T("set")] = 9;
	m_MonthNamesMap[_T("ott")] = 9;
	m_MonthNamesMap[_T("dic")] = 12;

	//Spanish month names
	m_MonthNamesMap[_T("ene")] = 1;
	m_MonthNamesMap[_T("fbro")] = 2;
	m_MonthNamesMap[_T("mzo")] = 3;
	m_MonthNamesMap[_T("ab")] = 4;
	m_MonthNamesMap[_T("abr")] = 4;
	m_MonthNamesMap[_T("agto")] = 8;
	m_MonthNamesMap[_T("sbre")] = 9;
	m_MonthNamesMap[_T("obre")] = 9;
	m_MonthNamesMap[_T("nbre")] = 9;
	m_MonthNamesMap[_T("dbre")] = 9;

	//Polish month names
	m_MonthNamesMap[_T("sty")] = 1;
	m_MonthNamesMap[_T("lut")] = 2;
	m_MonthNamesMap[_T("kwi")] = 4;
	m_MonthNamesMap[_T("maj")] = 5;
	m_MonthNamesMap[_T("cze")] = 6;
	m_MonthNamesMap[_T("lip")] = 7;
	m_MonthNamesMap[_T("sie")] = 8;
	m_MonthNamesMap[_T("wrz")] = 9;
	m_MonthNamesMap[_T("paŸ")] = 10;
	m_MonthNamesMap[_T("lis")] = 11;
	m_MonthNamesMap[_T("gru")] = 12;

	//Russian month names
	m_MonthNamesMap[_T("ÿíâ")] = 1;
	m_MonthNamesMap[_T("ôåâ")] = 2;
	m_MonthNamesMap[_T("ìàð")] = 3;
	m_MonthNamesMap[_T("àïð")] = 4;
	m_MonthNamesMap[_T("ìàé")] = 5;
	m_MonthNamesMap[_T("èþí")] = 6;
	m_MonthNamesMap[_T("èþë")] = 7;
	m_MonthNamesMap[_T("àâã")] = 8;
	m_MonthNamesMap[_T("ñåí")] = 9;
	m_MonthNamesMap[_T("îêò")] = 10;
	m_MonthNamesMap[_T("íîÿ")] = 11;
	m_MonthNamesMap[_T("äåê")] = 12;

	//Dutch month names
	m_MonthNamesMap[_T("mrt")] = 3;
	m_MonthNamesMap[_T("mei")] = 5;

	//Portuguese month names
	m_MonthNamesMap[_T("out")] = 10;

	//Japanese month names
	m_MonthNamesMap[_T("1\x8c\x8e")] = 1;
	m_MonthNamesMap[_T("2\x8c\x8e")] = 2;
	m_MonthNamesMap[_T("3\x8c\x8e")] = 3;
	m_MonthNamesMap[_T("4\x8c\x8e")] = 4;
	m_MonthNamesMap[_T("5\x8c\x8e")] = 5;
	m_MonthNamesMap[_T("6\x8c\x8e")] = 6;
	m_MonthNamesMap[_T("7\x8c\x8e")] = 7;
	m_MonthNamesMap[_T("8\x8c\x8e")] = 8;
	m_MonthNamesMap[_T("9\x8c\x8e")] = 9;
	m_MonthNamesMap[_T("10\x8c\x8e")] = 10;
	m_MonthNamesMap[_T("11\x8c\x8e")] = 11;
	m_MonthNamesMap[_T("12\x8c\x8e")] = 12;

	//There are more languages and thus month 
	//names, but as long as knowbody reports a 
	//problem, I won't add them, there are way 
	//too many languages

	// Some servers send a combination of month name and number,
	// Add corresponding numbers to the month names.
	std::map<wxString, int> combo;
	for (std::map<wxString, int>::iterator iter = m_MonthNamesMap.begin(); iter != m_MonthNamesMap.end(); iter++)
	{
		// January could be 1 or 0, depends how the server counts
		combo[wxString::Format(_T("%s%02d"), iter->first, iter->second)] = iter->second;
		combo[wxString::Format(_T("%s%02d"), iter->first, iter->second - 1)] = iter->second;
		if (iter->second < 10)
			combo[wxString::Format(_T("%s%d"), iter->first, iter->second)] = iter->second;
		else
			combo[wxString::Format(_T("%s%d"), iter->first, iter->second % 10)] = iter->second;
		if (iter->second <= 10)
			combo[wxString::Format(_T("%s%d"), iter->first, iter->second - 1)] = iter->second;
		else
			combo[wxString::Format(_T("%s%d"), iter->first, (iter->second - 1) % 10)] = iter->second;
	}
	m_MonthNamesMap.insert(combo.begin(), combo.end());

	m_MonthNamesMap[_T("1")] = 1;
	m_MonthNamesMap[_T("2")] = 2;
	m_MonthNamesMap[_T("3")] = 3;
	m_MonthNamesMap[_T("4")] = 4;
	m_MonthNamesMap[_T("5")] = 5;
	m_MonthNamesMap[_T("6")] = 6;
	m_MonthNamesMap[_T("7")] = 7;
	m_MonthNamesMap[_T("8")] = 8;
	m_MonthNamesMap[_T("9")] = 9;
	m_MonthNamesMap[_T("10")] = 10;
	m_MonthNamesMap[_T("11")] = 11;
	m_MonthNamesMap[_T("12")] = 12;
}

CDirectoryListingParser::~CDirectoryListingParser()
{
}

bool CDirectoryListingParser::Parse()
{
	int i = 40;
	bool res;
	while (*data[++i])
	{
		CLine line(data[i], strlen(data[i]));
		res = ParseLine(&line);
	}
	return false;
}

bool CDirectoryListingParser::ParseLine(CLine *pLine)
{
	CDirentry entry;
	bool res = ParseAsUnix(pLine, entry);

	return res;
}

bool CDirectoryListingParser::ParseAsUnix(CLine *pLine, CDirentry &entry)
{
	int index = 0;
	CToken token;
	if (!pLine->GetToken(index, token))
		return false;

	char chr = token[0];
	if (chr != 'b' &&
		chr != 'c' &&
		chr != 'd' &&
		chr != 'l' &&
		chr != 'p' &&
		chr != 's' &&
		chr != '-')
		return false;

	entry.permissions = token.GetString();

	if (chr == 'd' || chr == 'l')
		entry.dir = true;
	else
		entry.dir = false;

	if (chr == 'l')
		entry.link = true;
	else
		entry.link = false;

	// Check for netware servers, which split the permissions into two parts
	bool netware = false;
	if (token.GetLength() == 1)
	{
		if (!pLine->GetToken(++index, token))
			return false;
		entry.permissions += _T(" ") + token.GetString();
		netware = true;
	}

	int numOwnerGroup = 3;
	if (!netware)
	{
		// Filter out groupid, we don't need it
		if (!pLine->GetToken(++index, token))
			return false;

		if (!token.IsNumeric())
			index--;
	}

	// Repeat until numOwnerGroup is 0 since not all servers send every possible field
	int startindex = index;
	do
	{
		// Reset index
		index = startindex;

		entry.ownerGroup.clear();
		for (int i = 0; i < numOwnerGroup; i++)
		{
			if (!pLine->GetToken(++index, token))
				return false;
			if (i)
				entry.ownerGroup += _T(" ");
			entry.ownerGroup += token.GetString();
		}

		if (!pLine->GetToken(++index, token))
			return false;

		// Check for concatenated groupname and size fields
		if (!token.IsNumeric() && !token.IsRightNumeric())
			continue;

		entry.size = token.GetNumber();

		// Append missing group to ownerGroup
		if (!token.IsNumeric() && token.IsRightNumeric())
		{
			if (i)
				entry.ownerGroup += _T(" ");
			entry.ownerGroup += token.GetString(1);
		}

		// Get the month date field
		wxString dateMonth;
		if (!pLine->GetToken(++index, token))
			continue;
		dateMonth = token.GetString();

		// Get day field
		if (!pLine->GetToken(++index, token))
			continue;

		int dateDay;

		// Check for non-numeric day
		if (!token.IsNumeric() && !token.IsLeftNumeric())
		{
			if (!dateMonth.IsNumber())
				continue;
			unsigned long tmp;
			dateMonth.ToULong(&tmp);
			dateDay = tmp;
			dateMonth = token.GetString();
		}
		else
			dateDay = token.GetNumber().GetLo();

		if (dateDay < 1 || dateDay > 31)
			continue;
		entry.date.day = dateDay;

		// Check month name
		dateMonth.MakeLower();
		std::map<wxString, int>::iterator iter = m_MonthNamesMap.find(dateMonth);
		if (iter == m_MonthNamesMap.end())
			continue;
		entry.date.month = iter->second;

		// Get time/year field
		if (!pLine->GetToken(++index, token))
			continue;

		int pos = token.Find(':');
		if (pos == -1)
			pos = token.Find('.');
		if (pos == -1)
			pos = token.Find('-');

		if (pos != -1)
		{
			// token is a time
			if (!pos || pos == (token.GetLength() - 1))
				continue;

			wxString str = token.GetString();
			long hour, minute;
			if (!str.Left(pos).ToLong(&hour))
				continue;
			if (!str.Mid(pos + 1).ToLong(&minute))
				continue;

			if (hour < 0 || hour > 23)
				continue;
			if (minute < 0 || minute > 59)
				continue;

			entry.hasTime = true;
			entry.time.hour = hour;
			entry.time.minute = minute;
		}
		else
		{
			// token is a year
			if (!token.IsNumeric())
				continue;

			wxLongLong year = token.GetNumber();
			if (year > 3000)
				continue;
			if (year < 1000)
				year += 1900;

			entry.date.year = year.GetLo();
			entry.hasTime = false;
		}
		// Get the filename
		if (!pLine->GetToken(++index, token, 1))
			continue;

		entry.name = token.GetString();

		// Filter out cpecial chars at the end of the filenames
		chr = token[token.GetLength() - 1];
		if (chr == '/' ||
			chr == '|' ||
			chr == '*')
			entry.name.RemoveLast();

		entry.hasDate = true;
		
		return true;
	}
	while (--numOwnerGroup);

	return false;
}