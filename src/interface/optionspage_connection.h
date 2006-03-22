#ifndef __OPTIONSPAGE_CONNECTION_H__
#define __OPTIONSPAGE_CONNECTION_H__

class COptionsPageConnection : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_CONNECTION"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:
	DECLARE_EVENT_TABLE()
	void OnWizard(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_CONNECTION_H__
