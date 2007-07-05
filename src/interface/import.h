#ifndef __IMPORT_H__
#define __IMPORT_H__

#include "dialogex.h"

class TiXmlElement;
class CImportDialog : public wxDialogEx
{
public:
	CImportDialog(wxWindow* parent);

	void Show();

protected:
	bool ImportLegacy(TiXmlElement* pSites);
	bool ImportLegacy(TiXmlElement* pSitesToImport, TiXmlElement* pExistingSites);

	wxWindow* const m_parent;
};

#endif //__IMPORT_H__
