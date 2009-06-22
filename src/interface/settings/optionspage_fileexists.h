#ifndef __OPTIONSPAGE_FILEEXISTS_H__
#define __OPTIONSPAGE_FILEEXISTS_H__

class COptionsPageFileExists : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_DEFAULTFILEEXISTS"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();
};

#endif //__OPTIONSPAGE_FILEEXISTS_H__
