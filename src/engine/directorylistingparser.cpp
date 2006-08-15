#include "FileZilla.h"
#include "directorylistingparser.h"
#include "ControlSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

std::map<wxString, int> CDirectoryListingParser::m_MonthNamesMap;
bool CDirectoryListingParser::m_MonthNamesMapInitialized = false;

//#define LISTDEBUG
#ifdef LISTDEBUG
static char data[][110]={
	"-rw-r--r--   1 root     other        531 3 29 03:26 01-unix-std file",
	"dr-xr-xr-x   2 root     other        512 Apr  8  1994 02-unix-std dir",
	"dr-xr-xr-x   2 root                  512 Apr  8  1994 03-unix-nogroup dir",
	"lrwxrwxrwx   1 root     other          7 Jan 25 00:17 04-unix-std link -> usr/bin",
		
	/* Some listings with uncommon date/time format: */
	"-rw-r--r--   1 root     other        531 09-26 2000 05-unix-date file",
	"-rw-r--r--   1 root     other        531 09-26 13:45 06-unix-date file",
	"-rw-r--r--   1 root     other        531 2005-06-07 21:22 07-unix-date file",
		
	/* Unix style with size information in kilobytes */
	"-rw-r--r--   1 root     other  33.5k Oct 5 21:22 08-unix-namedsize file",

	/* Also NetWare: */
	"d [R----F--] supervisor            512       Jan 16 18:53    09-netware dir",
	"- [R----F--] rhesus             214059       Oct 20 15:27    10-netware file",
		
	/* Also NetPresenz for the Mac: */
	"-------r--         326  1391972  1392298 Nov 22  1995 11-netpresenz file",
	"drwxrwxr-x               folder        2 May 10  1996 12-netpresenz dir",

	/* A format with domain field some windows servers send */
	"-rw-r--r--   1 group domain user 531 Jan 29 03:26 13-unix-domain file",

	/* EPLF directory listings */
	"+i8388621.48594,m825718503,r,s280,\t14-eplf file",
	"+i8388621.50690,m824255907,/,\t15-eplf dir",

	/* MSDOS type listing used by IIS */
	"04-27-00  12:09PM       <DIR>          16-dos-dateambigious dir",
	"04-14-00  03:47PM                  589 17-dos-dateambigious file",

	/* Another type of MSDOS style listings */
	"2002-09-02  18:48       <DIR>          18-dos-longyear dir",
	"2002-09-02  19:06                9,730 19-dos-longyear file",

	/* Numerical Unix style format */
	"0100644   500  101   12345    123456789       20-unix-numerical file",

	/* This one is used by SSH-2.0-VShell_2_1_2_143, this is the old VShell format */
	"206876  Apr 04, 2000 21:06 21-vshell-old file",
	"0  Dec 12, 2002 02:13 22-vshell-old dir/",

	/* This type of directory listings is sent by some newer versions of VShell
	 * both year and time in one line is uncommon.
	 */
	"-rwxr-xr-x    1 user group        9 Oct 08, 2002 09:47 23-vshell-new file",

	/* Next ones come from an OS/2 server. The server obviously isn't Y2K aware */
	"36611      A    04-23-103  10:57  24-os2 file",
	" 1123      A    07-14-99   12:37  25-os2 file",
	"    0 DIR       02-11-103  16:15  26-os2 dir",
	" 1123 DIR  A    10-05-100  23:38  27-os2 dir",

	/* Some servers send localized date formats, here the German one: */
	"dr-xr-xr-x   2 root     other      2235 26. Juli, 20:10 28-datetest-ger dir",
	"-r-xr-xr-x   2 root     other      2235 2.   Okt.  2003 29-datetest-ger file",
	"-r-xr-xr-x   2 root     other      2235 1999/10/12 17:12 30-datetest file",
	"-r-xr-xr-x   2 root     other      2235 24-04-2003 17:12 31-datetest file",

	/* Here a Japanese one: */
	"-rw-r--r--   1 root       sys           8473  4\x8c\x8e 18\x93\xfa 2003\x94\x4e 32-datatest-japanese file",

	/* VMS style listings */
	"33-vms-dir.DIR;1  1 19-NOV-2001 21:41 [root,root] (RWE,RWE,RE,RE)",
	"34-vms-file;1       155   2-JUL-2003 10:30:13.64",

	/* VMS style listings without time */
	"35-vms-notime-file;1    2/8    15-JAN-2000    [IV2_XXX]   (RWED,RWED,RE,)",
	"36-vms-notime-file;1    6/8    15-JUI-2002    PRONAS   (RWED,RWED,RE,)",

	/* VMS multiline */
	"37-vms-multiline-file;1\r\n170774/170775     24-APR-2003 08:16:15  [FTP_CLIENT,SCOT]      (RWED,RWED,RE,)",
	"38-vms-multiline-file;1\r\n10			     2-JUL-2003 10:30:08.59  [FTP_CLIENT,SCOT]      (RWED,RWED,RE,)",

	/* IBM AS/400 style listing */
	"QSYS            77824 02/23/00 15:09:55 *DIR 39-ibm-as400 dir/",
	"QSYS            77824 23/02/00 15:09:55 *FILE 40-ibm-as400-date file",

	/* aligned directory listing with too long size */
	"-r-xr-xr-x longowner longgroup123456 Feb 12 17:20 41-unix-concatsize file",

	/* short directory listing with month name */
	"-r-xr-xr-x 2 owner group 4512 01-jun-99 42_unix_shortdatemonth file",

	/* the following format is sent by the Connect:Enterprise server by Sterling Commerce */
	"-C--E-----FTP B BCC3I1       7670  1294495 Jan 13 07:42 43-conent file",
	"-C--E-----FTS B BCC3I1       7670  1294495 Jan 13 07:42 44-conent-file",

	"-AR--M----TCP B ceunix      17570  2313708 Mar 29 08:56 45-conent-file",

	/* Nortel wfFtp router */
	"46-nortel-wfftp-file       1014196  06/03/04  Thur.   10:20:03",

	/* VxWorks based server used in Nortel routers */
	"2048    Feb-28-1998  05:23:30   47-nortel-vxworks dir <DIR>",

	/* IBM MVS listings */
	// Volume Unit    Referred Ext Used Recfm Lrecl BlkSz Dsorg Dsname
	"  WYOSPT 3420   2003/05/21  1  200  FB      80  8053  PS  48-MVS.FILE",
	"  WPTA01 3290   2004/03/04  1    3  FB      80  3125  PO  49-MVS.DATASET",
	"  TSO004 3390   VSAM 50-mvs-file",
	"  TSO005 3390   2005/06/06 213000 U 0 27998 PO 51-mvs-dir",
	"  NRP004 3390   **NONE**    1   15  NONE     0     0  PO  52-MVS-NONEDATE.DATASET",

	/* Dataset members */
	// Name         VV.MM   Created      Changed       Size  Init  Mod Id
	// ADATAB /* filenames without data, only check for those on MVS servers */
	"  53-MVSPDSMEMBER 01.01 2004/06/22 2004/06/22 16:32   128   128    0 BOBY12",

	"54-MVSPDSMEMBER2 00B308 000411  00 FO                31    ANY",
	"55-MVSPDSMEMBER3 00B308 000411  00 FO        RU      ANY    24",

	// Some asian listing format. Those >127 chars are just examples
	"-rwxrwxrwx   1 root     staff          0 2003   3\xed\xef 20 56-asian date file",
	"-r--r--r-- 1 root root 2096 8\xed 17 08:52 57-asian date file",

	"-r-xr-xr-x   2 root  root  96 2004.07.15   58-dotted-date file",

	// VMS style listing with a different field order
	"59-vms-altenrate-field-order-file;1   [SUMMARY]    1/3     2-AUG-2006 13:05  (RWE,RWE,RE,)",

	""};

#endif

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

	enum t_numberBase
	{
		decimal,
		hex
	};

	CToken(const wxChar* p, unsigned int len)
	{
		m_pToken = p;
		m_len = len;
		m_numeric = Unknown;
		m_leftNumeric = Unknown;
		m_rightNumeric = Unknown;
		m_number = -1;
	}

	const wxChar* GetToken() const
	{
		return m_pToken;
	}

	unsigned int GetLength() const
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
			wxString str(m_pToken, m_len);
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

			wxString str(m_pToken, pos + 1);
			m_str[type] = str;
			return str;
		}
		else if (type == 2)
		{
			if (!IsLeftNumeric() || IsNumeric())
				return _T("");

			int pos = 0;
			while (m_pToken[pos] >= '0' && m_pToken[pos] <= '9')
				pos++;

			wxString str(m_pToken + pos, m_len - pos);
			m_str[type] = str;
			return str;
		}

		return _T("");
	}

	bool IsNumeric(t_numberBase base = decimal)
	{
		switch (base)
		{
		case decimal:
		default:
			if (m_numeric == Unknown)
			{
				m_numeric = Yes;
				for (unsigned int i = 0; i < m_len; i++)
					if (m_pToken[i] < '0' || m_pToken[i] > '9')
					{
						m_numeric = No;
						break;
					}
			}
			return m_numeric == Yes;
		case hex:
			for (unsigned int i = 0; i < m_len; i++)
			{
				const char c = m_pToken[i];
				if ((c < '0' || c > '9') && (c < 'A' || c > 'F') && (c < 'a' || c > 'f'))
					return false;
			}
			return true;
		}
	}

	bool IsNumeric(unsigned int start, unsigned int len)
	{
		for (unsigned int i = start; i < wxMin(start + len, m_len); i++)
			if (m_pToken[i] < '0' || m_pToken[i] > '9')
				return false;
		return true;
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

	int Find(const wxChar* chr, int start = 0) const
	{
		if (!chr)
			return -1;
		
		for (unsigned int i = start; i < m_len; i++)
		{
			for (int c = 0; chr[c]; c++)
			{
				if (m_pToken[i] == chr[c])
					return i;
			}
		}
		return -1;
	}
	
	int Find(wxChar chr, int start = 0) const
	{
		if (!m_pToken)
			return -1;

		for (unsigned int i = start; i < m_len; i++)
			if (m_pToken[i] == chr)
				return i;
		
		return -1;
	}

	wxLongLong GetNumber(unsigned int start, int len)
	{
		if (len == -1)
			len = m_len - start;
		if (len < 1)
			return -1;

		if (start + static_cast<unsigned int>(len) > m_len)
			return -1;

		if (m_pToken[start] < '0' || m_pToken[start] > '9')
			return -1;

		wxLongLong number = 0;
		for (unsigned int i = start; i < (start + len); i++)
		{
			if (m_pToken[i] < '0' || m_pToken[i] > '9')
				break;
			number *= 10;
			number += m_pToken[i] - '0';
		}
		return number;
	}

	wxLongLong GetNumber(t_numberBase base = decimal)
	{
		switch (base)
		{
		default:
		case decimal:
			if (m_number == -1)
			{
				if (IsNumeric() || IsLeftNumeric())
				{
					m_number = 0;
					for (unsigned int i = 0; i < m_len; i++)
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
					for (unsigned int i = start; i < m_len; i++)
					{
						m_number *= 10;
						m_number += m_pToken[i] - '0';
					}
				}
			}
			return m_number;
		case hex:
			{
				wxLongLong number = 0;
				for (unsigned int i = 0; i < m_len; i++)
				{
					const wxChar& c = m_pToken[i];
					if (c >= '0' && c <= '9')
					{
						number *= 16;
						number += c - '0';
					}
					else if (c >= 'a' && c <= 'f')
					{
						number *= 16;
						number += c - '0' + 10;
					}
					else if (c >= 'A' && c <= 'F')
					{
						number *= 16;
						number += c - 'A' + 10;
					}
					else
						return -1;
				}
				return number;
			}
		}
	}

	char operator[](unsigned int n) const
	{
		if (n >= m_len)
			return 0;

		return m_pToken[n];
	}

protected:
	const wxChar* m_pToken;
	unsigned int m_len;

	TokenInformation m_numeric;
	TokenInformation m_leftNumeric;
	TokenInformation m_rightNumeric;
	wxLongLong m_number;
	wxString m_str[3];
};

class CLine
{
public:
	CLine(wxChar* p)
	{
		m_pLine = p;
		m_len = wxStrlen(p);

		m_parsePos = 0;
	}

	~CLine()
	{
		delete [] m_pLine;

		std::vector<CToken *>::iterator iter;
		for (iter = m_Tokens.begin(); iter != m_Tokens.end(); iter++)
			delete *iter;
		for (iter = m_LineEndTokens.begin(); iter != m_LineEndTokens.end(); iter++)
			delete *iter;
	}
	
	bool GetToken(unsigned int n, CToken &token, bool toEnd = false)
	{
		if (!toEnd)
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

			for (unsigned int i = static_cast<unsigned int>(m_LineEndTokens.size()); i <= n; i++)
			{
				const CToken *refToken = m_Tokens[i];
				const wxChar* p = refToken->GetToken();
				CToken *pToken = new CToken(p, m_len - (p - m_pLine));
				m_LineEndTokens.push_back(pToken);
			}
			token = *(m_LineEndTokens[n]);
			return true;
		}
	};

	CLine *Concat(const CLine *pLine) const
	{
		int newLen = m_len + pLine->m_len + 1;
		wxChar* p = new wxChar[newLen];
		memcpy(p, m_pLine, m_len);
		p[m_len] = ' ';
		memcpy(p + m_len + 1, pLine->m_pLine, pLine->m_len);
		
		return new CLine(p);
	}

protected:
	std::vector<CToken *> m_Tokens;
	std::vector<CToken *> m_LineEndTokens;
	int m_parsePos;
	int m_len;
	wxChar* m_pLine;
};

CDirectoryListingParser::CDirectoryListingParser(CFileZillaEnginePrivate *pEngine, CControlSocket* pControlSocket, const CServer& server)
	: m_pEngine(pEngine), m_pControlSocket(pControlSocket), m_server(server)
{
	startOffset = 0;
	m_curLine = 0;
	m_prevLine = 0;

	if (!m_MonthNamesMapInitialized)
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
		m_MonthNamesMap[_T("m\xe4r")] = 3;
		m_MonthNamesMap[_T("m\xe4rz")] = 3;
		m_MonthNamesMap[_T("mai")] = 5;
		m_MonthNamesMap[_T("juni")] = 5;
		m_MonthNamesMap[_T("juli")] = 6;
		m_MonthNamesMap[_T("okt")] = 10;
		m_MonthNamesMap[_T("dez")] = 12;

		//Austrian month names
		m_MonthNamesMap[_T("j\xe4n")] = 1;

		//French month names
		m_MonthNamesMap[_T("janv")] = 1;
		m_MonthNamesMap[_T("f\xe9")_T("b")] = 1;
		m_MonthNamesMap[_T("f\xe9v")] = 2;
		m_MonthNamesMap[_T("fev")] = 2;
		m_MonthNamesMap[_T("f\xe9vr")] = 2;
		m_MonthNamesMap[_T("fevr")] = 2;
		m_MonthNamesMap[_T("mars")] = 3;
		m_MonthNamesMap[_T("mrs")] = 3;
		m_MonthNamesMap[_T("avr")] = 4;
		m_MonthNamesMap[_T("juin")] = 6;
		m_MonthNamesMap[_T("juil")] = 7;
		m_MonthNamesMap[_T("jui")] = 7;
		m_MonthNamesMap[_T("ao\xfb")] = 8;
		m_MonthNamesMap[_T("ao\xfbt")] = 8;
		m_MonthNamesMap[_T("aout")] = 8;
		m_MonthNamesMap[_T("d\xe9")_T("c")] = 12;
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
		m_MonthNamesMap[_T("pa\x9f")] = 10;
		m_MonthNamesMap[_T("lis")] = 11;
		m_MonthNamesMap[_T("gru")] = 12;

		//Russian month names
		m_MonthNamesMap[_T("\xff\xed\xe2")] = 1;
		m_MonthNamesMap[_T("\xf4\xe5\xe2")] = 2;
		m_MonthNamesMap[_T("\xec\xe0\xf0")] = 3;
		m_MonthNamesMap[_T("\xe0\xef\xf0")] = 4;
		m_MonthNamesMap[_T("\xec\xe0\xe9")] = 5;
		m_MonthNamesMap[_T("\xe8\xfe\xed")] = 6;
		m_MonthNamesMap[_T("\xe8\xfe\xeb")] = 7;
		m_MonthNamesMap[_T("\xe0\xe2\xe3")] = 8;
		m_MonthNamesMap[_T("\xf1\xe5\xed")] = 9;
		m_MonthNamesMap[_T("\xee\xea\xf2")] = 10;
		m_MonthNamesMap[_T("\xed\xee\xff")] = 11;
		m_MonthNamesMap[_T("\xe4\xe5\xea")] = 12;

		//Dutch month names
		m_MonthNamesMap[_T("mrt")] = 3;
		m_MonthNamesMap[_T("mei")] = 5;

		//Portuguese month names
		m_MonthNamesMap[_T("out")] = 10;

		//Finnish month names
		m_MonthNamesMap[_T("tammi")] = 1;
		m_MonthNamesMap[_T("helmi")] = 2;
		m_MonthNamesMap[_T("maalis")] = 3;
		m_MonthNamesMap[_T("huhti")] = 4;
		m_MonthNamesMap[_T("touko")] = 5;
		m_MonthNamesMap[_T("kes\xe4")] = 6;
		m_MonthNamesMap[_T("hein\xe4")] = 7;
		m_MonthNamesMap[_T("elo")] = 8;
		m_MonthNamesMap[_T("syys")] = 9;
		m_MonthNamesMap[_T("loka")] = 10;
		m_MonthNamesMap[_T("marras")] = 11;
		m_MonthNamesMap[_T("joulu")] = 12;

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
			combo[wxString::Format(_T("%s%02d"), iter->first.c_str(), iter->second)] = iter->second;
			combo[wxString::Format(_T("%s%02d"), iter->first.c_str(), iter->second - 1)] = iter->second;
			if (iter->second < 10)
				combo[wxString::Format(_T("%s%d"), iter->first.c_str(), iter->second)] = iter->second;
			else
				combo[wxString::Format(_T("%s%d"), iter->first.c_str(), iter->second % 10)] = iter->second;
			if (iter->second <= 10)
				combo[wxString::Format(_T("%s%d"), iter->first.c_str(), iter->second - 1)] = iter->second;
			else
				combo[wxString::Format(_T("%s%d"), iter->first.c_str(), (iter->second - 1) % 10)] = iter->second;
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

		m_MonthNamesMapInitialized = true;
	}

#ifdef LISTDEBUG
	for (unsigned int i = 0; data[i][0]; i++)
	{
		unsigned int len = (unsigned int)strlen(data[i]);
		char *pData = new char[len + 3];
		strcpy(pData, data[i]);
		strcat(pData, "\r\n");
		AddData(pData, len + 2);
	}
#endif
}

CDirectoryListingParser::~CDirectoryListingParser()
{
	for (std::list<t_list>::iterator iter = m_DataList.begin(); iter != m_DataList.end(); iter++)
		delete [] iter->p;

	delete m_curLine;
	delete m_prevLine;
}

CDirectoryListing* CDirectoryListingParser::Parse(const CServerPath &path)
{
	CLine *pLine = GetLine();
	m_curLine = pLine;
	while (pLine)
	{
		bool res = ParseLine(pLine, m_server.GetType());
		if (!res)
		{
			if (m_prevLine)
			{
				if (m_curLine != pLine)
				{
					delete m_prevLine;
					m_prevLine = m_curLine;
					delete pLine;
					
					pLine = GetLine();
					m_curLine = pLine;
				}
				else
					pLine = m_prevLine->Concat(m_curLine);
			}
			else
			{
				m_prevLine = pLine;

				pLine = GetLine();
				m_curLine = pLine;
			}
		}
		else
		{
			delete m_prevLine;
			m_prevLine = 0;
			delete m_curLine;

			pLine = GetLine();
			m_curLine = pLine;
		}
	};

	CDirectoryListing *pListing = new CDirectoryListing;
	pListing->path = path;
	pListing->m_entryCount = static_cast<unsigned int>(m_entryList.size());
	pListing->m_pEntries = new CDirentry[m_entryList.size()];
	
	int i = 0;
	for (std::list<CDirentry>::iterator iter = m_entryList.begin(); iter != m_entryList.end(); iter++)
		pListing->m_pEntries[i++] = *iter;

	return pListing;
}

bool CDirectoryListingParser::ParseLine(CLine *pLine, const enum ServerType serverType)
{
	CDirentry entry;
	bool res = ParseAsUnix(pLine, entry);
	if (res)
		goto done;
	res = ParseAsDos(pLine, entry);
	if (res)
		goto done;
	res = ParseAsEplf(pLine, entry);
	if (res)
		goto done;
	res = ParseAsVms(pLine, entry);
	if (res)
		goto done;
	res = ParseOther(pLine, entry);
	if (res)
		goto done;
	res = ParseAsIbm(pLine, entry);
	if (res)
		goto done;
	res = ParseAsWfFtp(pLine, entry);
	if (res)
		goto done;
	res = ParseAsIBM_MVS(pLine, entry);
	if (res)
		goto done;
	res = ParseAsIBM_MVS_PDS(pLine, entry);
	if (res)
		goto done;
#ifndef LISTDEBUG
	if (serverType == MVS)
#endif //LISTDEBUG
	{
		res = ParseAsIBM_MVS_PDS2(pLine, entry);
		if (res)
			goto done;
	}

	return false;
done:

	// Don't add . or ..
	if (entry.name == _T(".") || entry.name == _T(".."))
		return true;

	int offset = m_server.GetTimezoneOffset();
	if (offset && entry.hasTime)
	{
		// Apply timezone offset
		wxDateTime time;
		if (VerifySetDate(time, entry.date.year, (wxDateTime::Month)(wxDateTime::Jan + entry.date.month - 1), entry.date.day, entry.time.hour, entry.time.minute))
		{
			wxTimeSpan span(0, offset, 0, 0);
			time.Add(span);
			entry.date.year = time.GetYear();
			entry.date.month = time.GetMonth() - wxDateTime::Jan + 1;
			entry.date.day = time.GetDay();
			entry.time.hour = time.GetHour();
			entry.time.minute = time.GetMinute();
		}
	}
		
	entry.unsure = false;
	m_entryList.push_back(entry);

	return true;
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
		if (!ParseComplexFileSize(token, entry.size))
		{
			if (!token.IsRightNumeric())
				continue;
			entry.size = token.GetNumber();
		}

		// Append missing group to ownerGroup
		if (!token.IsNumeric() && token.IsRightNumeric())
		{
			if (!entry.ownerGroup.IsEmpty())
				entry.ownerGroup += _T(" ");
			entry.ownerGroup += token.GetString(1);
		}

		if (!ParseUnixDateTime(pLine, index, entry))
			continue;

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

		if (entry.link)
		{
			int pos;
			if ((pos = entry.name.Find(_T(" -> "))) != -1)
			{
			    entry.target = entry.name.Mid(pos + 4);
			    entry.name = entry.name.Left(pos);
			}
		}

		return true;
	}
	while (--numOwnerGroup);

	return false;
}

bool CDirectoryListingParser::ParseUnixDateTime(CLine *pLine, int &index, CDirentry &entry)
{
	bool mayHaveTime = true;
	bool bHasYearAndTime = false;

	CToken token;

	// Get the month date field
	CToken dateMonth;
	if (!pLine->GetToken(++index, token))
		return false;

	entry.date.year = 0;
	entry.date.month = 0;
	entry.date.day = 0;
	entry.time.hour = 0;
	entry.time.minute = 0;

	// Some servers use the following date formats:
	// 26-05 2002, 2002-10-14, 01-jun-99 or 2004.07.15
	// slashes instead of dashes are also possible
	int pos = token.Find(_T("-/."));
	if (pos != -1)
	{
		int pos2 = token.Find(_T("-/."), pos + 1);
		if (pos2 == -1)
		{
			if (token[pos] != '.')
			{
				// something like 26-05 2002
				int day = token.GetNumber(pos + 1, token.GetLength() - pos - 1).GetLo();
				if (day < 1 || day > 31)
					return false;
				entry.date.day = day;
				dateMonth = CToken(token.GetToken(), pos);
			}
			else
				dateMonth = token;
		}
		else if (token[pos] != token[pos2])
			return false;
		else
		{
			if (!ParseShortDate(token, entry))
				return false;

			if (token[pos] == '.')
			{
				entry.hasTime = false;
				return true;
			}
		}
	}
	else if (token.IsNumeric())
	{
		if (token.GetNumber() > 1000 && token.GetNumber() < 10000)
		{
			// Two possible variants:
			// 1) 2005 3 13
			// 2) 2005 13 3
			// assume first one.
			entry.date.year = token.GetNumber().GetLo();
			if (!pLine->GetToken(++index, dateMonth))
				return false;
			mayHaveTime = false;
		}
		else
			dateMonth = token;
	}
	else
		dateMonth = token;

	if (!entry.date.day)
	{
		// Get day field
		if (!pLine->GetToken(++index, token))
			return false;
	
		int dateDay;
	
		// Check for non-numeric day
		if (!token.IsNumeric() && !token.IsLeftNumeric())
		{
			int offset = 0;
			if (dateMonth.GetString().Right(1) == _T("."))
				offset++;
			if (!dateMonth.IsNumeric(0, dateMonth.GetLength() - offset))
				return false;
			dateDay = dateMonth.GetNumber(0, dateMonth.GetLength() - 1).GetLo();
			dateMonth = token;
		}
		else
		{
			dateDay = token.GetNumber().GetLo();
			if (token[token.GetLength() - 1] == ',')
				bHasYearAndTime = true;
		}

		if (dateDay < 1 || dateDay > 31)
			return false;
		entry.date.day = dateDay;
	}

	if (!entry.date.month)
	{
		wxString month = dateMonth.GetString();
		if (dateMonth.IsLeftNumeric() && (unsigned int)month[month.Length() - 1] > 127)
		{
			// Most likely an Asian server sending some unknown language specific 
			// suffix at the end of the monthname. Filter it out.
			int i;
			for (i = month.Length() - 1; i > 0; i--)
			{
				if (month[i] >= '0' && month[i] <= '9')
					break;
			}
			month = month.Left(i + 1);
		}
		// Check month name
		if (month.Right(1) == _T(",") || month.Right(1) == _T("."))
			month.RemoveLast();
		month.MakeLower();
		std::map<wxString, int>::iterator iter = m_MonthNamesMap.find(month);
		if (iter == m_MonthNamesMap.end())
			return false;
		entry.date.month = iter->second;
	}

	// Get time/year field
	if (!pLine->GetToken(++index, token))
		return false;

	pos = token.Find(_T(":.-"));
	if (pos != -1 && mayHaveTime)
	{
		// token is a time
		if (!pos || static_cast<size_t>(pos) == (token.GetLength() - 1))
			return false;

		wxString str = token.GetString();
		long hour, minute;
		if (!str.Left(pos).ToLong(&hour))
			return false;
		if (!str.Mid(pos + 1).ToLong(&minute))
			return false;

		if (hour < 0 || hour > 23)
			return false;
		if (minute < 0 || minute > 59)
			return false;

		entry.hasTime = true;
		entry.time.hour = hour;
		entry.time.minute = minute;

		// Some servers use times only for files nweer than 6 months,
		int year = wxDateTime::Now().GetYear();
		int now = wxDateTime::Now().GetDay() + 31 * wxDateTime::Now().GetMonth();
		int file = (entry.date.month - 1) * 31 + (entry.date.day - 1);
		if (now > file)
			entry.date.year = year;
		else
			entry.date.year = year - 1;
	}
	else if (!entry.date.year)
	{
		// token is a year
		if (!token.IsNumeric() && !token.IsLeftNumeric())
			return false;

		wxLongLong year = token.GetNumber();
		if (year > 3000)
			return false;
		if (year < 1000)
			year += 1900;

		entry.date.year = year.GetLo();

		if (bHasYearAndTime)
		{
			if (!pLine->GetToken(++index, token))
				return false;

			if (token.Find(':') == 2 && token.GetLength() == 5 && token.IsLeftNumeric() && token.IsRightNumeric())
			{
				int pos = token.Find(':');
				// token is a time
				if (!pos || static_cast<size_t>(pos) == (token.GetLength() - 1))
					return false;

				wxString str = token.GetString();
				long hour, minute;
				if (!str.Left(pos).ToLong(&hour))
					return false;
				if (!str.Mid(pos + 1).ToLong(&minute))
					return false;

				if (hour < 0 || hour > 23)
					return false;
				if (minute < 0 || minute > 59)
					return false;

				entry.hasTime = true;
				entry.time.hour = hour;
				entry.time.minute = minute;
			}
			else
			{
				entry.hasTime = false;
				index--;
			}
		}
		else
			entry.hasTime = false;
	}
	else
		index--;

	entry.hasDate = true;

	return true;
}

bool CDirectoryListingParser::ParseShortDate(CToken &token, CDirentry &entry)
{
	if (token.GetLength() < 1)
		return false;

	bool gotYear = false;
	bool gotMonth = false;
	bool gotDay = false;
	bool gotMonthName = false;

	int value = 0;

	int pos = token.Find(_T("-./"));
	if (pos < 1)
		return false;

	if (!token.IsNumeric(0, pos))
	{
		// Seems to be monthname-dd-yy
		
		// Check month name
		wxString dateMonth = token.GetString().Mid(0, pos);
		dateMonth.MakeLower();
		std::map<wxString, int>::iterator iter = m_MonthNamesMap.find(dateMonth);
		if (iter == m_MonthNamesMap.end())
			return false;
		entry.date.month = iter->second;
		gotMonth = true;
		gotMonthName = true;
	}
	else if (pos == 4)
	{
		// Seems to be yyyy-mm-dd
		wxLongLong year = token.GetNumber(0, pos);
		if (year < 1900 || year > 3000)
			return false;
		entry.date.year = year.GetLo();
		gotYear = true;
	}
	else if (pos <= 2)
	{
		wxLongLong value = token.GetNumber(0, pos);
		if (token[pos] == '.')
		{
			// Maybe dd.mm.yyyy
			if (value < 1900 || value > 3000)
				return false;
			entry.date.day = value.GetLo();
			gotDay = true;
		}
		else
		{
			// Detect mm-dd-yyyy or mm/dd/yyyy and
			// dd-mm-yyyy or dd/mm/yyyy
			if (value < 1)
				return false;
			if (value > 12)
			{
				if (value > 31)
					return false;

				entry.date.day = value.GetLo();
				gotDay = true;
			}
			else
			{
				entry.date.month = value.GetLo();
				gotMonth = true;
			}
		}			
	}
	else
		return false;

	int pos2 = token.Find(_T("-./"), pos + 1);
	if (pos2 == -1 || (pos2 - pos) == 1)
		return false;
	if (static_cast<size_t>(pos2) == (token.GetLength() - 1))
		return false;

	// If we already got the month and the second field is not numeric, 
	// change old month into day and use new token as month
	if (!token.IsNumeric(pos + 1, pos2 - pos - 1) && gotMonth)
	{
		if (gotMonthName)
			return false;

		if (gotDay)
			return false;

		gotDay = true;
		gotMonth = false;
		entry.date.day = entry.date.month;
	}

	if (gotYear || gotDay)
	{
		// Month field in yyyy-mm-dd or dd-mm-yyyy
		// Check month name
		wxString dateMonth = token.GetString().Mid(pos + 1, pos2 - pos - 1);
		dateMonth.MakeLower();
		std::map<wxString, int>::iterator iter = m_MonthNamesMap.find(dateMonth);
		if (iter == m_MonthNamesMap.end())
			return false;
		entry.date.month = iter->second;
		gotMonth = true;
	}
	else
	{
		wxLongLong value = token.GetNumber(pos + 1, pos2 - pos - 1);
		// Day field in mm-dd-yyyy
		if (value < 1 || value > 31)
			return false;
		entry.date.day = value.GetLo();
		gotDay = true;
	}

	value = token.GetNumber(pos2 + 1, token.GetLength() - pos2 - 1).GetLo();
	if (gotYear)
	{
		// Day field in yyy-mm-dd
		if (!value || value > 31)
			return false;
		entry.date.day = value;
		gotDay = true;
	}
	else
	{
		if (value < 0)
			return false;

		if (value < 50)
			value += 2000;
		else if (value < 1000)
			value += 1900;
		entry.date.year = value;

		gotYear = true;
	}

	entry.hasDate = true;

	if (!gotMonth || !gotDay || !gotYear)
		return false;
		
	return true;
}

bool CDirectoryListingParser::ParseAsDos(CLine *pLine, CDirentry &entry)
{
	int index = 0;
	CToken token;

	// Get first token, has to be a valid date
	if (!pLine->GetToken(index, token))
		return false;

	if (!ParseShortDate(token, entry))
		return false;

	// Extract time
	if (!pLine->GetToken(++index, token))
		return false;

	if (!ParseTime(token, entry))
		return false;

	// If next token is <DIR>, entry is a directory
	// else, it should be the filesize.
	if (!pLine->GetToken(++index, token))
		return false;

	if (token.GetString() == _T("<DIR>"))
	{
		entry.dir = true;
		entry.size = -1;
	}
	else if (token.IsNumeric() || token.IsLeftNumeric())
	{
		// Convert size, filter out separators
		int size = 0;
		int len = token.GetLength();
		for (int i = 0; i < len; i++)
		{
			char chr = token[i];
			if (chr == ',' || chr == '.')
				continue;
			if (chr < '0' || chr > '9')
				return false;

			size *= 10;
			size += chr - '0';
		}
		entry.size = size;
		entry.dir = false;
	}
	else
		return false;

	// Extract filename
	if (!pLine->GetToken(++index, token, true))
		return false;
	entry.name = token.GetString();
	
	return true;
}

bool CDirectoryListingParser::ParseTime(CToken &token, CDirentry &entry)
{
	int pos = token.Find(':');
	if (pos < 1 || static_cast<unsigned int>(pos) >= (token.GetLength() - 1))
		return false;

	wxLongLong hour = token.GetNumber(0, pos);
	if (hour < 0 || hour > 23)
		return false;

	wxLongLong minute = token.GetNumber(pos + 1, -1);
	if (minute < 0 || minute > 59)
		return false;

	// Convert to 24h format
	if (!token.IsRightNumeric())
	{
		if (token[token.GetLength() - 2] == 'P')
		{
			if (hour < 12)
				hour += 12;
		}
		else
			if (hour == 12)
				hour = 0;
	}

	entry.time.hour = hour.GetLo();
	entry.time.minute = minute.GetLo();
	entry.hasTime = true;

	return true;
}

bool CDirectoryListingParser::ParseAsEplf(CLine *pLine, CDirentry &entry)
{
	CToken token;
	if (!pLine->GetToken(0, token, true))
		return false;

	if (token[0] != '+')
		return false;

	int pos = token.Find('\t');
	if (pos == -1 || static_cast<size_t>(pos) == (token.GetLength() - 1))
		return false;

	entry.name = token.GetString().Mid(pos + 1);

	entry.dir = false;
	entry.link = false;
	entry.hasDate = false;
	entry.hasTime = false;
	entry.size = -1;
	
	int fact = 1;
	int separator = token.Find(',', fact);
	while (separator != -1 && separator < pos)
	{
		int len = separator - fact;
		char type = token[fact];

		if (type == '/')
			entry.dir = true;
		else if (type == 's')
			entry.size = token.GetNumber(fact + 1, len - 1);
		else if (type == 'm')
		{
			wxLongLong number = token.GetNumber(fact + 1, len - 1);
			if (number < 0)
				return false;
			wxDateTime dateTime(number);
			entry.date.year = dateTime.GetYear();
			entry.date.month = dateTime.GetMonth();
			entry.date.day = dateTime.GetDay();
			entry.time.hour = dateTime.GetHour();
			entry.time.minute = dateTime.GetMinute();

			entry.hasTime = true;
			entry.hasDate = true;
		}
		else if (type == 'u' && len > 2 && token[fact + 1] == 'p')
			entry.permissions = token.GetString().Mid(fact + 2, len - 2);

		fact = separator + 1;
		separator = token.Find(',', fact);
	}

	return true;
}

bool CDirectoryListingParser::ParseAsVms(CLine *pLine, CDirentry &entry)
{
	CToken token;
	int index = 0;

	if (!pLine->GetToken(index, token))
		return false;

	int pos = token.Find(';');
	if (pos == -1)
		return false;

	if (pos > 4 && token.GetString().Mid(pos - 4, 4) == _T(".DIR"))
	{
		entry.dir = true;
		entry.name = token.GetString().Left(pos - 4) + token.GetString().Mid(pos);
	}
	else
	{
		entry.dir = false;
		entry.name = token.GetString();
	}

	// This field is either the size or a username (???) enclosed in [].
	if (!pLine->GetToken(++index, token))
		return false;

	bool gotSize = false;
	if (!token.IsNumeric() && !token.IsLeftNumeric())
	{
		const int len = token.GetLength();
		if (len < 3 || token[0] != '[' || token[len - 1] != ']')
			return false;
		entry.ownerGroup = token.GetString().Mid(1, len - 2);
	}
	else
	{
		gotSize = true;
		entry.size = token.GetNumber();
	}
	
	if (!gotSize)
	{
		// Get size
		if (!pLine->GetToken(++index, token))
			return false;

		if (!token.IsNumeric() && !token.IsLeftNumeric())
			return false;
	
		entry.size = token.GetNumber();
	}

	// Get date
	if (!pLine->GetToken(++index, token))
		return false;

	if (!ParseShortDate(token, entry))
		return false;

	// Get time
	if (!pLine->GetToken(++index, token))
		return true;

	if (!ParseTime(token, entry))
	{
		int len = token.GetLength();
		if (token[0] == '[' && token[len - 1] != ']')
			return false;
		if (token[0] == '(' && token[len - 1] != ')')
			return false;
		if (token[0] != '[' && token[len - 1] == ']')
			return false;
		if (token[0] != '(' && token[len - 1] == ')')
			return false;
		entry.hasTime = false;
		index--;
	}

	// Owner / group
	while (pLine->GetToken(++index, token))
	{
		const int len = token.GetLength();
		if (len > 2 && token[0] == '(' && token[len - 1] == ')')
		{
			if (entry.permissions != _T(""))
				entry.permissions += _T(" ");
			entry.permissions = token.GetString().Mid(1, len - 2);
		}
		else if (len > 2 && token[0] == '[' && token[len - 1] == ']')
		{
			if (entry.ownerGroup != _T(""))
				entry.ownerGroup += _T(" ");
			entry.ownerGroup = token.GetString().Mid(1, len - 2);
		}
		else
		{
			if (entry.permissions != _T(""))
				entry.permissions += _T(" ");
			entry.permissions = token.GetString();
		}
	}

	return true;
}

bool CDirectoryListingParser::ParseAsIbm(CLine *pLine, CDirentry &entry)
{
	int index = 0;
	CToken token;

	// Get owner
	if (!pLine->GetToken(index, token))
		return false;

	entry.ownerGroup = token.GetString();

	// Get size
	if (!pLine->GetToken(++index, token))
		return false;

	if (!token.IsNumeric())
		return false;

	entry.size = token.GetNumber();

	// Get date
	if (!pLine->GetToken(++index, token))
		return false;

	if (!ParseShortDate(token, entry))
		return false;

	// Get time
	if (!pLine->GetToken(++index, token))
		return false;

	if (!ParseTime(token, entry))
		return false;

	// Get filename
	if (!pLine->GetToken(index + 2, token, 1))
		return false;
	
	entry.name = token.GetString();
	if (token[token.GetLength() - 1] == '/')
	{
		entry.name.RemoveLast();
		entry.dir = true;
	}
	else
		entry.dir = false;

	return true;
}

bool CDirectoryListingParser::ParseOther(CLine *pLine, CDirentry &entry)
{
	int index = 0;
	CToken firstToken;

	if (!pLine->GetToken(index, firstToken))
		return false;

	if (!firstToken.IsNumeric())
		return false;

	// Possible formats: Numerical unix, VShell or OS/2

	CToken token;
	if (!pLine->GetToken(++index, token))
		return false;

	// If token is a number, than it's the numerical Unix style format,
	// else it's the VShell, OS/2 or nortel.VxWorks format
	if (token.IsNumeric())
	{
		entry.permissions = firstToken.GetString();
		if (firstToken.GetLength() >= 2 && firstToken[1] == '4')
			entry.dir = true;
		else
			entry.dir = false;

		entry.ownerGroup += token.GetString();

		if (!pLine->GetToken(++index, token))
			return false;

		entry.ownerGroup += _T(" ") + token.GetString();

		// Get size
		if (!pLine->GetToken(++index, token))
			return false;

		if (!token.IsNumeric())
			return false;

		entry.size = token.GetNumber();

		// Get date/time
		if (!pLine->GetToken(++index, token))
			return false;

		wxLongLong number = token.GetNumber();
		if (number < 0)
			return false;
		wxDateTime dateTime(number);
		entry.date.year = dateTime.GetYear();
		entry.date.month = dateTime.GetMonth();
		entry.date.day = dateTime.GetDay();
		entry.time.hour = dateTime.GetHour();
		entry.time.minute = dateTime.GetMinute();

		entry.hasTime = true;
		entry.hasDate = true;

		// Get filename
		if (!pLine->GetToken(++index, token, true))
			return false;

		entry.name = token.GetString();
	}
	else
	{
		// VShell, OS/2 or nortel.VxWorks style format
		entry.size = firstToken.GetNumber();

		// Get date
		wxString dateMonth = token.GetString();
		dateMonth.MakeLower();
		std::map<wxString, int>::const_iterator iter = m_MonthNamesMap.find(dateMonth);
		if (iter == m_MonthNamesMap.end())
		{
			// OS/2 or nortel.VxWorks
			entry.dir = false;
			int skippedCount = 0;
			do
			{
				if (token.GetString() == _T("DIR"))
					entry.dir = true;
				else if (token.Find(_T("-/.")) != -1)
					break;

				skippedCount++;

				if (!pLine->GetToken(++index, token))
					return false;
			} while (true);

			if (!ParseShortDate(token, entry))
				return false;

			// Get time
			if (!pLine->GetToken(++index, token))
				return false;

			if (!ParseTime(token, entry))
				return false;

			// Get filename
			if (!pLine->GetToken(++index, token, true))
				return false;

			entry.name = token.GetString();
			if (!skippedCount && entry.name.Right(5).MakeUpper() == _T("<DIR>"))
			{
				entry.dir = true;
				entry.name = entry.name.Left(entry.name.Length() - 5);
				while (entry.name.Last() == ' ')
					entry.name.RemoveLast();
			}
		}
		else
		{
			entry.date.month = iter->second;

			// Get day
			if (!pLine->GetToken(++index, token))
				return false;

			if (!token.IsNumeric() && !token.IsLeftNumeric())
				return false;

			wxLongLong day = token.GetNumber();
			if (day < 0 || day > 31)
				return false;
			entry.date.day = day.GetLo();

			// Get Year
			if (!pLine->GetToken(++index, token))
				return false;

			if (!token.IsNumeric())
				return false;

			wxLongLong year = token.GetNumber();
			if (year < 50)
				year += 2000;
			else if (year < 1000)
				year += 1900;
			entry.date.year = year.GetLo();

			entry.hasDate = true;

			// Get time
			if (!pLine->GetToken(++index, token))
				return false;

			if (!ParseTime(token, entry))
				return false;

			// Get filename
			if (!pLine->GetToken(++index, token, 1))
				return false;

			entry.name = token.GetString();
			char chr = token[token.GetLength() - 1];
			if (chr == '/' || chr == '\\')
			{
				entry.dir = true;
				entry.name.RemoveLast();
			}
			else
				entry.dir = false;
		}
	}

	return true;
}

void CDirectoryListingParser::AddData(char *pData, int len)
{
	t_list item;
	item.p = pData;
	item.len = len;

	m_DataList.push_back(item);

	CLine *pLine = GetLine(true);
	m_curLine = pLine;
	while (pLine)
	{
		bool res = ParseLine(pLine, m_server.GetType());
		if (!res)
		{
			if (m_prevLine)
			{
				if (m_curLine != pLine)
				{
					delete m_prevLine;
					m_prevLine = m_curLine;
					delete pLine;
					
					pLine = GetLine(true);
					m_curLine = pLine;
				}
				else
					pLine = m_prevLine->Concat(m_curLine);
			}
			else
			{
				m_prevLine = pLine;

				pLine = GetLine(true);
				m_curLine = pLine;
			}
		}
		else
		{
			delete m_prevLine;
			m_prevLine = 0;
			delete m_curLine;

			pLine = GetLine(true);
			m_curLine = pLine;
		}
	}
}

CLine *CDirectoryListingParser::GetLine(bool breakAtEnd /*=false*/)
{
	while (!m_DataList.empty())
	{	
		// Trim empty lines and spaces
		std::list<t_list>::iterator iter = m_DataList.begin();
		int len = iter->len;
		while (iter->p[startOffset]=='\r' || iter->p[startOffset]=='\n' || iter->p[startOffset]==' ' || iter->p[startOffset]=='\t')
		{
			startOffset++;
			if (startOffset >= len)
			{
				delete [] iter->p;
				iter++;
				startOffset = 0;
				if (iter == m_DataList.end())
				{
					m_DataList.clear();
					return 0;
				}
				len = iter->len;
			}
		}
		m_DataList.erase(m_DataList.begin(), iter);
		iter = m_DataList.begin();

		// Remember start offset and find next linebreak
		int startpos = startOffset;
		int reslen = 0;

		int emptylen = 0;

		int newStartOffset = startOffset;
		while ((iter->p[newStartOffset] != '\n') && (iter->p[newStartOffset] != '\r'))
		{
			if (iter->p[newStartOffset] != ' ' && iter->p[newStartOffset] != '\t')
			{
				reslen += emptylen + 1;
				emptylen = 0;
			}
			else
				emptylen++;
		
			newStartOffset++;
			if (newStartOffset >= len)
			{
				iter++;
				if (iter == m_DataList.end())
				{
					if (breakAtEnd)
						return 0;
					break;
				}
				len = iter->len;
				newStartOffset = 0;
			}
		}
		startOffset = newStartOffset;

		// Reslen is now the length of the line, excluding any terminating whitespace
		char *res = new char[reslen + 1];
		res[reslen] = 0;

		int	respos = 0;

		// Copy line data
		std::list<t_list>::iterator i = m_DataList.begin();
		while (i != iter && reslen)
		{
			int copylen = i->len - startpos;
			if (copylen > reslen)
				copylen = reslen;
			memcpy(&res[respos], &i->p[startpos], copylen);
			reslen -= copylen;
			respos += i->len - startpos;
			startpos = 0;

			delete [] i->p;
			i++;
		}
		// Delete all extra whitespace
		while (i != iter)
		{
			delete [] i->p;
			i++;
		}

		// Copy last chunk
		if (iter != m_DataList.end() && reslen)
		{
			int copylen = startOffset-startpos;
			if (copylen>reslen)
				copylen=reslen;
			memcpy(&res[respos], &iter->p[startpos], copylen);
			if (reslen >= iter->len)
			{
				delete [] iter->p;
				m_DataList.erase(m_DataList.begin(), ++iter);
			}
			else
				m_DataList.erase(m_DataList.begin(), iter);
		}
		else
			m_DataList.erase(m_DataList.begin(), iter);

		wxChar* buffer = m_pControlSocket->ConvToLocalBuffer(res);

		delete [] res;

		if (!buffer)
			continue;

		return new CLine(buffer);
	}

	return 0;
}

bool CDirectoryListingParser::ParseAsWfFtp(CLine *pLine, CDirentry &entry)
{
	int index = 0;
	CToken token;

	// Get filename
	if (!pLine->GetToken(index++, token))
		return false;

	entry.name = token.GetString();

	// Get filesize
	if (!pLine->GetToken(index++, token))
		return false;

	if (!token.IsNumeric())
		return false;

	entry.size = token.GetNumber();

	// Parse daste
	if (!pLine->GetToken(index++, token))
		return false;

	if (!ParseShortDate(token, entry))
		return false;

	// Unused token
	if (!pLine->GetToken(index++, token))
		return false;

	if (token.GetString().Right(1) != _T("."))
		return false;

	// Parse time
	if (!pLine->GetToken(index++, token, true))
		return false;

	if (!ParseTime(token, entry))
		return false;

	entry.dir = false;
	entry.link = false;
	entry.ownerGroup = _T("");
	entry.permissions = _T("");
	
	return true;
}

bool CDirectoryListingParser::ParseAsIBM_MVS(CLine *pLine, CDirentry &entry)
{
	int index = 0;
	CToken token;

	// volume
	if (!pLine->GetToken(index++, token))
		return false;

	// unit
	if (!pLine->GetToken(index++, token))
		return false;

	// Referred date
	if (!pLine->GetToken(index++, token))
		return false;

	if (token.GetString() == _T("**NONE**"))
		entry.hasDate = false;
	else if (!ParseShortDate(token, entry))
	{
		// Perhaps of the following type:
		// TSO004 3390 VSAM FOO.BAR
		if (token.GetString() != _T("VSAM"))
			return false;

		if (!pLine->GetToken(index++, token))
			return false;

		entry.name = token.GetString();
		if (entry.name.Find(' ') != -1)
			return false;

		entry.size = -1;
		entry.dir = false;
		entry.ownerGroup = _T("");
		entry.permissions = _T("");
		entry.hasDate = false;
		entry.hasTime = false;
		entry.link = false;

		return true;
	}

	// ext
	if (!pLine->GetToken(index++, token))
		return false;
	if (!token.IsNumeric())
		return false;

	int prevLen = token.GetLength();

	// used
	if (!pLine->GetToken(index++, token))
		return false;
	if (token.IsNumeric())
	{
		// recfm
		if (!pLine->GetToken(index++, token))
			return false;
		if (token.IsNumeric())
			return false;
	}
	else
	{
		if (prevLen < 6)
			return false;
	}

	// lrecl
	if (!pLine->GetToken(index++, token))
		return false;
	if (!token.IsNumeric())
		return false;

	// blksize
	if (!pLine->GetToken(index++, token))
		return false;
	if (!token.IsNumeric())
		return false; 

	// dsorg
	if (!pLine->GetToken(index++, token))
		return false;

	if (token.GetString() == _T("PO"))
	{
		entry.dir = true;
		entry.size = -1;
	}
	else
	{
		entry.dir = false;
		entry.size = 100;
	}
	
	// name of dataset or sequential file
	if (!pLine->GetToken(index++, token, true))
		return false;

	entry.name = token.GetString();

	entry.ownerGroup = _T("");
	entry.permissions = _T("");
	entry.hasTime = false;
	entry.link = false;
	
	return true;
}

bool CDirectoryListingParser::ParseAsIBM_MVS_PDS(CLine *pLine, CDirentry &entry)
{
	int index = 0;
	CToken token;

	// pds member name
	if (!pLine->GetToken(index++, token))
		return false;
	entry.name = token.GetString();

	// vv.mm
	if (!pLine->GetToken(index++, token))
		return false;
	
	// creation date
	if (!pLine->GetToken(index++, token))
		return false;
	if (!ParseShortDate(token, entry))
		return false;

	// modification date
	if (!pLine->GetToken(index++, token))
		return false;
	if (!ParseShortDate(token, entry))
		return false;

	// modification time
	if (!pLine->GetToken(index++, token))
		return false;
	if (!ParseTime(token, entry))
		return false;

	// size
	if (!pLine->GetToken(index++, token))
		return false;
	if (!token.IsNumeric())
		return false;
	entry.size = token.GetNumber();

	// init
	if (!pLine->GetToken(index++, token))
		return false;
	if (!token.IsNumeric())
		return false;

	// mod
	if (!pLine->GetToken(index++, token))
		return false;
	if (!token.IsNumeric())
		return false;

	// id
	if (!pLine->GetToken(index++, token, true))
		return false;	
	
	entry.dir = false;
	entry.ownerGroup = _T("");
	entry.permissions = _T("");
	entry.hasTime = false;
	entry.link = false;
	
	return true;
}

bool CDirectoryListingParser::ParseAsIBM_MVS_PDS2(CLine *pLine, CDirentry &entry)
{
	int index = 0;
	CToken token;
	if (!pLine->GetToken(index, token))
		return false;

	entry.name = token.GetString();

	entry.dir = false;
	entry.link = false;
	entry.ownerGroup = _T("");
	entry.permissions = _T("");
	entry.size = -1;
	entry.hasDate = false;
	entry.hasTime = false;

	if (!pLine->GetToken(++index, token))
		return true;

	entry.size = token.GetNumber(CToken::hex);
	if (entry.size == -1)
		return false;

	// Unused hexadecimal token
	if (!pLine->GetToken(++index, token))
		return false;
	if (!token.IsNumeric(CToken::hex))
		return false;

	// Unused numeric token
	if (!pLine->GetToken(++index, token))
		return false;
	if (!token.IsNumeric())
		return false;
	
	int start = ++index;
	while (pLine->GetToken(index, token))
	{
		index++;
	}
	if ((index - start < 2))
		return false;
	index--;

	pLine->GetToken(index, token);
	if (!token.IsNumeric() && (token.GetString() != _T("ANY")))
		return false;

	pLine->GetToken(index - 1, token);
	if (!token.IsNumeric() && (token.GetString() != _T("ANY")))
		return false;

	for (int i = start; i < index - 1; i++)
	{
		pLine->GetToken(i, token);
		int len = token.GetLength();
		for (int j = 0; j < len; j++)
			if (token[j] < 'A' || token[j] > 'Z')
				return false;
	}

	return true;
}

bool CDirectoryListingParser::ParseComplexFileSize(CToken& token, wxLongLong& size)
{
	if (token.IsNumeric())
	{
		size = token.GetNumber();
		return true;
	}

	int len = token.GetLength() - 1;

	char last = token[len];
	if (last == 'B' || last == 'b')
	{
		char c = token[len];
		if (c < '0' || c > '9')
		{
			last = token[len];
			len--;
		}
	}

	size = 0;

	int dot = -1;
	for (int i = 0; i < len; i++)
	{
		char c = token[i];
		if (c >= '0' && c <= '9')
		{
			size *= 10;
			size += c - '0';
		}
		else if (c == '.')
		{
			if (dot != -1)
				return false;
			dot = len - i - 1;
		}
		else
			return false;
	}
	switch (last)
	{
	case 'k':
	case 'K':
		size *= 1000;
		break;
	case 'm':
	case 'M':
		size *= 1000 * 1000;
		break;
	case 'g':
	case 'G':
		size *= 1000 * 1000 * 1000;
		break;
	case 't':
	case 'T':
		size *= 1000 * 1000;
		size *= 1000 * 1000;
		break;
	case 'b':
	case 'B':
		break;
	default:
		return false;
	}
	while (dot-- > 0)
		size /= 10;

	return true;
}
