#ifndef __NETCONFWIZARD_H__
#define __NETCONFWIZARD_H__

#include <wx/wizard.h>

class COptions;
class CNetConfWizard : public wxWizard
{
public:
	CNetConfWizard(wxWindow* parent, COptions* pOptions);
	virtual ~CNetConfWizard();

	bool Load();
	bool Run();

protected:
	wxWindow* m_parent;
	COptions* m_pOptions;

	std::vector<wxWizardPageSimple*> m_pages;

	DECLARE_EVENT_TABLE();
	void OnPageChanging(wxWizardEvent& event);
	void OnPageChanged(wxWizardEvent& event);
};

#endif //__NETCONFWIZARD_H__
