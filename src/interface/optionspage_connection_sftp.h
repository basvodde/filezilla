#ifndef __OPTIONSPAGE_CONNECTION_SFTP_H__
#define __OPTIONSPAGE_CONNECTION_SFTP_H__

#include <wx/process.h>

class COptionsPageConnectionSFTP : public COptionsPage
{
public:
	COptionsPageConnectionSFTP();
	virtual ~COptionsPageConnectionSFTP() {}
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_CONNECTION_SFTP"); }
	virtual bool LoadPage();
	virtual bool SavePage();

protected:
	enum ReplyCode
	{
		success,
		error,
		failure // Like program terminated
	};

	bool AddKey(wxString keyFile, bool silent);
	bool LoadKeyFile(wxString& keyFile, bool silent, wxString& comment, wxString& data);
	bool LoadProcess();
	bool Send(const wxString& cmd);
	enum ReplyCode GetReply(wxString& reply);

	wxProcess* m_pProcess;
	bool m_initialized;

	DECLARE_EVENT_TABLE()
	void OnEndProcess(wxProcessEvent& event);
	void OnAdd(wxCommandEvent& event);
	void OnRemove(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_CONNECTION_SFTP_H__
