#ifndef __OPTIONSPAGE_H__
#define __OPTIONSPAGE_H__

#define SAFE_XRCCTRL(id, type) \
	if (!XRCCTRL(*this, id, type)) \
		return false; \
	XRCCTRL(*this, id, type)

// Default title of the validation failed message box
extern wxString validationFailed;

class COptions;
class CSettingsDialog;
class COptionsPage : public wxPanel
{
public:
	bool CreatePage(COptions* pOptions, CSettingsDialog* pOwner, wxWindow* parent, wxSize& maxSize);

	virtual wxString GetResourceName() = 0;
	virtual bool LoadPage() = 0;
	virtual bool SavePage() = 0;
	virtual bool Validate() { return true; }

	void SetCheck(int id, bool checked, bool& failure);
	void SetRCheck(int id, bool checked, bool& failure);
	void SetTextFromOption(int ctrlId, int optionId, bool& failure);
	void SetStaticText(int id, const wxString& text, bool& failure);
	void SetChoice(int id, int selection, bool& failure);

	// The GetXXX functions do never return an error since the controls were 
	// checked to exist while loading the dialog.
	bool GetCheck(int id);
	bool GetRCheck(int id);
	wxString GetText(int id);
	bool SetText(int id, const wxString& text, bool& failure);
	wxString GetStaticText(int id);
	int GetChoice(int id);

	void SetOptionFromText(int ctrlId, int optionId);

	void ReloadSettings();

protected:
	COptions* m_pOptions;
	CSettingsDialog* m_pOwner;
};

#endif //__OPTIONSPAGE_H__
