#ifndef __DEFAULTFILEEXISTSDLG_H__
#define __DEFAULTFILEEXISTSDLG_H__

#include "dialogex.h"

class CDefaultFileExistsDlg : protected wxDialogEx
{
public:
	CDefaultFileExistsDlg();

	bool Load(wxWindow *parent);

	static int GetDefault(bool download);

	void Run();

protected:
	static int m_defaults[2];
};

#endif //__DEFAULTFILEEXISTSDLG_H__
