#ifndef __OPTIONSPAGE_CONNECTION_ACTIVE_H__
#define __OPTIONSPAGE_CONNECTION_ACTIVE_H__

class COptionsPageConnectionActive : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_CONNECTION_ACTIVE"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:
	virtual void SetCtrlState();

	DECLARE_EVENT_TABLE();
	void OnRadioOrCheckEvent(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_CONNECTION_ACTIVE_H__
