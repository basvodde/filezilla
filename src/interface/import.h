#ifndef __IMPORT_H__
#define __IMPORT_H__

#include "dialogex.h"

class CImportDialog : public wxDialogEx
{
public:
	CImportDialog(wxWindow* parent);

	void Show();

protected:
	wxWindow* const m_parent;
};

#endif //__IMPORT_H__
