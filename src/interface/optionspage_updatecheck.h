#ifndef __OPTIONSPAGE_UPDATECHECK_H__
#define __OPTIONSPAGE_UPDATECHECK_H__

#if FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

class COptionsPageUpdateCheck : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_UPDATECHECK"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:
	void OnRunUpdateCheck(wxCommandEvent& event);

	DECLARE_EVENT_TABLE();
};

#endif //FZ_MANUALUPDATECHECK && FZ_AUTOUPDATECHECK

#endif //__OPTIONSPAGE_UPDATECHECK_H__
