#ifndef __DIRECTORYLISTINGPARSER_H__
#define __DIRECTORYLISTINGPARSER_H__

class CLine;
class CToken;
class CDirectoryListingParser
{
public:
	CDirectoryListingParser(CFileZillaEngine *pEngine, enum ServerType serverType);
	~CDirectoryListingParser();

	CDirectoryListing* Parse(const CServerPath &path);

	void AddData(char *pData, int len);

protected:
	CLine *GetLine(bool breakAtEnd = false);
	bool ParseLine(CLine *pLine, const enum ServerType serverType);

	bool ParseAsUnix(CLine *pLine, CDirentry &entry);
	bool ParseAsDos(CLine *pLine, CDirentry &entry);
	bool ParseAsEplf(CLine *pLine, CDirentry &entry);
	bool ParseAsVms(CLine *pLine, CDirentry &entry);
	bool ParseAsIbm(CLine *pLine, CDirentry &entry);
	bool ParseOther(CLine *pLine, CDirentry &entry);
	bool ParseAsWfFtp(CLine *pLine, CDirentry &entry);
	bool ParseAsIBM_MVS(CLine *pLine, CDirentry &entry);
	bool ParseAsIBM_MVS_PDS(CLine *pLine, CDirentry &entry);
	bool ParseAsIBM_MVS_PDS2(CLine *pLine, CDirentry &entry);

	// Date / time parsers
	bool ParseUnixDateTime(CLine *pLine, int &index, CDirentry &entry);
	bool ParseShortDate(CToken &token, CDirentry &entry);
	bool ParseTime(CToken &token, CDirentry &entry);

	// Parse file sizes given like this: 123.4M
	bool ParseComplexFileSize(CToken& token, wxLongLong& size);

	CFileZillaEngine *m_pEngine;

	std::map<wxString, int> m_MonthNamesMap;

	struct t_list
	{
		char *p;
		int len;
	};
	int startOffset;

	std::list<t_list> m_DataList;
	std::list<CDirentry> m_entryList;

	CLine *m_curLine;
	CLine *m_prevLine;

	enum ServerType m_serverType;
};

#endif
