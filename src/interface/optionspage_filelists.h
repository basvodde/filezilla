#ifndef __OPTIONSPAGE_FILELISTS_H__
#define __OPTIONSPAGE_FILELISTS_H__

class COptionsPageFilelists : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_FILELISTS"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();
};

#endif //__OPTIONSPAGE_FILELISTS_H__
