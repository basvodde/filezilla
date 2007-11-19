#ifndef __OPTIONSPAGE_FTPPROXY_H__
#define __OPTIONSPAGE_FTPPROXY_H__

class COptionsPageFtpProxy : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_CONNECTION_FTP_PROXY"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:

	void SetCtrlState();

	DECLARE_EVENT_TABLE();
	void OnProxyTypeChanged(wxCommandEvent& event);
	void OnLoginSequenceChanged(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_FTPPROXY_H__
