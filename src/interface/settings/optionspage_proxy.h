#ifndef __OPTIONSPAGE_PROXY_H__
#define __OPTIONSPAGE_PROXY_H__

class COptionsPageProxy : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_CONNECTION_PROXY"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:

	void SetCtrlState();

	DECLARE_EVENT_TABLE();
	void OnProxyTypeChanged(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_PROXY_H__
