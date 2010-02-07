#include <filezilla.h>

#include "../Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_edit_associations.h"

bool COptionsPageEditAssociations::LoadPage()
{
	bool failure = false;

	COptions* pOptions = COptions::Get();

	SetTextFromOption(XRCID("ID_ASSOCIATIONS"), OPTION_EDIT_CUSTOMASSOCIATIONS, failure);
	SetCheck(XRCID("ID_INHERIT"), pOptions->GetOptionVal(OPTION_EDIT_INHERITASSOCIATIONS) != 0, failure);

	return !failure;
}

bool COptionsPageEditAssociations::SavePage()
{
	COptions* pOptions = COptions::Get();

	SetOptionFromText(XRCID("ID_ASSOCIATIONS"), OPTION_EDIT_CUSTOMASSOCIATIONS);
	
	pOptions->SetOption(OPTION_EDIT_INHERITASSOCIATIONS, GetCheck(XRCID("ID_INHERIT")) ? 1 : 0);
		
	return true;
}

// optionspage_edit.cpp
extern bool UnquoteCommand(wxString& command, wxString& arguments);
extern bool ProgramExists(const wxString& editor);

bool COptionsPageEditAssociations::Validate()
{
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
