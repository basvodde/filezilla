#include "FileZilla.h"
#include "inputdialog.h"

BEGIN_EVENT_TABLE(CInputDialog, wxDialog)
EVT_TEXT(XRCID("ID_STRING"), CInputDialog::OnValueChanged)
EVT_BUTTON(XRCID("wxID_OK"), CInputDialog::OnOK)
EVT_BUTTON(XRCID("wxID_CANCEL"), CInputDialog::OnCancel)
END_EVENT_TABLE();

wxString WrapText(const wxString &text, unsigned long
				  maxLength, wxWindow* pWindow)
{
	wxString wrappedText;

	unsigned long lastCut = 0, lastBlank = 0,
		lineLength = 0, saveLength = 0;
	int width, height;

	for (unsigned long i = 0; i < text.Length(); i++)
	{
		pWindow->GetTextExtent(text.c_str()[i], &width, &height);
		lineLength += width;

		if (text.c_str()[i] == ' ')
		{
			lastBlank = i;
			saveLength = lineLength - width;
		}
		else if (text.c_str()[i] == '\n')
		{
			wrappedText += text.SubString(lastCut, i);
			wrappedText.Trim();
			lineLength = 0;
			lastCut = i + 1;
		}

		if (lineLength > maxLength)
		{
			wrappedText += text.SubString(lastCut, lastBlank) + '\n';
			lineLength -= saveLength;
			lastCut = lastBlank + 1;
		}
	}

	if (text.Length() - 1 > lastCut)
	{
		wrappedText += text.SubString(lastCut, text.Length() - 1);
	}

	return wrappedText;

}

bool CInputDialog::Create(wxWindow* parent, const wxString& title, const wxString& text)
{
	m_allowEmpty = false;
	SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
	SetParent(parent);

	if (!wxXmlResource::Get()->LoadDialog(this, parent, _T("ID_INPUTDIALOG")))
		return false;

	SetTitle(title);

	if (!XRCCTRL(*this, "wxID_OK", wxButton))
		return false;

	if (!XRCCTRL(*this, "wxID_CANCEL", wxButton))
		return false;

	if (!XRCCTRL(*this, "ID_STRING", wxTextCtrl))
		return false;

	wxStaticText* pText = XRCCTRL(*this, "ID_TEXT", wxStaticText);
	if (!pText)
		return false;

	pText->SetLabel(WrapText(text, 250, pText));

	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);

	return true;
}

void CInputDialog::OnValueChanged(wxCommandEvent& event)
{
	wxString value = XRCCTRL(*this, "ID_STRING", wxTextCtrl)->GetValue();
	XRCCTRL(*this, "wxID_OK", wxButton)->Enable(m_allowEmpty ? true : (value != _T("")));
}

void CInputDialog::SetValue(const wxString& value)
{
	XRCCTRL(*this, "ID_STRING", wxTextCtrl)->SetValue(value);
}

wxString CInputDialog::GetValue() const
{
	return XRCCTRL(*this, "ID_STRING", wxTextCtrl)->GetValue();
}

bool CInputDialog::SelectText(int start, int end)
{
	Show();
	XRCCTRL(*this, "ID_STRING", wxTextCtrl)->SetSelection(start, end);
	return true;
}

void CInputDialog::OnOK(wxCommandEvent& event)
{
	EndDialog(wxID_OK);
}

void CInputDialog::OnCancel(wxCommandEvent& event)
{
	EndDialog(wxID_CANCEL);
}
