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

	// Import function for Site Manager
	bool HasEntryWithName(TiXmlElement* pElement, const wxString& name);
	TiXmlElement* GetFolderWithName(TiXmlElement* pElement, const wxString& name);
	bool ImportSites(TiXmlElement* pSites);
	bool ImportSites(TiXmlElement* pSitesToImport, TiXmlElement* pExistingSites);
	bool ImportLegacySites(TiXmlElement* pSites);
	bool ImportLegacySites(TiXmlElement* pSitesToImport, TiXmlElement* pExistingSites);
	wxString DecodeLegacyPassword(wxString pass);

	wxWindow* const m_parent;
};

#endif //__IMPORT_H__
