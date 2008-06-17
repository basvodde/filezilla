#ifndef __DEFAULTFILEEXISTSDLG_H__
#define __DEFAULTFILEEXISTSDLG_H__

#include "dialogex.h"

class CDefaultFileExistsDlg : protected wxDialogEx
{
public:
	CDefaultFileExistsDlg();

	bool Load(wxWindow *parent, bool fromQueue);

	static enum CFileExistsNotification::OverwriteAction GetDefault(bool download);
	static void SetDefault(bool download, enum CFileExistsNotification::OverwriteAction action);

	bool Run(enum CFileExistsNotification::OverwriteAction *downloadAction = 0, enum CFileExistsNotification::OverwriteAction *uploadAction = 0);

protected:
	static enum CFileExistsNotification::OverwriteAction m_defaults[2];
};

#endif //__DEFAULTFILEEXISTSDLG_H__
