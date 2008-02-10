#ifndef __OPTIONSPAGE_LOGGING_H__
#define __OPTIONSPAGE_LOGGING_H__

class COptionsPageLogging : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_LOGGING"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();
};

#endif //__OPTIONSPAGE_LOGGING_H__
