#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#define OPTIONS_NUM (OPTIONS_ENGINE_NUM + 6)

#define OPTION_NUMTRANSFERS (OPTIONS_ENGINE_NUM + 0)
#define OPTION_TRANSFERRETRYCOUNT (OPTIONS_ENGINE_NUM + 1)
#define OPTION_ASCIIBINARY (OPTIONS_ENGINE_NUM + 2)
#define OPTION_ASCIIFILES (OPTIONS_ENGINE_NUM + 3)
#define OPTION_ASCIINOEXT (OPTIONS_ENGINE_NUM + 4)
#define OPTION_ASCIIDOTFILE (OPTIONS_ENGINE_NUM + 5)

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
	COptions();
	virtual ~COptions();

	virtual int GetOptionVal(unsigned int nID);
	virtual wxString GetOption(unsigned int nID);

	virtual bool SetOption(unsigned int nID, int value);
	virtual bool SetOption(unsigned int nID, wxString value);
	
	TiXmlElement* GetXml();
	void FreeXml(bool save);
	
	// path is element path below document root, separated by slashes
	void SetServer(wxString path, const CServer& server);
	bool GetServer(wxString path, CServer& server);
	
protected:
	int Validate(unsigned int nID, int value);
	wxString Validate(unsigned int nID, wxString value);

	void SetXmlValue(unsigned int nID, wxString value);
	bool GetXmlValue(unsigned int nID, wxString &value);

	void CreateSettingsXmlElement();

	TiXmlDocument *m_pXmlDocument;

	t_OptionsCache m_optionsCache[OPTIONS_NUM];
	bool m_acquired;
};

#endif //__OPTIONS_H__
