#ifndef __OPTIONS_H__
#define __OPTIONS_H_

#define OPTIONS_NUM (OPTIONS_ENGINE_NUM + 2)

#define OPTION_NUMTRANSFERS (OPTIONS_ENGINE_NUM + 0)
#define OPTION_TRANSFERRETRYCOUNT (OPTIONS_ENGINE_NUM + 1)

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
	void FreeXml();
	
	// path is element path below document root, separated by slashes
	void SetServer(wxString path, const CServer& server);
	bool GetServer(wxString path, CServer& server);
	
	void SetServer(TiXmlElement *node, const CServer& server) const;
	bool GetServer(TiXmlElement *node, CServer& server);
	
	static char* ConvUTF8(wxString value);
	static wxString ConvLocal(const char *value);
	static void AddTextElement(TiXmlElement* node, const char* name, const wxString& value);

protected:
	int Validate(unsigned int nID, int value);
	wxString Validate(unsigned int nID, wxString value);

	void SetXmlValue(unsigned int nID, wxString value);
	bool GetXmlValue(unsigned int nID, wxString &value);

	void CreateNewXmlDocument();

	TiXmlDocument *m_pXmlDocument;
	bool m_allowSave;

	t_OptionsCache m_optionsCache[OPTIONS_NUM];
	bool m_acquired;
};

#endif
