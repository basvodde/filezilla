#ifndef __OPTIONSPAGE_CONNECTION_FTP_H__
#define __OPTIONSPAGE_CONNECTION_FTP_H__

class COptionsPageConnectionFTP : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_CONNECTION_FTP"); }
	virtual bool LoadPage();
	virtual bool SavePage();
};

#endif //__OPTIONSPAGE_CONNECTION_FTP_H__
