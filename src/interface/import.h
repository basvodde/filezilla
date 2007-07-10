#ifndef __IMPORT_H__
#define __IMPORT_H__

#include "dialogex.h"

class TiXmlElement;
class CQueueView;
class CImportDialog : public wxDialogEx
{
public:
	CImportDialog(wxWindow* parent, CQueueView* pQueueView);

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
	CQueueView* m_pQueueView;
};

#endif //__IMPORT_H__
