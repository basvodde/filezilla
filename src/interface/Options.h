#ifndef __OPTIONS_H__
#define __OPTIONS_H_

#define OPTIONS_NUM OPTIONS_ENGINE_NUM

struct t_OptionsCache
{
	bool cached;
	int numValue;
	wxString strValue;
};

class TiXmlDocument;
class COptions : public COptionsBase
{
public:
	COptions();
	virtual ~COptions();

	virtual int GetOptionVal(unsigned int nID);
	virtual wxString GetOption(unsigned int nID);

	virtual bool SetOption(unsigned int nID, int value);
	virtual bool SetOption(unsigned int nID, wxString value);

protected:
	void Validate(unsigned int nID, int &value);
	void Validate(unsigned int nID, wxString &value);

	void SetXmlValue(unsigned int nID, wxString value);
	bool GetXmlValue(unsigned int nID, wxString &value);

	void CreateNewXmlDocument();

	TiXmlDocument *m_pXmlDocument;
	bool m_allowSave;

	t_OptionsCache m_optionsCache[OPTIONS_NUM];
};

#endif
