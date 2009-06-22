#ifndef __OPTIONSPAGE_INTERFACE_H__
#define __OPTIONSPAGE_INTERFACE_H__

class COptionsPageInterface : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_INTERFACE"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

	DECLARE_EVENT_TABLE();
	void OnLayoutChange(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_INTERFACE_H__
