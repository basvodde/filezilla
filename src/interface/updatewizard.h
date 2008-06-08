#ifndef __UPDATEWIZARD_H__
#define __UPDATEWIZARD_H__

#if FZ_MANUALUPDATECHECK

#include <wx/wizard.h>
#include "wrapengine.h"

class CUpdateWizard : public wxWizard, protected CWrapEngine
{
public:
	CUpdateWizard(wxWindow* pParent);
	virtual ~CUpdateWizard();

	bool Load();
	bool Run();

	void InitAutoUpdateCheck();
	void DisplayUpdateAvailability(bool showDialog, bool forceMenu = false);

protected:
	void FailedTransfer();
	void ParseData();

	void PrepareUpdateAvailablePage(const wxString &newVersion, wxString newUrl);

	void RewrapPage(int page);

	wxString m_data;

	wxWindow* m_parent;

	std::vector<wxWizardPageSimple*> m_pages;

	CFileZillaEngine* m_pEngine;

	void SetTransferStatus(const CTransferStatus* pStatus);

	void PrepareUpdateCheckPage();
	void StartUpdateCheck();

	int SendTransferCommand();

	wxString GetDownloadDir();

	DECLARE_EVENT_TABLE()
	void OnCheck(wxCommandEvent& event);
	void OnPageChanging(wxWizardEvent& event);
	void OnPageChanged(wxWizardEvent& event);
	void OnFinish(wxWizardEvent& event);
	void OnEngineEvent(wxEvent& event);
	void OnTimer(wxTimerEvent& event);

	bool m_inTransfer;
	bool m_skipPageChanging;

	int m_currentPage;

	wxString m_urlServer;
	wxString m_urlFile;
	wxString m_localFile;

	wxTimer m_statusTimer;

	bool m_loaded;

	// Hold changelog of what's new
	wxString m_news;

	// Auto check related functions and variables
	// ------------------------------------------

	bool CanAutoCheckForUpdateNow();

	wxTimer m_autoCheckTimer;

	bool m_autoUpdateCheckRunning;
	bool m_updateShown;
	bool m_start_check;
};

#endif //FZ_MANUALUPDATECHECK

#endif //__UPDATEWIZARD_H__
