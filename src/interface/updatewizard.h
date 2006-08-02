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
	void FailedTransfer();
	void ParseData();

	wxString m_data;

	wxWindow* m_parent;

	std::vector<wxWizardPageSimple*> m_pages;

	CFileZillaEngine* m_pEngine;

	void SetTransferStatus(const CTransferStatus* pStatus);

	DECLARE_EVENT_TABLE()
	void OnCheck(wxCommandEvent& event);
	void OnPageChanging(wxWizardEvent& event);
	void OnPageChanged(wxWizardEvent& event);
	void OnEngineEvent(wxEvent& event);
	void OnTimer(wxTimerEvent& event);

	bool m_inTransfer;
	bool m_skipPageChanging;

	int m_currentPage;

	wxString m_urlServer;
	wxString m_urlFile;
	wxString m_localFile;

	wxTimer m_timer;
};

#endif //__UPDATEWIZARD_H__
