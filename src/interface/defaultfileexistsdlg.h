#ifndef __DEFAULTFILEEXISTSDLG_H__
#define __DEFAULTFILEEXISTSDLG_H__

#include "dialogex.h"

class CDefaultFileExistsDlg : protected wxDialogEx
{
public:
	CDefaultFileExistsDlg();

	bool Load(wxWindow *parent, bool fromQueue);

	static int GetDefault(bool download);
	static void SetDefault(bool download, int action);

	bool Run(int *downloadAction = 0, int *uploadAction = 0);

protected:
	static int m_defaults[2];
};

#endif //__DEFAULTFILEEXISTSDLG_H__
