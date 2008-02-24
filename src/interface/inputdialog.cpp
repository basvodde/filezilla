#include "FileZilla.h"
#include "inputdialog.h"

BEGIN_EVENT_TABLE(CInputDialog, wxDialogEx)
EVT_TEXT(XRCID("ID_STRING"), CInputDialog::OnValueChanged)
EVT_TEXT(XRCID("ID_STRING_PW"), CInputDialog::OnValueChanged)
EVT_BUTTON(XRCID("wxID_OK"), CInputDialog::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CInputDialog::OnCancel)
END_EVENT_TABLE();

bool CInputDialog::Create(wxWindow* parent, const wxString& title, wxString text)
{
	m_allowEmpty = false;
	SetParent(parent);

	if (!wxXmlResource::Get()->LoadDialog(this, parent, _T("ID_INPUTDIALOG")))
		return false;

	SetTitle(title);

	if (!XRCCTRL(*this, "wxID_OK", wxButton))
		return false;

	if (!XRCCTRL(*this, "wxID_CANCEL", wxButton))
		return false;

	m_pTextCtrl = XRCCTRL(*this, "ID_STRING", wxTextCtrl);
	if (!m_pTextCtrl)
		return false;

	if (!XRCCTRL(*this, "ID_STRING_PW", wxTextCtrl))
		return false;

	wxStaticText* pText = XRCCTRL(*this, "ID_TEXT", wxStaticText);
	if (!pText)
		return false;

	WrapRecursive(this, 2.0);
	pText->SetLabel(text);

	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);

	XRCCTRL(*this, "ID_STRING", wxTextCtrl)->SetFocus();

	XRCCTRL(*this, "wxID_OK", wxButton)->Enable(false);

	return true;
}

void CInputDialog::AllowEmpty(bool allowEmpty)
{
	m_allowEmpty = allowEmpty;
	XRCCTRL(*this, "wxID_OK", wxButton)->Enable(m_allowEmpty ? true : (m_pTextCtrl->GetValue() != _T("")));
}

void CInputDialog::OnValueChanged(wxCommandEvent& event)
{
	wxString value = m_pTextCtrl->GetValue();
	XRCCTRL(*this, "wxID_OK", wxButton)->Enable(m_allowEmpty ? true : (value != _T("")));
}

void CInputDialog::SetValue(const wxString& value)
{
	m_pTextCtrl->SetValue(value);
}

wxString CInputDialog::GetValue() const
{
	return m_pTextCtrl->GetValue();
}

bool CInputDialog::SelectText(int start, int end)
{
#ifdef __WXGTK__
	Show();
#endif
	m_pTextCtrl->SetFocus();
	m_pTextCtrl->SetSelection(start, end);
	return true;
}

void CInputDialog::OnOK(wxCommandEvent& event)
{
	EndModal(wxID_OK);
}

void CInputDialog::OnCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

bool CInputDialog::SetPasswordMode(bool password)
{
	if (password)
	{
		m_pTextCtrl = XRCCTRL(*this, "ID_STRING_PW", wxTextCtrl);
		m_pTextCtrl->Show();
		XRCCTRL(*this, "ID_STRING", wxTextCtrl)->Hide();
	}
	else
	{
		m_pTextCtrl = XRCCTRL(*this, "ID_STRING", wxTextCtrl);
		m_pTextCtrl->Show();
		XRCCTRL(*this, "ID_STRING_PW", wxTextCtrl)->Hide();
	}
	return true;
}
