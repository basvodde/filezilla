#ifndef __OPTIONSPAGE_EDIT_H__
#define __OPTIONSPAGE_EDIT_H__

class COptionsPageEdit : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_EDIT"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:

	DECLARE_EVENT_TABLE();
};

#endif //__OPTIONSPAGE_EDIT_H__
