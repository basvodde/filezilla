#ifndef __OPTIONSPAGE_FILETYPE_H__
#define __OPTIONSPAGE_FILETYPE_H__

class COptionsPageFiletype : public COptionsPage
{
public:
	virtual wxString GetResourceName() { return _T("ID_SETTINGS_FILETYPE"); }
	virtual bool LoadPage();
	virtual bool SavePage();
	virtual bool Validate();

protected:
	void SetCtrlState();

	DECLARE_EVENT_TABLE();
	void OnRemove(wxCommandEvent& event);
	void OnAdd(wxCommandEvent& event);
	void OnSelChanged(wxListEvent& event);
	void OnTextChanged(wxCommandEvent& event);
};

#endif //__OPTIONSPAGE_FILETYPE_H__
