#ifndef __OPTIONS_H__
#define __OPTIONS_H__

enum interfaceOptions
{
	OPTION_NUMTRANSFERS = OPTIONS_ENGINE_NUM,
	OPTION_TRANSFERRETRYCOUNT,
	OPTION_ASCIIBINARY,
	OPTION_ASCIIFILES,
	OPTION_ASCIINOEXT,
	OPTION_ASCIIDOTFILE,
	OPTION_THEME,
	OPTION_LANGUAGE,
	OPTION_LASTSERVERPATH,
	OPTION_CONCURRENTDOWNLOADLIMIT,
	OPTION_CONCURRENTUPLOADLIMIT,
	OPTION_UPDATECHECK,
	OPTION_UPDATECHECK_INTERVAL,
	OPTION_UPDATECHECK_LASTDATE,
	OPTION_UPDATECHECK_NEWVERSION,
	OPTION_UPDATECHECK_URL,
	OPTION_DEBUG_MENU,
	OPTION_FILEEXISTS_DOWNLOAD,
	OPTION_FILEEXISTS_UPLOAD,
	OPTION_ASCIIRESUME,
	OPTION_GREETINGVERSION,
	OPTION_ONETIME_DIALOGS,

	// Has to be last element
	OPTIONS_NUM
};

struct t_OptionsCache
{
	bool cached;
	int numValue;
	wxString strValue;
};

class TiXmlDocument;
class TiXmlElement;
class COptions : public COptionsBase
{
public:
	virtual int GetOptionVal(unsigned int nID);
	virtual wxString GetOption(unsigned int nID);

	virtual bool SetOption(unsigned int nID, int value);
	virtual bool SetOption(unsigned int nID, wxString value);
	
	TiXmlElement* GetXml();
	void FreeXml(bool save);
	
	// path is element path below document root, separated by slashes
	void SetServer(wxString path, const CServer& server);
	bool GetServer(wxString path, CServer& server);

	void SetLastServer(const CServer& server);
	bool GetLastServer(CServer& server);

	static COptions* Get();
	static void Init();
	static void Destroy();
	
protected:
	COptions();
	virtual ~COptions();

	int Validate(unsigned int nID, int value);
	wxString Validate(unsigned int nID, wxString value);

	void SetXmlValue(unsigned int nID, wxString value);
	bool GetXmlValue(unsigned int nID, wxString &value);

	void CreateSettingsXmlElement();

	TiXmlDocument *m_pXmlDocument;

	t_OptionsCache m_optionsCache[OPTIONS_NUM];
	bool m_acquired;

	CServer* m_pLastServer;

	static COptions* m_theOptions;
};

#endif //__OPTIONS_H__
