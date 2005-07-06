#include "FileZilla.h"
#include "dialogex.h"

BEGIN_EVENT_TABLE(wxDialogEx, wxDialog)
EVT_CHAR_HOOK(wxDialogEx::OnChar)
END_EVENT_TABLE()

void wxDialogEx::OnChar(wxKeyEvent& event)
{
	if (event.GetKeyCode() == WXK_ESCAPE)
	{
		wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL);
		ProcessEvent(event);
	}
	else
		event.Skip();
}

bool wxDialogEx::Load(wxWindow* pParent, const wxString& name)
{
	SetParent(pParent);
	if (!wxXmlResource::Get()->LoadDialog(this, pParent, name))
		return false;

	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);

	return true;
}

wxString wxDialogEx::WrapText(const wxString &text, unsigned long maxLength, wxWindow* pWindow)
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
			wrappedText += text.SubString(lastCut, lastBlank) + _T("\n");
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

bool wxDialogEx::WrapText(int id, unsigned long maxLength)
{
	wxStaticText* pText = wxDynamicCast(FindWindow(id), wxStaticText);
	if (!pText)
		return false;

	pText->SetLabel(WrapText(pText->GetLabel(), maxLength, this));

	return true;
}

bool wxDialogEx::SetLabel(int id, const wxString& label, unsigned long maxLength /*=0*/)
{
	wxStaticText* pText = wxDynamicCast(FindWindow(id), wxStaticText);
	if (!pText)
		return false;

	if (!maxLength)
		pText->SetLabel(label);
	else
		pText->SetLabel(WrapText(label, maxLength, this));

	return true;
}

wxString wxDialogEx::GetLabel(int id)
{
	wxStaticText* pText = wxDynamicCast(FindWindow(id), wxStaticText);
	if (!pText)
		return _T("");

	return pText->GetLabel();
}
