#ifndef __OPTIONSPAGE_H__
#define __OPTIONSPAGE_H__

// Default title of the validation failed message box
extern wxString validationFailed;

class COptions;
class CSettingsDialog;
class COptionsPage : public wxPanel
{
public:
	bool CreatePage(COptions* pOptions, wxWindow* parent, wxSize& maxSize);

	virtual wxString GetResourceName() = 0;
	virtual bool LoadPage() = 0;
	virtual bool SavePage() = 0;
	virtual bool Validate() { return true; }

	void SetCheck(const char* id, bool checked, bool& failure);
	void SetRCheck(const char* id, bool checked, bool& failure);
	void SetText(const char* id, const wxString& text, bool& failure);

	// The GetXXX functions do never return an error since the controls were 
	// checked to exist while loading the dialog.
	bool GetCheck(const char* id);
	bool GetRCheck(const char* id);
	wxString GetText(const char* id);

protected:
	COptions* m_pOptions;
};

#endif //__OPTIONSPAGE_H__
