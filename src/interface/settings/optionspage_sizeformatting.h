#ifndef __OPTIONSPAGE_SIZEFORMATTING_H__
#define __OPTIONSPAGE_SIZEFORMATTING_H__

class COptionsPageSizeFormatting : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_SIZEFORMATTING"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

	void UpdateControls();
	void UpdateExamples();

	int GetFormat();

	DECLARE_EVENT_TABLE();
	void OnRadio(wxCommandEvent& event);
	void OnCheck(wxCommandEvent& event);
	void OnSpin(wxSpinEvent& event);

	wxString FormatSize(const wxLongLong& size);
};

#endif //__OPTIONSPAGE_SIZEFORMATTING_H__
