#ifndef __UPDATEWIZARD_H__
#define __UPDATEWIZARD_H__

#include <wx/wizard.h>
#include "wrapengine.h"

class CUpdateWizard : public wxWizard, protected CWrapEngine
{
public:
	CUpdateWizard(wxWindow* pParent);
	virtual ~CUpdateWizard();

	bool Load();
	bool Run();

protected:
	void FailedCheck();
	void ParseData();

	wxString m_data;

	wxWindow* m_parent;

	std::vector<wxWizardPageSimple*> m_pages;

	CFileZillaEngine* m_pEngine;

	DECLARE_EVENT_TABLE()
	void OnCheck(wxCommandEvent& event);
	void OnPageChanging(wxWizardEvent& event);
	void OnEngineEvent(wxEvent& event);

	bool m_inTransfer;
	bool m_skipPageChanging;

	int m_currentPage;
};

#endif //__UPDATEWIZARD_H__
