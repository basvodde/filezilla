#include <filezilla.h>
#include "defaultfileexistsdlg.h"

enum CFileExistsNotification::OverwriteAction CDefaultFileExistsDlg::m_defaults[2] = {CFileExistsNotification::unknown, CFileExistsNotification::unknown};

CDefaultFileExistsDlg::CDefaultFileExistsDlg()
{
}

bool CDefaultFileExistsDlg::Load(wxWindow *parent, bool fromQueue)
{
	if (!wxDialogEx::Load(parent, _T("ID_DEFAULTFILEEXISTSDLG")))
		return false;

	if (fromQueue)
		XRCCTRL(*this, "ID_DESCRIPTION", wxStaticText)->SetLabel(_("Select default file exists action only for the currently selected files in the queue."));
	else
		XRCCTRL(*this, "ID_DESCRIPTION", wxStaticText)->SetLabel(_("Select default file exists action if the target file already exists. This selection is valid only for the current session."));

	WrapRecursive(this, 1.8, "DEFAULTFILEEXISTS");

	if (fromQueue)
		return true;

	SelectDefaults(&m_defaults[0], &m_defaults[1]);

	return true;
}

void CDefaultFileExistsDlg::SelectDefaults(enum CFileExistsNotification::OverwriteAction* downloadAction, enum CFileExistsNotification::OverwriteAction* uploadAction)
{
	if (downloadAction)
		XRCCTRL(*this, "ID_DOWNLOAD_ACTION", wxChoice)->SetSelection(*downloadAction + 1);
	if (uploadAction)
		XRCCTRL(*this, "ID_UPLOAD_ACTION", wxChoice)->SetSelection(*uploadAction + 1);
}

enum CFileExistsNotification::OverwriteAction CDefaultFileExistsDlg::GetDefault(bool download)
{
	return m_defaults[download ? 0 : 1];
}

bool CDefaultFileExistsDlg::Run(enum CFileExistsNotification::OverwriteAction *downloadAction, enum CFileExistsNotification::OverwriteAction *uploadAction)
{
	SelectDefaults(downloadAction, uploadAction);

	// Remove one side of the dialog if not needed
	if (!downloadAction && uploadAction)
	{
		XRCCTRL(*this, "ID_DOWNLOAD_ACTION_DESC", wxWindow)->Hide();
		XRCCTRL(*this, "ID_DOWNLOAD_ACTION", wxWindow)->Hide();
	}
	else if (downloadAction && !uploadAction)
	{
		XRCCTRL(*this, "ID_UPLOAD_ACTION_DESC", wxStaticText)->Hide();
		XRCCTRL(*this, "ID_UPLOAD_ACTION", wxWindow)->Hide();
	}
	Layout();
	GetSizer()->Fit(this);

	if (ShowModal() != wxID_OK)
		return false;

	if (downloadAction || !uploadAction)
	{
		int dl = XRCCTRL(*this, "ID_DOWNLOAD_ACTION", wxChoice)->GetSelection();
		if (dl >= 0)
			dl--;
		enum CFileExistsNotification::OverwriteAction action = (enum CFileExistsNotification::OverwriteAction)dl;

		if (downloadAction)
			*downloadAction = action;
		else
			m_defaults[0] = action;
	}

	if (!downloadAction || uploadAction)
	{
		int ul = XRCCTRL(*this, "ID_UPLOAD_ACTION", wxChoice)->GetSelection();
		if (ul >= 0)
			ul--;
		enum CFileExistsNotification::OverwriteAction action = (enum CFileExistsNotification::OverwriteAction)ul;

		if (uploadAction)
			*uploadAction = action;
		else
			m_defaults[1] = action;
	}

	return true;
}

void CDefaultFileExistsDlg::SetDefault(bool download, enum CFileExistsNotification::OverwriteAction action)
{
	m_defaults[download ? 0 : 1] = action;
}
