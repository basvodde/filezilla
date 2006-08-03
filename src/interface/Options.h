#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#define OPTIONS_NUM (OPTIONS_ENGINE_NUM + 13)

#define OPTION_NUMTRANSFERS (OPTIONS_ENGINE_NUM + 0)
#define OPTION_TRANSFERRETRYCOUNT (OPTIONS_ENGINE_NUM + 1)
#define OPTION_ASCIIBINARY (OPTIONS_ENGINE_NUM + 2)
#define OPTION_ASCIIFILES (OPTIONS_ENGINE_NUM + 3)
#define OPTION_ASCIINOEXT (OPTIONS_ENGINE_NUM + 4)
#define OPTION_ASCIIDOTFILE (OPTIONS_ENGINE_NUM + 5)
#define OPTION_THEME (OPTIONS_ENGINE_NUM + 6)
#define OPTION_LANGUAGE (OPTIONS_ENGINE_NUM + 7)
#define OPTION_LASTSERVERPATH (OPTIONS_ENGINE_NUM + 8)
#define OPTION_CONCURRENTDOWNLOADLIMIT (OPTIONS_ENGINE_NUM + 9)
#define OPTION_CONCURRENTUPLOADLIMIT (OPTIONS_ENGINE_NUM + 10)
#define OPTION_UPDATECHECK (OPTIONS_ENGINE_NUM + 11)
#define OPTION_UPDATECHECK_INTERVAL (OPTIONS_ENGINE_NUM + 12)

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
