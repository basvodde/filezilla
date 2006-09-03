#ifndef __DEFAULTFILEEXISTSDLG_H__
#define __DEFAULTFILEEXISTSDLG_H__

#include "dialogex.h"

class CDefaultFileExistsDlg : protected wxDialogEx
{
public:
	CDefaultFileExistsDlg();

	bool Load(wxWindow *parent);

	static int GetDefault(bool download);
	static void SetDefault(bool download, int action);

	void Run();

protected:
	static int m_defaults[2];
};

#endif //__DEFAULTFILEEXISTSDLG_H__
