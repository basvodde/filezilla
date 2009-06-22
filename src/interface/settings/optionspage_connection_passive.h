#ifndef __OPTIONSPAGE_CONNECTION_PASSIVE_H__
#define __OPTIONSPAGE_CONNECTION_PASSIVE_H__

class COptionsPageConnectionPassive : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_CONNECTION_PASSIVE"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();
};

#endif //__OPTIONSPAGE_CONNECTION_PASSIVE_H__
