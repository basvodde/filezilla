#ifndef __OPTIONSPAGE_FILETYPE_H__
#define __OPTIONSPAGE_FILETYPE_H__

class COptionsPageFiletype : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_FILETYPE"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();
};

#endif //__OPTIONSPAGE_FILETYPE_H__
