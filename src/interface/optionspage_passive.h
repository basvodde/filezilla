#ifndef __OPTIONSPAGE_PASSIVE_H__
#define __OPTIONSPAGE_PASSIVE_H__

class COptionsPagePassive : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_PASSIVE"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:
	virtual void SetCtrlState();

	DECLARE_EVENT_TABLE();
	void OnToggleLimitPorts(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_PASSIVE_H__
