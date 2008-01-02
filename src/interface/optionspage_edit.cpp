#include "FileZilla.h"

#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_edit.h"

BEGIN_EVENT_TABLE(COptionsPageEdit, COptionsPage)
EVT_BUTTON(XRCID("ID_BROWSE"), COptionsPageEdit::OnBrowseEditor)
END_EVENT_TABLE()

bool COptionsPageEdit::LoadPage()
{
	bool failure = false;

	SetTextFromOption(XRCID("ID_EDITOR"), OPTION_EDIT_DEFAULTEDITOR, failure);
	SetTextFromOption(XRCID("ID_ASSOCIATIONS"), OPTION_EDIT_CUSTOMASSOCIATIONS, failure);

	COptions* pOptions = COptions::Get();
	SetCheck(XRCID("ID_INHERIT"), pOptions->GetOptionVal(OPTION_EDIT_INHERITASSOCIATIONS) != 0, failure);

	if (pOptions->GetOptionVal(OPTION_EDIT_ALWAYSDEFAULT))
		SetRCheck(XRCID("ID_USEDEFAULT"), true, failure);
	else
		SetRCheck(XRCID("ID_USEASSOCIATIONS"), true, failure);

	return !failure;
}

bool COptionsPageEdit::SavePage()
{
	SetOptionFromText(XRCID("ID_EDITOR"), OPTION_EDIT_DEFAULTEDITOR);
	SetOptionFromText(XRCID("ID_ASSOCIATIONS"), OPTION_EDIT_CUSTOMASSOCIATIONS);
	
	COptions* pOptions = COptions::Get();

	pOptions->SetOption(OPTION_EDIT_INHERITASSOCIATIONS, GetCheck(XRCID("ID_INHERIT")) ? 1 : 0);
	if (GetRCheck(XRCID("ID_USEDEFAULT")))
		pOptions->SetOption(OPTION_EDIT_ALWAYSDEFAULT, 1);
	else
		pOptions->SetOption(OPTION_EDIT_ALWAYSDEFAULT, 0);
		
	return true;
}

bool UnquoteCommand(wxString& command, wxString& arguments)
{
	arguments = _T("");

	if (command == _T(""))
		return true;

	bool inQuotes = false;
	wxString file;
	for (unsigned int i = 0; i < command.Len(); i++)
	{
		if (command[i] == '"')
		{
			if (!inQuotes)
				inQuotes = true;
			else if (command[i + 1] == '"')
			{
				file += '"';
				i++;
			}
			else
				inQuotes = false;
		}
		else if (command[i] == ' ' && !inQuotes)
		{
			arguments = command.Mid(i + 1);
			arguments.Trim(false);
			break;
		}
		else
			file += command[i];
	}
	if (inQuotes)
		return false;

	command = file;

	return true;
}

bool ProgramExists(const wxString& editor)
{
	if (wxFileName::FileExists(editor))
		return true;

#ifdef __WXMAC__
	if (editor.Right(4) == _T(".app") && wxFileName::DirExists(editor))
		return true;
#endif

	return false;
}

bool COptionsPageEdit::Validate()
{
	bool failure = false;

	wxString editor = GetText(XRCID("ID_EDITOR"));
	editor.Trim(true);
	editor.Trim(false);
	SetText(XRCID("EDITOR"), editor, failure);

	if (editor != _T(""))
	{
		wxString args;
		if (!UnquoteCommand(editor, args))
			return DisplayError(_T("ID_EDITOR"), _("Default editor not properly quoted."));
		
		if (editor == _T(""))
			return DisplayError(_T("ID_EDITOR"), _("Empty quoted string."));

		if (!ProgramExists(editor))
			return DisplayError(_T("ID_EDITOR"), _("Selected file does not exist."));
	}

	if (GetRCheck(XRCID("ID_USEDEFAULT")) && editor == _T(""))
		return DisplayError(_T("ID_EDITOR"), _("A default editor needs to be set."));

	wxString associations = GetText(XRCID("ID_ASSOCIATIONS")) + _T("\n");
	associations.Replace(_T("\r"), _T(""));
	int pos;
	while ((pos = associations.Find('\n')) != -1)
	{
		wxString assoc = associations.Left(pos);
		associations = associations.Mid(pos + 1);

		if (assoc == _T(""))
			continue;

		wxString command;
		if (!UnquoteCommand(assoc, command))
			return DisplayError(_T("ID_ASSOCIATIONS"), _("Improperly quoted association."));

		if (assoc == _T(""))
			return DisplayError(_T("ID_ASSOCIATIONS"), _("Empty file extension."));

		wxString args;
		if (!UnquoteCommand(command, args))
			return DisplayError(_T("ID_ASSOCIATIONS"), _("Improperly quoted association."));
		
		if (command == _T(""))
			return DisplayError(_T("ID_ASSOCIATIONS"), _("Empty command."));

		if (!ProgramExists(command))
		{
			wxString error = _("Associated program not found:");
			error += '\n';
			error += command;
			return DisplayError(_T("ID_ASSOCIATIONS"), error);
		}		
	}

	return true;
}

void COptionsPageEdit::OnBrowseEditor(wxCommandEvent& event)
{
	wxFileDialog dlg(this, _("Select default editor"), _T(""), _T(""),
#ifdef __WXMSW__
		_T("Executable file (*.exe)|*.exe"),
#elif __WXMAC__
		_T("Applications (*.app)|*.app"),
#else
		wxFileSelectorDefaultWildcardStr,
#endif
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (dlg.ShowModal() != wxID_OK)
		return;

	wxString editor = dlg.GetPath();
	if (editor == _T(""))
		return;

	if (!ProgramExists(editor))
	{
		XRCCTRL(*this, "ID_EDITOR", wxWindow)->SetFocus();
		wxMessageBox(_("Selected editor does not exist"), _("File not found"), wxICON_EXCLAMATION, this);
		return;
	}

	if (editor.Find(' ') != -1)
		editor = _T("\"") + editor + _T("\"");

	bool tmp;
	SetText(XRCID("ID_EDITOR"), editor, tmp);
}
