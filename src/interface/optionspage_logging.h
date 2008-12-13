#ifndef __OPTIONSPAGE_LOGGING_H__
#define __OPTIONSPAGE_LOGGING_H__

class COptionsPageLogging : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_LOGGING"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:
	void SetCtrlState();

	DECLARE_EVENT_TABLE()
	void OnBrowse(wxCommandEvent& event);
	void OnCheck(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_LOGGING_H__
