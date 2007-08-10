#include "FileZilla.h"
#include "directorylistingparser.h"
#include "ControlSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

std::map<wxString, int> CDirectoryListingParser::m_MonthNamesMap;

//#define LISTDEBUG_MVS
//#define LISTDEBUG
#ifdef LISTDEBUG
static char data[][110]={
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

	// Migrated MVS file
	"Migrated				SOME.FILE",

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

	wxChar operator[](unsigned int n) const
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
	CLine(wxChar* p, int len = -1)
	{
		m_pLine = p;
		if (len != -1)
			m_len = len;
		else
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
				token = *(m_LineEndTokens[n]);
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
		memcpy(p, m_pLine, m_len * sizeof(wxChar));
		p[m_len] = ' ';
		memcpy(p + m_len + 1, pLine->m_pLine, pLine->m_len * sizeof(wxChar));
		
		return new CLine(p, m_len + pLine->m_len + 1);
	}

protected:
	std::vector<CToken *> m_Tokens;
	std::vector<CToken *> m_LineEndTokens;
	int m_parsePos;
	int m_len;
	wxChar* m_pLine;
};

CDirectoryListingParser::CDirectoryListingParser(CControlSocket* pControlSocket, const CServer& server)
	: m_pControlSocket(pControlSocket), m_server(server)
{
	m_currentOffset = 0;
	m_prevLine = 0;
	m_fileListOnly = true;
	m_maybeMultilineVms = false;

	if (m_MonthNamesMap.empty())
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
		m_MonthNamesMap[_T("juni")] = 6;
		m_MonthNamesMap[_T("juli")] = 7;
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
		m_MonthNamesMap[_T("ott")] = 10;
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

		//Slovenian month names
		m_MonthNamesMap[_T("avg")] = 8;

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

	delete m_prevLine;
}

void CDirectoryListingParser::ParseData(bool partial)
{
	CLine *pLine = GetLine(partial);
	while (pLine)
	{
		bool res = ParseLine(pLine, m_server.GetType(), false);
		if (!res)
		{
			if (m_prevLine)
			{
				CLine* pConcatenatedLine = m_prevLine->Concat(pLine);
				bool res = ParseLine(pConcatenatedLine, m_server.GetType(), true);
				delete pConcatenatedLine;
				delete m_prevLine;

				if (res)
				{
					delete pLine;
					m_prevLine = 0;
				}
				else
					m_prevLine = pLine;
			}
			else
				m_prevLine = pLine;
		}
		else
		{
			delete m_prevLine;
			m_prevLine = 0;
			delete pLine;
		}
		pLine = GetLine(partial);
	};
}

CDirectoryListing CDirectoryListingParser::Parse(const CServerPath &path)
{
	ParseData(false);

	CDirectoryListing listing;
	listing.path = path;
	listing.m_firstListTime = CTimeEx::Now();
	
	if (!m_fileList.empty())
	{
		wxASSERT(m_entryList.empty());
		
		listing.SetCount(m_fileList.size());
		unsigned int i = 0;
		for (std::list<wxString>::const_iterator iter = m_fileList.begin(); iter != m_fileList.end(); iter++, i++)
		{
			CDirentry entry;
			entry.name = *iter;
			entry.dir = false;
			entry.hasDate = false;
			entry.hasTime = false;
			entry.link = false;
			entry.size = -1;
			entry.unsure = false;
			listing[i] = entry;
		}
	}
	else
	{
		listing.SetCount(m_entryList.size());
		listing.Assign(m_entryList);
	}

	return listing;
}

bool CDirectoryListingParser::ParseLine(CLine *pLine, const enum ServerType serverType, bool concatenated)
{
	CDirentry entry;
	bool res = ParseAsUnix(pLine, entry);
	if (res)
		goto done;
	res = ParseAsDos(pLine, entry);
	if (res)
		goto done;
	res = ParseAsMlsd(pLine, entry);
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
	res = ParseAsOS9(pLine, entry);
	if (res)
		goto done;
#ifndef LISTDEBUG_MVS
	if (serverType == MVS)
#endif //LISTDEBUG_MVS
	{
		res = ParseAsIBM_MVS_PDS2(pLine, entry);
		if (res)
			goto done;
		res = ParseAsIBM_MVS_Migrated(pLine, entry);
		if (res)
			goto done;
	}

	// Some servers just send a list of filenames. If a line could not be parsed,
	// check if it's a filename. If that's the case, store it for later, else clear
	// list of stored files.
	// If parsing finishes and no entries could be parsed and none of the lines 
	// contained a space, assume it's a raw filelisting.

	if (!concatenated)
	{
		CToken token;
		if (!pLine->GetToken(0, token, true) || token.Find(' ') != -1)
		{
			m_maybeMultilineVms = false;
			m_fileList.clear();
			m_fileListOnly = false;
		}
		else
		{
			m_maybeMultilineVms = token.Find(';') != -1;
			if (m_fileListOnly)
				m_fileList.push_back(token.GetString());
		}
	}
	else
		m_maybeMultilineVms = false;

	return false;
done:

	m_maybeMultilineVms = false;
	m_fileList.clear();
	m_fileListOnly = false;

	// Don't add . or ..
	if (entry.name == _T(".") || entry.name == _T(".."))
		return true;

	if (serverType == VMS && entry.dir)
	{
		// Trim version information from directories
		int pos = entry.name.Find(';', true);
		if (pos > 0)
			entry.name = entry.name.Left(pos);
	}

	int offset = m_server.GetTimezoneOffset();
	if (offset && entry.hasTime)
	{
		// Apply timezone offset
		wxTimeSpan span(0, offset, 0, 0);
		entry.time.Add(span);
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

	wxChar chr = token[0];
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

	int year = 0;
	int month = 0;
	int day = 0;
	long hour = 0;
	long minute = 0;

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
				day = token.GetNumber(pos + 1, token.GetLength() - pos - 1).GetLo();
				if (day < 1 || day > 31)
					return false;
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

			year = entry.time.GetYear();
			month = entry.time.GetMonth() - wxDateTime::Jan + 1;
			day = entry.time.GetDay();
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
			year = token.GetNumber().GetLo();
			if (!pLine->GetToken(++index, dateMonth))
				return false;
			mayHaveTime = false;
		}
		else
			dateMonth = token;
	}
	else
		dateMonth = token;

	if (!day)
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
			dateDay = dateMonth.GetNumber(0, dateMonth.GetLength() - offset).GetLo();
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
		day = dateDay;
	}

	if (!month)
	{
		wxString strMonth = dateMonth.GetString();
		if (dateMonth.IsLeftNumeric() && (unsigned int)strMonth[strMonth.Length() - 1] > 127)
		{
			// Most likely an Asian server sending some unknown language specific 
			// suffix at the end of the monthname. Filter it out.
			int i;
			for (i = strMonth.Length() - 1; i > 0; i--)
			{
				if (strMonth[i] >= '0' && strMonth[i] <= '9')
					break;
			}
			strMonth = strMonth.Left(i + 1);
		}
		// Check month name
		if (strMonth.Right(1) == _T(",") || strMonth.Right(1) == _T("."))
			strMonth.RemoveLast();
		strMonth.MakeLower();
		std::map<wxString, int>::iterator iter = m_MonthNamesMap.find(strMonth);
		if (iter == m_MonthNamesMap.end())
			return false;
		month = iter->second;
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
		if (!str.Left(pos).ToLong(&hour))
			return false;
		if (!str.Mid(pos + 1).ToLong(&minute))
			return false;

		if (hour < 0 || hour > 23)
			return false;
		if (minute < 0 || minute > 59)
			return false;

		entry.hasTime = true;

		// Some servers use times only for files newer than 6 months
		if (!year)
		{
			year = wxDateTime::Now().GetYear();
			int currentDayOfYear = wxDateTime::Now().GetDay() + 31 * (wxDateTime::Now().GetMonth() - wxDateTime::Jan);
			int fileDayOfYear = (month - 1) * 31 + (day - 1);
			if (currentDayOfYear <= fileDayOfYear)
				year -= 1;
		}
	}
	else if (!year)
	{
		// token is a year
		if (!token.IsNumeric() && !token.IsLeftNumeric())
			return false;

		year = token.GetNumber().GetLo();

		if (year > 3000)
			return false;
		if (year < 1000)
			year += 1900;

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

				if (!str.Left(pos).ToLong(&hour))
					return false;
				if (!str.Mid(pos + 1).ToLong(&minute))
					return false;

				if (hour < 0 || hour > 23)
					return false;
				if (minute < 0 || minute > 59)
					return false;

				entry.hasTime = true;
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
	{
		entry.hasTime = false;
		index--;
	}

	entry.time = wxDateTime();
	if (!VerifySetDate(entry.time, year, (wxDateTime::Month)(wxDateTime::Jan + month - 1), day, hour, minute))
		return false;
	entry.hasDate = true;

	return true;
}

bool CDirectoryListingParser::ParseShortDate(CToken &token, CDirentry &entry, bool saneFieldOrder /*=false*/)
{
	if (token.GetLength() < 1)
		return false;

	bool gotYear = false;
	bool gotMonth = false;
	bool gotDay = false;
	bool gotMonthName = false;

	int year = 0;
	int month = 0;
	int day = 0;

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
		month = iter->second;
		gotMonth = true;
		gotMonthName = true;
	}
	else if (pos == 4)
	{
		// Seems to be yyyy-mm-dd
		year = token.GetNumber(0, pos).GetLo();
		if (year < 1900 || year > 3000)
			return false;
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
			day = value.GetLo();
			gotDay = true;
		}
		else
		{
			if (saneFieldOrder)
			{
				year = value.GetLo();
				if (year < 50)
					year += 2000;
				else
					year += 1900;
				gotYear = true;
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

					day = value.GetLo();
					gotDay = true;
				}
				else
				{
					month = value.GetLo();
					gotMonth = true;
				}
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
		day = month;
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
		month = iter->second;
		gotMonth = true;
	}
	else
	{
		wxLongLong value = token.GetNumber(pos + 1, pos2 - pos - 1);
		// Day field in mm-dd-yyyy
		if (value < 1 || value > 31)
			return false;
		day = value.GetLo();
		gotDay = true;
	}

	value = token.GetNumber(pos2 + 1, token.GetLength() - pos2 - 1).GetLo();
	if (gotYear)
	{
		// Day field in yyy-mm-dd
		if (!value || value > 31)
			return false;
		day = value;
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
		year = value;

		gotYear = true;
	}

	entry.hasDate = true;

	if (!gotMonth || !gotDay || !gotYear)
		return false;

	entry.time = wxDateTime();
	if (!VerifySetDate(entry.time, year, (wxDateTime::Month)(wxDateTime::Jan + month - 1), day))
		return false;
	entry.hasDate = true;
		
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
		wxLongLong size = 0;
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

	entry.link = false;
	entry.target = _T("");
	entry.ownerGroup = _T("");
	entry.permissions = _T("");
	
	return true;
}

bool CDirectoryListingParser::ParseTime(CToken &token, CDirentry &entry)
{
	if (!entry.hasDate)
		return false;

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

	wxTimeSpan span(hour.GetLo(), minute.GetLo());
	entry.time.Add(span);
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

	entry.ownerGroup = _T("");
	entry.permissions = _T("");
	entry.dir = false;
	entry.link = false;
	entry.hasDate = false;
	entry.hasTime = false;
	entry.size = -1;
	
	int fact = 1;
	while (fact < pos)
	{
		int separator = token.Find(',', fact);
		int len;
		if (separator == -1)
			len = pos - fact;
		else
			len = separator - fact;

		if (!len)
		{
			fact++;
			continue;
		}

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
			entry.time = wxDateTime((time_t)number.GetValue());

			entry.hasTime = true;
			entry.hasDate = true;
		}
		else if (type == 'u' && len > 2 && token[fact + 1] == 'p')
			entry.permissions = token.GetString().Mid(fact + 2, len - 2);

		fact += len + 1;
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

	if (!pLine->GetToken(++index, token))
		return false;

	entry.ownerGroup = _T("");

	bool gotSize = false;
	// This field can either be the filesize, a username (at least that's what I think) enclosed in [] or a date.
	if (!token.IsNumeric() && !token.IsLeftNumeric())
	{
		// Must be username

		const int len = token.GetLength();
		if (len < 3 || token[0] != '[' || token[len - 1] != ']')
			return false;
		entry.ownerGroup = token.GetString().Mid(1, len - 2);

		if (!pLine->GetToken(++index, token))
			return false;
		if (!token.IsNumeric() && !token.IsLeftNumeric())
			return false;
	}

	// Current token is either size or date
	pos = token.Find('/');
	if (token.IsNumeric() || (pos != -1 && token.Find('/', pos + 1) == -1))
	{
		// Definitely size
		gotSize = true;
		entry.size = token.GetNumber();

		if (!pLine->GetToken(++index, token))
			return false;
	}

	// Get date
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

	if (!gotSize)
	{
		// Get size
		if (!pLine->GetToken(++index, token))
			return false;

		if (!token.IsNumeric() && !token.IsLeftNumeric())
			return false;
	
		entry.size = token.GetNumber();
	}

	// Owner / group and permissions
	entry.permissions = _T("");
	while (pLine->GetToken(++index, token))
	{
		const int len = token.GetLength();
		if (len > 2 && token[0] == '(' && token[len - 1] == ')')
		{
			if (entry.permissions != _T(""))
				entry.permissions += _T(" ");
			entry.permissions += token.GetString().Mid(1, len - 2);
		}
		else if (len > 2 && token[0] == '[' && token[len - 1] == ']')
		{
			if (entry.ownerGroup != _T(""))
				entry.ownerGroup += _T(" ");
			entry.ownerGroup += token.GetString().Mid(1, len - 2);
		}
		else
		{
			if (entry.permissions != _T(""))
				entry.permissions += _T(" ");
			entry.ownerGroup += token.GetString();
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
		entry.time = wxDateTime((time_t)number.GetValue());
		
		entry.hasTime = true;
		entry.hasDate = true;

		// Get filename
		if (!pLine->GetToken(++index, token, true))
			return false;

		entry.name = token.GetString();

		entry.link = false;
		entry.target = _T("");
	}
	else
	{
		// Possible conflict with multiline VMS listings
		if (m_maybeMultilineVms)
			return false;

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
			int month = iter->second;

			// Get day
			if (!pLine->GetToken(++index, token))
				return false;

			if (!token.IsNumeric() && !token.IsLeftNumeric())
				return false;

			wxLongLong day = token.GetNumber();
			if (day < 0 || day > 31)
				return false;

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
			
			entry.time = wxDateTime();
			if (!VerifySetDate(entry.time, year.GetLo(), (wxDateTime::Month)(month - 1), day.GetLo()))
				return false;

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
		entry.link = false;
		entry.target = _T("");
		entry.ownerGroup = _T("");
		entry.permissions = _T("");
	}

	return true;
}

void CDirectoryListingParser::AddData(char *pData, int len)
{
	t_list item;
	item.p = pData;
	item.len = len;

	m_DataList.push_back(item);

	ParseData(true);
}

void CDirectoryListingParser::AddLine(const wxChar* pLine)
{
	const int len = wxStrlen(pLine);

	wxChar* p = new wxChar[len + 1];
	wxStrcpy(p, pLine);

	CLine line(p, len);

	ParseLine(&line, m_server.GetType(), false);
}

CLine *CDirectoryListingParser::GetLine(bool breakAtEnd /*=false*/)
{
	while (!m_DataList.empty())
	{	
		// Trim empty lines and spaces
		std::list<t_list>::iterator iter = m_DataList.begin();
		int len = iter->len;
		while (iter->p[m_currentOffset]=='\r' || iter->p[m_currentOffset]=='\n' || iter->p[m_currentOffset]==' ' || iter->p[m_currentOffset]=='\t')
		{
			m_currentOffset++;
			if (m_currentOffset >= len)
			{
				delete [] iter->p;
				iter++;
				m_currentOffset = 0;
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
		int startpos = m_currentOffset;
		int reslen = 0;

		int emptylen = 0;

		int currentOffset = m_currentOffset;
		while ((iter->p[currentOffset] != '\n') && (iter->p[currentOffset] != '\r'))
		{
			if (iter->p[currentOffset] != ' ' && iter->p[currentOffset] != '\t')
			{
				reslen += emptylen + 1;
				emptylen = 0;
			}
			else
				emptylen++;
		
			currentOffset++;
			if (currentOffset >= len)
			{
				iter++;
				if (iter == m_DataList.end())
				{
					if (breakAtEnd)
						return 0;
					break;
				}
				len = iter->len;
				currentOffset = 0;
			}
		}
		m_currentOffset = currentOffset;

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
			int copylen = m_currentOffset-startpos;
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

		wxChar* buffer;
		if (m_pControlSocket)
		{
			buffer = m_pControlSocket->ConvToLocalBuffer(res);
			m_pControlSocket->LogMessageRaw(RawList, buffer);
		}
		else
		{
			wxString str(res, wxConvUTF8);
			if (str == _T(""))
				str = wxString(res, wxConvLocal);
			buffer = new wxChar[str.Len() + 1];
			wxStrcpy(buffer, str.c_str());
		}
		delete [] res;

		if (!buffer)
		{
			// Line contained no usable data, start over
			continue;
		}

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

bool CDirectoryListingParser::ParseAsIBM_MVS_Migrated(CLine *pLine, CDirentry &entry)
{
	// Migrated MVS file
	// "Migrated				SOME.NAME"

	int index = 0;
	CToken token;
	if (!pLine->GetToken(index, token))
		return false;

	if (token.GetString().CmpNoCase(_T("Migrated")))
		return false;

	if (!pLine->GetToken(++index, token))
		return false;

	entry.name = token.GetString();

	entry.dir = false;
	entry.link = false;
	entry.ownerGroup = _T("");
	entry.permissions = _T("");
	entry.size = -1;
	entry.hasDate = false;
	entry.hasTime = false;

	if (pLine->GetToken(++index, token))
		return false;

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
		size *= 1024;
		break;
	case 'm':
	case 'M':
		size *= 1024 * 1024;
		break;
	case 'g':
	case 'G':
		size *= 1024 * 1024 * 1024;
		break;
	case 't':
	case 'T':
		size *= 1024 * 1024;
		size *= 1024 * 1024;
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

bool CDirectoryListingParser::ParseAsMlsd(CLine *pLine, CDirentry &entry)
{
	// MLSD format as described here: http://www.ietf.org/internet-drafts/draft-ietf-ftpext-mlst-16.txt
	
	// Parsing is done strict, abort on slightest error.

	CToken token;

	if (!pLine->GetToken(0, token))
		return false;

	wxString facts = token.GetString();
	if (facts == _T(""))
		return false;

	entry.dir = false;
	entry.link = false;
	entry.size = -1;
	entry.hasDate = false;
	entry.hasTime = false;
	entry.ownerGroup = _T("");
	entry.permissions = _T("");

	while (facts != _T(""))
	{
		int delim = facts.Find(';');
		if (delim < 3)
			return false;

		int pos = facts.Find('=');
		if (pos < 1 || (pos + 2) >= delim)
			return false;

		wxString factname = facts.Left(pos).Lower();
		wxString value = facts.Mid(pos + 1, delim - pos - 1);
		if (factname == _T("type"))
		{
			if (!value.CmpNoCase(_T("dir")) ||
				!value.CmpNoCase(_T("cdir")) ||
				!value.CmpNoCase(_T("pdir")))
				entry.dir = true;
		}
		else if (factname == _T("size"))
		{
			entry.size = 0;

			for (unsigned int i = 0; i < value.Len(); i++)
			{
				if (value[i] < '0' || value[i] > '9')
					return false;
				entry.size *= 10;
				entry.size += value[i] - '0';
			}
		}
		else if (factname == _T("modify") ||
			(!entry.hasDate && factname == _T("create")))
		{
			wxDateTime dateTime;
			const wxChar* time = dateTime.ParseFormat(value, _T("%Y%m%d"));

			if (!time)
				return false;

			if (*time)
			{
				if (!dateTime.ParseFormat(time, _T("%H%M"), dateTime))
					return false;
				entry.hasTime = true;
			}
			else
				entry.hasTime = false;
			
			entry.time = dateTime.FromTimezone(wxDateTime::GMT0);

			entry.hasDate = true;
		}
		else if (factname == _T("perm"))
			entry.permissions = value;
		
		facts = facts.Mid(delim + 1);
	}

	if (!pLine->GetToken(1, token, true))
		return false;

	entry.name = token.GetString();

	entry.unsure = false;

	return true;
}

bool CDirectoryListingParser::ParseAsOS9(CLine *pLine, CDirentry &entry)
{
	int index = 0;
	CToken token;

	// Get owner
	if (!pLine->GetToken(index++, token))
		return false;

	// Make sure it's number.number
	int pos = token.Find('.');
	if (pos == -1 || !pos || pos == ((int)token.GetLength() - 1))
		return false;

	if (!token.IsNumeric(0, pos))
		return false;

	if (!token.IsNumeric(pos + 1, token.GetLength() - pos - 1))
		return false;

	entry.ownerGroup = token.GetString();

	// Get date
	if (!pLine->GetToken(index++, token))
		return false;

	if (!ParseShortDate(token, entry, true))
		return false;

	// Unused token
	if (!pLine->GetToken(index++, token))
		return false;

	// Get perms
	if (!pLine->GetToken(index++, token))
		return false;

	entry.permissions = token.GetString();

	entry.dir = token[0] == 'd';

	// Unused token
	if (!pLine->GetToken(index++, token))
		return false;

	// Get Size
	if (!pLine->GetToken(index++, token))
		return false;

	if (!token.IsNumeric())
		return false;

	entry.size = token.GetNumber();

	// Filename
	if (!pLine->GetToken(index++, token, true))
		return false;

	entry.name = token.GetString();

	entry.hasTime = false;
	entry.link = false;
	entry.unsure = false;

	return true;
}

void CDirectoryListingParser::Reset()
{
	for (std::list<t_list>::iterator iter = m_DataList.begin(); iter != m_DataList.end(); iter++)
		delete [] iter->p;
	m_DataList.clear();

	delete m_prevLine;
	m_prevLine = 0;

	m_entryList.clear();
	m_fileList.clear();
	m_currentOffset = 0;
	m_fileListOnly = true;
	m_maybeMultilineVms = false;
}
