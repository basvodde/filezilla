#include "FileZilla.h"
#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_transfer.h"

bool COptionsPageTransfer::LoadPage()
{
	bool failure = false;
	SetTextFromOption(XRCID("ID_NUMTRANSFERS"), OPTION_NUMTRANSFERS, failure);
	SetTextFromOption(XRCID("ID_NUMDOWNLOADS"), OPTION_CONCURRENTDOWNLOADLIMIT, failure);
	SetTextFromOption(XRCID("ID_NUMUPLOADS"), OPTION_CONCURRENTUPLOADLIMIT, failure);
	SetTextFromOption(XRCID("ID_TIMEOUT"), OPTION_TIMEOUT, failure);
	SetTextFromOption(XRCID("ID_DOWNLOADLIMIT"), OPTION_SPEEDLIMIT_INBOUND, failure);
	SetTextFromOption(XRCID("ID_UPLOADLIMIT"), OPTION_SPEEDLIMIT_OUTBOUND, failure);
	
	wxChoice* pChoice = XRCCTRL(*this, "ID_BURSTTOLERANCE", wxChoice);
	if (!pChoice)
		return false;
	int selection = m_pOptions->GetOptionVal(OPTION_SPEEDLIMIT_BURSTTOLERANCE);
	pChoice->SetSelection(selection);
	return !failure;
}

bool COptionsPageTransfer::SavePage()
{
	SetOptionFromText(XRCID("ID_NUMTRANSFERS"), OPTION_NUMTRANSFERS);
	SetOptionFromText(XRCID("ID_NUMDOWNLOADS"), OPTION_CONCURRENTDOWNLOADLIMIT);
	SetOptionFromText(XRCID("ID_NUMUPLOADS"), OPTION_CONCURRENTUPLOADLIMIT);
	SetOptionFromText(XRCID("ID_TIMEOUT"), OPTION_TIMEOUT);
	SetOptionFromText(XRCID("ID_DOWNLOADLIMIT"), OPTION_SPEEDLIMIT_INBOUND);
	SetOptionFromText(XRCID("ID_UPLOADLIMIT"), OPTION_SPEEDLIMIT_OUTBOUND);

	wxChoice* pChoice = XRCCTRL(*this, "ID_BURSTTOLERANCE", wxChoice);
	m_pOptions->SetOption(OPTION_SPEEDLIMIT_BURSTTOLERANCE, pChoice->GetSelection());

	return true;
}

bool COptionsPageTransfer::Validate()
{
	long tmp;
	wxTextCtrl* pCtrl;

	pCtrl = XRCCTRL(*this, "ID_NUMTRANSFERS", wxTextCtrl);
	if (!pCtrl->GetValue().ToLong(&tmp) || tmp < 1 || tmp > 10)
	{
		pCtrl->SetFocus();
		wxMessageBox(_("Please enter a number between 1 and 10 for the number of concurrent transfers."), validationFailed, wxICON_EXCLAMATION, this);
		return false;
	}

	pCtrl = XRCCTRL(*this, "ID_NUMDOWNLOADS", wxTextCtrl);
	if (!pCtrl->GetValue().ToLong(&tmp) || tmp < 0 || tmp > 10)
	{
		pCtrl->SetFocus();
		wxMessageBox(_("Please enter a number between 0 and 10 for the number of concurrent downloads."), validationFailed, wxICON_EXCLAMATION, this);
		return false;
	}

	pCtrl = XRCCTRL(*this, "ID_NUMUPLOADS", wxTextCtrl);
	if (!pCtrl->GetValue().ToLong(&tmp) || tmp < 0 || tmp > 10)
	{
		pCtrl->SetFocus();
		wxMessageBox(_("Please enter a number between 0 and 10 for the number of concurrent uploads."), validationFailed, wxICON_EXCLAMATION, this);
		return false;
	}

	pCtrl = XRCCTRL(*this, "ID_TIMEOUT", wxTextCtrl);
	if (!pCtrl->GetValue().ToLong(&tmp) || ((tmp < 5 || tmp > 9999) && tmp != 0))
	{
		pCtrl->SetFocus();
		wxMessageBox(_("Please enter a timeout between 5 and 9999 seconds or 0 to disable timeouts."), validationFailed, wxICON_EXCLAMATION, this);
		return false;
	}

	pCtrl = XRCCTRL(*this, "ID_DOWNLOADLIMIT", wxTextCtrl);
	if (!pCtrl->GetValue().ToLong(&tmp) || (tmp < 0))
	{
		pCtrl->SetFocus();
		wxMessageBox(_("Please enter a download speedlimit greater or equal to 0 KB/s."), validationFailed, wxICON_EXCLAMATION, this);
		return false;
	}

	pCtrl = XRCCTRL(*this, "ID_UPLOADLIMIT", wxTextCtrl);
	if (!pCtrl->GetValue().ToLong(&tmp) || (tmp < 0))
	{
		pCtrl->SetFocus();
		wxMessageBox(_("Please enter an upload speedlimit greater or equal to 0 KB/s."), validationFailed, wxICON_EXCLAMATION, this);
		return false;
	}

	return true;
}
