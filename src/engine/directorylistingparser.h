#ifndef __DIRECTORYLISTINGPARSER_H__
#define __DIRECTORYLISTINGPARSER_H__

class CLine;
class CToken;
class CDirectoryListingParser
{
public:
	CDirectoryListingParser(CFileZillaEngine *pEngine);
	~CDirectoryListingParser();

	bool Parse();

protected:

	CFileZillaEngine *m_pEngine;
	bool ParseLine(CLine *pLine, std::list<CDirentry> &entryList);

	bool ParseAsUnix(CLine *pLine, CDirentry &entry);
	bool ParseAsDos(CLine *pLine, CDirentry &entry);
	bool ParseAsEplf(CLine *pLine, CDirentry &entry);
	bool ParseAsVms(CLine *pLine, CDirentry &entry);
	bool ParseAsIbm(CLine *pLine, CDirentry &entry);
	bool ParseOther(CLine *pLine, CDirentry &entry);

	// Date / time parsers
	bool ParseUnixDateTime(CLine *pLine, int &index, CDirentry &entry);
	bool ParseShortDate(CToken &token, CDirentry &entry);
	bool ParseTime(CToken &token, CDirentry &entry);

	std::map<wxString, int> m_MonthNamesMap;
};

#endif
