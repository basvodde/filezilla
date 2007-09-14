#ifndef __OPTIONSPAGE_DATEFORMATTING_H__
#define __OPTIONSPAGE_DATEFORMATTING_H__

class COptionsPageDateFormatting : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_DATEFORMATTING"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:

	void SetCtrlState();

	DECLARE_EVENT_TABLE();
	void OnRadioChanged(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_DATEFORMATTING_H__
