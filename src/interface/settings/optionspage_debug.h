#ifndef __OPTIONSPAGE_DEBUG_H__
#define __OPTIONSPAGE_DEBUG_H__

class COptionsPageDebug : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_DEBUG"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();
};

#endif //__OPTIONSPAGE_DEBUG_H__
