#ifndef __CONDITIONALDIALOG_H__
#define __CONDITIONALDIALOG_H__

class CConditionalDialog : public wxDialog
{
public:
	enum Modes
	{
		ok,
		yesno
	};

	enum DialogType
	{
		rawcommand_quote,
		viewhidden,
		confirmexit,
		sitemanager_confirmdelete,
		confirmexit_edit, // Confirm closing FileZilla while files are still being edited
		confirm_preserve_timestamps,
		compare_treeviewmismatch,
		compare_changesorting
	};

	CConditionalDialog(wxWindow* parent, enum DialogType type, enum Modes mode, bool checked = false);

	void AddText(const wxString &text);

	bool Run();

protected:
	enum DialogType m_type;

	wxSizer* m_pTextSizer;

	DECLARE_EVENT_TABLE();
	void OnButton(wxCommandEvent& event);
};

#endif //__CONDITIONALDIALOG_H__
