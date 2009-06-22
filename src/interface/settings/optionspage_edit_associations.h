#ifndef __OPTIONSPAGE_EDIT_ASSOCIATIONS_H__
#define __OPTIONSPAGE_EDIT_ASSOCIATIONS_H__

class COptionsPageEditAssociations : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_EDIT_ASSOCIATIONS"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();
};

#endif //__OPTIONSPAGE_EDIT_ASSOCIATIONS_H__
