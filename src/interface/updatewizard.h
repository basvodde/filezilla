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
	wxWindow* m_parent;

	std::vector<wxWizardPageSimple*> m_pages;

	DECLARE_EVENT_TABLE()
	void OnCheck(wxCommandEvent& event);
	void OnPageChanging(wxWizardEvent& event);
};

#endif //__UPDATEWIZARD_H__
