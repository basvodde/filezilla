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

	//GetSizer()->Fit(this);
	//GetSizer()->SetSizeHints(this);

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
	{
		wxString wrapped = label;
		WrapText(this, wrapped, maxLength);
		pText->SetLabel(wrapped);
	}

	return true;
}

wxString wxDialogEx::GetLabel(int id)
{
	wxStaticText* pText = wxDynamicCast(FindWindow(id), wxStaticText);
	if (!pText)
		return _T("");

	return pText->GetLabel();
}

int wxDialogEx::ShowModal()
{
	CenterOnParent();

#ifdef __WXMSW__
	::EndMenu();
#endif
	return wxDialog::ShowModal();
}
