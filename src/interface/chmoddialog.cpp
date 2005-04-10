#include "FileZilla.h"
#include "chmoddialog.h"

BEGIN_EVENT_TABLE(CChmodDialog, wxDialog)
EVT_BUTTON(XRCID("wxID_OK"), CChmodDialog::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CChmodDialog::OnCancel)
EVT_TEXT(XRCID("ID_NUMERIC"), CChmodDialog::OnNumericChanged)
END_EVENT_TABLE();

extern wxString WrapText(const wxString &text, unsigned long
				  maxLength, wxWindow* pWindow);

bool CChmodDialog::Create(wxWindow* parent, int fileCount, int dirCount,
						  const wxString& name, const char permissions[9])
{
	m_noUserTextChange = false;
	lastChangedNumeric = false;

	memcpy(m_permissions, permissions, 9);

	SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
	SetParent(parent);

	wxString title;
	if (!dirCount)
	{
		if (fileCount == 1)
		{
			title = wxString::Format(_("Plase select the new attributes for the file \"%s\"."), name);
		}
		else
			title = _("Please select the new attributes for the selected files.");
	}
	else
	{
		if (!fileCount)
		{
			if (dirCount == 1)
			{
				title = wxString::Format(_("Plase select the new attributes for the directory \"%s\"."), name);
			}
			else
				title = _("Please select the new attributes for the selected directories.");
		}
		else
			title = _("Please select the new attributes for the selected files and directories.");
	}
	
	if (!wxXmlResource::Get()->LoadDialog(this, parent, _T("ID_CHMODDIALOG")))
		return false;

	if (!XRCCTRL(*this, "ID_DESC", wxStaticText))
		return false;

	XRCCTRL(*this, "ID_DESC", wxStaticText)->SetLabel(WrapText(title, 300, this));

	if (!XRCCTRL(*this, "wxID_OK", wxButton))
		return false;

	if (!XRCCTRL(*this, "wxID_CANCEL", wxButton))
		return false;

	if (!XRCCTRL(*this, "ID_NUMERIC", wxTextCtrl))
		return false;

	wxStaticText* pText = XRCCTRL(*this, "ID_NUMERICTEXT", wxStaticText);
	if (!pText)
		return false;

	wxString text = pText->GetLabel();
	pText->SetLabel(WrapText(text, 300, this));

	wxCheckBox* pRecurse = XRCCTRL(*this, "ID_RECURSE", wxCheckBox);
	if (!pRecurse)
		return false;

	if (!dirCount)
	{
		pRecurse->Hide();
	}

	const char* IDs[9] = { "ID_OWNERREAD", "ID_OWNERWRITE", "ID_OWNEREXECUTE",
						   "ID_GROUPREAD", "ID_GROUPWRITE", "ID_GROUPEXECUTE",
						   "ID_PUBLICREAD", "ID_PUBLICWRITE", "ID_PUBLICEXECUTE"
						 };

	for (int i = 0; i < 9; i++)
	{
		m_checkBoxes[i] = XRCCTRL(*this, IDs[i], wxCheckBox);
		
		if (!m_checkBoxes[i])
			return false;

		Connect(XRCID(IDs[i]), wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(CChmodDialog::OnCheckboxClick));

		switch (permissions[i])
		{
		default:
		case 0:
			m_checkBoxes[i]->Set3StateValue(wxCHK_UNDETERMINED);
			break;
		case 1:
			m_checkBoxes[i]->Set3StateValue(wxCHK_UNCHECKED);
			break;
		case 2:
			m_checkBoxes[i]->Set3StateValue(wxCHK_CHECKED);
			break;
		}
	}

	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);

	wxCommandEvent evt;
	OnCheckboxClick(evt);

	return true;
}

void CChmodDialog::OnOK(wxCommandEvent& event)
{
	wxCheckBox* pRecurse = XRCCTRL(*this, "ID_RECURSE", wxCheckBox);
	m_recursive = pRecurse->GetValue();	
	EndModal(wxID_OK);
}

void CChmodDialog::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

void CChmodDialog::OnCheckboxClick(wxCommandEvent& event)
{
	lastChangedNumeric = false;
	for (int i = 0; i < 9; i++)
	{
		wxCheckBoxState state = m_checkBoxes[i]->Get3StateValue();
		switch (state)
		{
		default:
		case wxCHK_UNDETERMINED:
			m_permissions[i] = 0;
			break;
		case wxCHK_UNCHECKED:
			m_permissions[i] = 1;
			break;
		case wxCHK_CHECKED:
			m_permissions[i] = 2;
			break;
		}
	}
	
	wxString numericValue;
	for (int i = 0; i < 3; i++)
	{
		if (!m_permissions[i * 3] || !m_permissions[i * 3 + 1] || !m_permissions[i * 3 + 2])
		{
			numericValue += 'x';
			continue;
		}

		numericValue += wxString::Format(_T("%d"), (m_permissions[i * 3] - 1) * 4 + (m_permissions[i * 3 + 1] - 1) * 2 + (m_permissions[i * 3 + 2] - 1) * 1);
	}

	wxTextCtrl *pTextCtrl = XRCCTRL(*this, "ID_NUMERIC", wxTextCtrl);
	wxString oldValue = pTextCtrl->GetValue();

	m_noUserTextChange = true;
	pTextCtrl->SetValue(oldValue.Left(oldValue.Length() - 3) + numericValue);
	m_noUserTextChange = false;
	oldNumeric = numericValue;
}

void CChmodDialog::OnNumericChanged(wxCommandEvent& event)
{
	if (m_noUserTextChange)
		return;

	lastChangedNumeric = true;
	
	wxTextCtrl *pTextCtrl = XRCCTRL(*this, "ID_NUMERIC", wxTextCtrl);
	wxString numeric = pTextCtrl->GetValue();
	if (numeric.Length() < 3)
		return;
	
	numeric = numeric.Right(3);
	for (int i = 0; i < 3; i++)
	{
		if ((numeric[i] < '0' || numeric[i] > '9') && numeric[i] != 'x')
			return;
	}
	for (int i = 0; i < 3; i++)
	{
		if (oldNumeric != _T("") && numeric[i] == oldNumeric[i])
			continue;
		if (numeric[i] == 'x')
		{
			m_permissions[i] = 0;
			m_permissions[i + 1] = 0;
			m_permissions[i + 2] = 0;
		}
		else
		{
			int value = numeric[i] - '0';
			m_permissions[i * 3] = (value & 4) ? 2 : 1;
			m_permissions[i * 3 + 1] = (value & 2) ? 2 : 1;
			m_permissions[i * 3 + 2] = (value & 1) ? 2 : 1;
		}
	}

	oldNumeric = numeric;

	for (int i = 0; i < 9; i++)
	{
		switch (m_permissions[i])
		{
		default:
		case 0:
			m_checkBoxes[i]->Set3StateValue(wxCHK_UNDETERMINED);
			break;
		case 1:
			m_checkBoxes[i]->Set3StateValue(wxCHK_UNCHECKED);
			break;
		case 2:
			m_checkBoxes[i]->Set3StateValue(wxCHK_CHECKED);
			break;
		}
	}
}

wxString CChmodDialog::GetPermissions(const char* previousPermissions)
{
	// Construct a new permission string

	wxTextCtrl *pTextCtrl = XRCCTRL(*this, "ID_NUMERIC", wxTextCtrl);
	wxString numeric = pTextCtrl->GetValue();
	if (numeric.Length() < 3)
		return numeric;

	numeric = numeric.Right(3);
	for (unsigned int i = numeric.Length() - 3; i < numeric.Length(); i++)
	{
		if ((numeric[i] < '0' || numeric[i] > '9') && numeric[i] != 'x')
			return numeric;
	}
	if (!previousPermissions)
	{
		// Use default of  (0...0)755
		if (numeric[numeric.Length() - 1] == 'x')
			numeric[numeric.Length() - 1] = 5;
		if (numeric[numeric.Length() - 2] == 'x')
			numeric[numeric.Length() - 2] = 5;
		if (numeric[numeric.Length() - 3] == 'x')
			numeric[numeric.Length() - 3] = 7;
		numeric.Replace(_T("x"), _T("0"));
		return numeric;
	}

	const char defaultPerms[9] = { 2, 2, 2, 1, 0, 1, 1, 0, 1 };
	char perms[9];
	memcpy(perms, m_permissions, 9);

	wxString permission = numeric.Left(numeric.Length() - 3);
	for (unsigned int i = numeric.Length() - 3; i < numeric.Length(); i++)
	{
		for (unsigned int j = i * 3; j < i * 3 + 3; j++)
		{
			if (!perms[j])
			{
				if (previousPermissions[j])
					perms[j] = previousPermissions[j];
				else
					perms[j] = defaultPerms[j];
			}
		}
		permission += wxString::Format(_T("%d"), (int)(perms[i * 3] - 1) * 4 + (perms[i * 3 + 1] - 1) * 2 + (perms[i * 3 + 2] - 1) * 1);
	}

	return permission;
}

bool CChmodDialog::Recursive()
{
	return m_recursive;
}