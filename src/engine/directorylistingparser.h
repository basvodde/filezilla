#ifndef __DIRECTORYLISTINGPARSER_H__
#define __DIRECTORYLISTINGPARSER_H__

class CLine;
class CDirectoryListingParser
{
public:
	CDirectoryListingParser();
	~CDirectoryListingParser();

	bool Parse();
	bool ParseLine(CLine *pLine);

	bool ParseAsUnix(CLine *pLine, CDirentry &entry);

protected:
	std::map<wxString, int> m_MonthNamesMap;
};

#endif