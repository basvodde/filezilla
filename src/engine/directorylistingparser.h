#ifndef __DIRECTORYLISTINGPARSER_H__
#define __DIRECTORYLISTINGPARSER_H__

/* This class is responsible for parsing the directory listings returned by
 * the server.
 * Unfortunatly, RFC959 did not specify the format of directory listings, so
 * each server uses its own format. In addition to that, in most cases the 
 * listings were not designed to be machine-parsable, they were meant to be
 * human readable by users of that particular server.
 * By far the most common format is the one returned by the Unix "ls -l"
 * command. However, legacy systems are still in place, especially in big
 * companies. These often use very exotic listing styles.
 * Another problem are localized listings containing date strings. In some
 * cases these listings are ambiguous and cannot be distinguished.
 * Example for an ambiguous date: 04-05-06. All of the 6 permutations for
 * the location of year, month and day are valid dates.
 * Some servers send multiline listings where a single entry can span two
 * lines, this has to be detected as well, as far as possible.
 *
 * Some servers send MVS style listings which can consist of just the 
 * filename without any additional data. In order to prevent problems, this 
 * format is only parsed if the server is in fact recognizes as MVS server.
 *
 * Please see tests/dirparsertest.cpp for a list of supported formats and the
 * expected parser result.
 *
 * If adding data to the parser, it first decomposes the raw data into lines,
 * which then are processed further. Each line gets consecutively tested for
 * different formats, starting with the most common Unix style format.
 * Lines not containing a recognized format (e.g. a part of a multiline
 * entry) are rememberd and if the next line cannot be parsed either, they
 * get concatenated to be parsed again (and discarded if not recognized).
 */

class CLine;
class CToken;
class CControlSocket;
class CDirectoryListingParser
{
public:
	CDirectoryListingParser(CControlSocket* pControlSocket, const CServer& server);
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
	bool ParseAsMlsd(CLine *pLine, CDirentry &entry);

	// Date / time parsers
	bool ParseUnixDateTime(CLine *pLine, int &index, CDirentry &entry);
	bool ParseShortDate(CToken &token, CDirentry &entry);
	bool ParseTime(CToken &token, CDirentry &entry);

	// Parse file sizes given like this: 123.4M
	bool ParseComplexFileSize(CToken& token, wxLongLong& size);

	CControlSocket* m_pControlSocket;

	static std::map<wxString, int> m_MonthNamesMap;
	static bool m_MonthNamesMapInitialized;

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

	const CServer& m_server;
};

#endif
