#ifndef __OPTIONSPAGE_SIZEFORMATTING_H__
#define __OPTIONSPAGE_SIZEFORMATTING_H__

class COptionsPageSizeFormatting : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_SIZEFORMATTING"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();
};

#endif //__OPTIONSPAGE_SIZEFORMATTING_H__
