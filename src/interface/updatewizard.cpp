#include "FileZilla.h"

#if FZ_MANUALUPDATECHECK

#include "updatewizard.h"
#include "filezillaapp.h"
#include "Options.h"
#include "buildinfo.h"
#include <wx/stdpaths.h>
#include "Mainfrm.h"

#define MAXCHECKPROGRESS 9 // Maximum value of progress bar

BEGIN_EVENT_TABLE(CUpdateWizard, wxWizard)
EVT_CHECKBOX(wxID_ANY, CUpdateWizard::OnCheck)
EVT_WIZARD_PAGE_CHANGING(wxID_ANY, CUpdateWizard::OnPageChanging)
EVT_WIZARD_PAGE_CHANGED(wxID_ANY, CUpdateWizard::OnPageChanged)
EVT_FZ_NOTIFICATION(wxID_ANY, CUpdateWizard::OnEngineEvent)
EVT_TIMER(wxID_ANY, CUpdateWizard::OnTimer)
EVT_WIZARD_FINISHED(wxID_ANY, CUpdateWizard::OnFinish)
END_EVENT_TABLE()

CUpdateWizard::CUpdateWizard(wxWindow* pParent)
	: m_parent(pParent)
{
	m_pEngine = new CFileZillaEngine;

	m_pEngine->Init(this, COptions::Get());

	m_inTransfer = false;
	m_skipPageChanging = false;
	m_currentPage = 0;

	wxString host = CBuildInfo::GetHostname();
	if (host == _T(""))
		host = _T("unknown");

	wxString version(PACKAGE_VERSION, wxConvLocal);
	version.Replace(_T(" "), _T("%20"));
	m_urlServer = _T("update.filezilla-project.org");
	m_urlFile = wxString::Format(_T("/updatecheck.php?platform=%s&version=%s"), host.c_str(), version.c_str());

	m_statusTimer.SetOwner(this, 0);
	m_autoCheckTimer.SetOwner(this, 1);

	m_autoUpdateCheckRunning = false;

	m_loaded = false;
	m_updateShown = false;
}

CUpdateWizard::~CUpdateWizard()
{
	delete m_pEngine;
}

bool CUpdateWizard::Load()
{
	if (!Create(m_parent, wxID_ANY, _("Check for updates")))
		return false;

	wxSize minPageSize = GetPageAreaSizer()->GetMinSize();

	for (int i = 1; i <= 5; i++)
	{
		wxWizardPageSimple* page = new wxWizardPageSimple();
		bool res = wxXmlResource::Get()->LoadPanel(page, this, wxString::Format(_T("ID_CHECKFORUPDATE%d"), i));
		if (!res)
		{
			delete page;
			return false;
		}
		page->Show(false);

		m_pages.push_back(page);
	}
	for (unsigned int i = 0; i < (m_pages.size() - 1); i++)
		m_pages[i]->Chain(m_pages[i], m_pages[i + 1]);


	GetPageAreaSizer()->Add(m_pages[0]);
	
	std::vector<wxWindow*> windows;
	for (unsigned int i = 0; i < m_pages.size(); i++)
		windows.push_back(m_pages[i]);

	wxGetApp().GetWrapEngine()->WrapRecursive(windows, 1.7, "UpdateCheck", wxSize(), minPageSize);

	XRCCTRL(*this, "ID_CHECKINGTEXT", wxStaticText)->Hide();
	XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge)->Hide();
	XRCCTRL(*this, "ID_CHECKINGTEXTPROGRESS", wxStaticText)->Hide();

	XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge)->SetRange(MAXCHECKPROGRESS);

	GetPageAreaSizer()->Add(m_pages[1]);
	m_pages[1]->SetPrev(0);
	for (int i = 2; i <= 4; i++)
	{
		m_pages[i]->SetPrev(0);
		m_pages[i]->SetNext(0);
		GetPageAreaSizer()->Add(m_pages[i]);
	}

	m_loaded = true;

	return true;
}

bool CUpdateWizard::Run()
{
	COptions* pOptions = COptions::Get();

	if (CBuildInfo::GetVersion() == _T("custom build"))
		return false;

	const wxString& newVersion = pOptions->GetOption(OPTION_UPDATECHECK_NEWVERSION);
	if (newVersion == _T(""))
		return RunWizard(m_pages.front());

	if (CBuildInfo::ConvertToVersionNumber(newVersion) <= CBuildInfo::ConvertToVersionNumber(CBuildInfo::GetVersion()))
	{
		pOptions->SetOption(OPTION_UPDATECHECK_NEWVERSION, _T(""));
		pOptions->SetOption(OPTION_UPDATECHECK_URL, _T(""));
		return RunWizard(m_pages.front());
	}

	PrepareUpdateAvailablePage(newVersion, pOptions->GetOption(OPTION_UPDATECHECK_URL));

	m_currentPage = 1;
	return RunWizard(m_pages[1]);
}

void CUpdateWizard::OnCheck(wxCommandEvent& event)
{
	if (event.GetId() == XRCID("ID_CHECKBETA") && event.IsChecked())
	{
		if (wxMessageBox(_("Do you really want to check for beta versions?\nUnless you want to test new features, keep using stable versions."), _("Update wizard"), wxICON_QUESTION | wxYES_NO, this) != wxYES)
		{
			XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->SetValue(0);
			return;
		}
	}
	else if (event.GetId() == XRCID("ID_CHECKNIGHTLY") && event.IsChecked())
	{
		if (wxMessageBox(_("Warning, use nightly builds at your own risk.\nNo support is given for nightly builds.\nNightly builds may not work as expected and might even damage your system.\n\nDo you really want to check for nightly builds?"), _("Update wizard"), wxICON_EXCLAMATION | wxYES_NO, this) != wxYES)
		{
			XRCCTRL(*this, "ID_CHECKNIGHTLY", wxCheckBox)->SetValue(0);
			return;
		}
	}
}

void CUpdateWizard::OnPageChanging(wxWizardEvent& event)
{
	if (m_skipPageChanging)
		return;
	m_skipPageChanging = true;

	if (event.GetPage() == m_pages[0])
	{
		XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->Disable();
		XRCCTRL(*this, "ID_CHECKNIGHTLY", wxCheckBox)->Disable();
		wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
		pNext->Disable();

		XRCCTRL(*this, "ID_CHECKINGTEXT", wxStaticText)->Show();
		wxGauge* pProgress = XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge);
		pProgress->Show();

		wxStaticText *pText = XRCCTRL(*this, "ID_CHECKINGTEXTPROGRESS", wxStaticText);
		pText->Show();
		pText->SetLabel(_("Resolving hostname"));


		event.Veto();

		StartUpdateCheck();
	}
	if (event.GetPage() == m_pages[1] && m_pages[1]->GetNext())
	{
		wxString filename = m_urlFile;
		int pos = filename.Find('/', true);
		if (pos != -1)
			filename = filename.Mid(pos + 1);

		const wxString defaultDir = wxStandardPaths::Get().GetDocumentsDir();

		const int flags = wxFD_SAVE | wxFD_OVERWRITE_PROMPT;

		const wxString& ext = filename.Right(4);
		wxString type;
		if (ext == _T(".exe"))
			type = _("Executable");
		if (ext == _T(".bz2"))
			type = _("Archive");
		else
			type = _("Package");

		wxString filter = wxString::Format(_T("%s (*%s)|*%s"), type.c_str(), ext.c_str(), ext.c_str());

		wxFileDialog dialog(this, _("Select download location for package"), defaultDir, filename, filter, flags);
		if (dialog.ShowModal() != wxID_OK)
		{
			event.Veto();
			m_skipPageChanging = false;
			return;
		}
		m_localFile = dialog.GetPath();
	}
	
	m_skipPageChanging = false;
}

void CUpdateWizard::OnPageChanged(wxWizardEvent& event)
{
	if (event.GetPage() != m_pages[2])
		return;

	wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
	pNext->Disable();

	m_currentPage = 2;

	XRCCTRL(*this, "ID_DOWNLOADTEXT", wxStaticText)->SetLabel(wxString::Format(_("Downloading %s"), wxString(_T("http://") + m_urlServer + m_urlFile).c_str()));
	
	m_inTransfer = false;

	int res = m_pEngine->Command(CConnectCommand(CServer(HTTP, DEFAULT, m_urlServer, 80)));
	if (res == FZ_REPLY_OK)
	{
		m_inTransfer = true;
		XRCCTRL(*this, "ID_DOWNLOADPROGRESSTEXT", wxStaticText)->SetLabel(_("Connecting to server"));
		CFileTransferCommand::t_transferSettings transferSettings;

		CServerPath path;
		wxString file = m_urlFile;
		path.SetPath(file, true);

		CFileTransferCommand cmd(m_localFile, path, file, true, transferSettings);
		res = m_pEngine->Command(cmd);

		XRCCTRL(*this, "ID_DOWNLOADPROGRESS", wxGauge)->SetRange(100);
	}
	if (res == FZ_REPLY_OK)
		ShowPage(m_pages[1]);
	else if (res != FZ_REPLY_WOULDBLOCK)
		FailedTransfer();
	else
	{
		RewrapPage(2);
	}
}

void CUpdateWizard::FailedTransfer()
{
	m_inTransfer = false;

	if (!m_loaded)
		return;

	if (!m_currentPage)
		XRCCTRL(*this, "ID_FAILURE", wxStaticText)->SetLabel(_("Failed to check for newer version of FileZilla."));
	else
		XRCCTRL(*this, "ID_FAILURE", wxStaticText)->SetLabel(_("Failed to download the latest version of FileZilla."));
	
	((wxWizardPageSimple*)GetCurrentPage())->SetNext(m_pages[3]);
	m_pages[3]->SetPrev((wxWizardPageSimple*)GetCurrentPage());	

	m_skipPageChanging = true;
	ShowPage(m_pages[3]);
	m_currentPage = 3;
	m_skipPageChanging = false;

	wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
	pNext->Enable();
	wxButton* pPrev = wxDynamicCast(FindWindow(wxID_BACKWARD), wxButton);
	pPrev->Disable();
}

void CUpdateWizard::OnEngineEvent(wxEvent& event)
{
	if (!m_pEngine)
		return;

	if (m_currentPage >= 3)
	{
		CNotification *pNotification = m_pEngine->GetNextNotification();
		while (pNotification)
		{
			delete pNotification;
			pNotification = m_pEngine->GetNextNotification();
		}
		return;
	}

	CNotification *pNotification = m_pEngine->GetNextNotification();
	while (pNotification)
	{
		switch (pNotification->GetID())
		{
		case nId_logmsg:
			{
				if (!m_loaded)
					break;

				CLogmsgNotification* pLogMsg = reinterpret_cast<CLogmsgNotification *>(pNotification);
				if (pLogMsg->msgType == Status || pLogMsg->msgType == Command)
				{
					if (!m_currentPage)
					{
						wxStaticText *pText = XRCCTRL(*this, "ID_CHECKINGTEXTPROGRESS", wxStaticText);
						pText->SetLabel(pLogMsg->msg);
						m_pages[0]->GetSizer()->Layout();
						wxGauge* pProgress = XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge);
						int value = pProgress->GetValue();
#ifdef __WXDEBUG__
						wxASSERT(value < MAXCHECKPROGRESS);
#endif
						if (value < MAXCHECKPROGRESS)
							pProgress->SetValue(value + 1);
					}
					else if (m_currentPage == 2)
					{
						XRCCTRL(*this, "ID_DOWNLOADPROGRESSTEXT", wxStaticText)->SetLabel(pLogMsg->msg);
						m_pages[2]->GetSizer()->Layout();
					}
				}
			}
			break;
		case nId_operation:
			{
				COperationNotification* pOpMsg = reinterpret_cast<COperationNotification*>(pNotification);
				if (pOpMsg->nReplyCode != FZ_REPLY_OK)
				{
					while (pNotification)
					{
						delete pNotification;
						pNotification = m_pEngine->GetNextNotification();
					}
					FailedTransfer();
					return;
				}
				if (!m_inTransfer)
				{
					m_inTransfer = true;

					if (m_loaded && !m_currentPage)
					{
						wxGauge* pProgress = XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge);
						pProgress->SetValue(pProgress->GetValue() + 1);
					}

					CFileTransferCommand::t_transferSettings transferSettings;

					CServerPath path;
					wxString file = m_urlFile;
					path.SetPath(file, true);

					CFileTransferCommand cmd(m_localFile, path, file, true, transferSettings);
					int res = m_pEngine->Command(cmd);
					if (res == FZ_REPLY_WOULDBLOCK)
						break;
					else if (res != FZ_REPLY_OK)
					{
						FailedTransfer();
						break;
					}
				}

				if (!m_loaded || !m_currentPage)
				{
					m_pEngine->Command(CDisconnectCommand());
					ParseData();
				}
				else if (m_currentPage == 2)
				{
					wxStaticText* pText = XRCCTRL(*this, "ID_DOWNLOADCOMPLETE", wxStaticText);
					wxASSERT(pText);

					wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
					pNext->Enable();

					XRCCTRL(*this, "ID_DOWNLOADPROGRESS", wxGauge)->SetValue(100);
#ifdef __WXMSW__
					pText->SetLabel(_("The most recent version has been downloaded. Click on Finish to close FileZilla and to start the installation."));
#else
					pText->SetLabel(_("The most recent version has been downloaded. Please install like you did install this version."));

#endif
					pText->Show();
					RewrapPage(2);
				}
			}
			break;
		case nId_data:
			{
				wxASSERT(!m_currentPage);
				CDataNotification* pOpMsg = reinterpret_cast<CDataNotification*>(pNotification);
				int len;
				char* data = pOpMsg->Detach(len);

				if (m_data.Len() + len > 4096)
				{
					delete [] data;
					m_pEngine->Command(CCancelCommand());
					FailedTransfer();
					break;
				}
				for (int i = 0; i < len; i++)
				{
					if (!data[i] || (unsigned char)data[i] > 127)
					{
						delete [] data;
						m_pEngine->Command(CCancelCommand());
						FailedTransfer();
						break;
					}
				}

				m_data += wxString(data, wxConvUTF8, len);
				delete [] data;
				break;
			}
		case nId_asyncrequest:
			{
				CAsyncRequestNotification* pData = reinterpret_cast<CAsyncRequestNotification *>(pNotification);
				if (pData->GetRequestID() == reqId_fileexists)
				{
					reinterpret_cast<CFileExistsNotification *>(pData)->overwriteAction = CFileExistsNotification::overwrite;
				}
				m_pEngine->SetAsyncRequestReply(pData);
			}
			break;
		case nId_transferstatus:
			if (!m_loaded)
				break;

			if (m_currentPage == 2)
			{
				CTransferStatusNotification *pTransferStatusNotification = reinterpret_cast<CTransferStatusNotification *>(pNotification);
				const CTransferStatus *pStatus = pTransferStatusNotification->GetStatus();
				SetTransferStatus(pStatus);
			}
			break;
		default:
			break;
		}
		delete pNotification;
		pNotification = m_pEngine->GetNextNotification();
	}
}

void CUpdateWizard::ParseData()
{
	const wxULongLong ownVersionNumber = CBuildInfo::ConvertToVersionNumber(CBuildInfo::GetVersion());

	wxString newVersion;
	wxULongLong newVersionNumber = 0;
	wxString newUrl;

	while (m_data != _T(""))
	{
		wxString line;
		int pos = m_data.Find('\n');
		if (pos != -1)
		{
			line = m_data.Left(pos);
			m_data = m_data.Mid(pos + 1);
		}
		else
		{
			line = m_data;
			m_data = _T("");
		}

		line.Trim(true);
		line.Trim(false);
		
		// Extract type of update
		pos = line.Find(' ');
		if (pos < 1)
			continue;

		wxString type = line.Left(pos);
		line = line.Mid(pos + 1);

		// Extract version/date
		pos = line.Find(' ');
		if (pos < 1)
			continue;

		wxString versionOrDate = line.Left(pos);
		line = line.Mid(pos + 1);

		// Extract URL
		pos = line.Find(' ');
		if (pos != -1)
			continue;
		wxString url = line;
		if (url == _T("none"))
			url = _T("");

		if (type == _T("nightly"))
		{
			if (!m_loaded)
				continue;

			if (!XRCCTRL(*this, "ID_CHECKNIGHTLY", wxCheckBox)->GetValue())
				continue;

			wxDateTime nightlyDate;
			if (!nightlyDate.ParseDate(versionOrDate))
				continue;

			wxDateTime buildDate = CBuildInfo::GetBuildDate();
			if (!buildDate.IsValid() || !nightlyDate.IsValid() || nightlyDate <= buildDate)
				continue;

			if (url == _T(""))
				continue;

			newVersion = versionOrDate + _T(" Nightly");
			newUrl = url;
			break;
		}
		else
		{
			if (CBuildInfo::ConvertToVersionNumber(versionOrDate) <= ownVersionNumber)
				continue;
		}
		if (type == _T("beta"))
		{
			// Update beta only if at least one of these conditions is true:
			// - Beta box is checked
			// - Current version is a beta and no newer stable version exists

			const bool checkBeta = m_loaded ? XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->GetValue() : false;

			if (!checkBeta &&
				((ownVersionNumber & 0x0FFFFF) == 0 || newVersionNumber != 0))
				continue;

			if (newVersionNumber >= CBuildInfo::ConvertToVersionNumber(versionOrDate))
				continue;
		}
		else
		{
			// Final releases
			if (m_loaded)
			{
				if (CBuildInfo::ConvertToVersionNumber(versionOrDate) < newVersionNumber && XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->GetValue())
					continue;
			}
		}

		newVersion = versionOrDate;
		newVersionNumber = CBuildInfo::ConvertToVersionNumber(versionOrDate);
		newUrl = url;
	}

	if (!m_loaded)
	{
		if (newVersion == _T(""))
			return;

		COptions* pOptions = COptions::Get();
		pOptions->SetOption(OPTION_UPDATECHECK_NEWVERSION, newVersion);
		pOptions->SetOption(OPTION_UPDATECHECK_URL, newUrl);

		DisplayUpdateAvailability(true);

		return;
	}

	if (newVersion == _T(""))
	{
		m_skipPageChanging = true;
		ShowPage(m_pages[4]);
		m_currentPage = 4;
		m_skipPageChanging = false;
	}
	else
	{
		PrepareUpdateAvailablePage(newVersion, newUrl);

		m_skipPageChanging = true;
		ShowPage(m_pages[1]);
		m_currentPage = 1;
		m_skipPageChanging = false;
	}

	wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
	pNext->Enable();
	wxButton* pPrev = wxDynamicCast(FindWindow(wxID_BACKWARD), wxButton);
	pPrev->Disable();
}

void CUpdateWizard::OnTimer(wxTimerEvent& event)
{
	if (!event.GetId())
	{
		bool changed;
		CTransferStatus status;
		if (!m_pEngine)
			m_statusTimer.Stop();
		else if (!m_pEngine->GetTransferStatus(status, changed))
			SetTransferStatus(0);
		else if (changed)
			SetTransferStatus(&status);
		else
			m_statusTimer.Stop();
	}
	else if (event.GetId() == m_autoCheckTimer.GetId())
	{
		if (m_autoUpdateCheckRunning)
			return;

		if (CanAutoCheckForUpdateNow())
		{
			m_autoUpdateCheckRunning = true;
			const wxDateTime& now = wxDateTime::Now();
			COptions::Get()->SetOption(OPTION_UPDATECHECK_LASTDATE, now.Format(_T("%Y-%m-%d %H:%M:%S")));
			StartUpdateCheck();
		}
	}
}

void CUpdateWizard::SetTransferStatus(const CTransferStatus* pStatus)
{
	if (!pStatus)
	{
		if (m_statusTimer.IsRunning())
			m_statusTimer.Stop();
		return;
	}

	
	wxTimeSpan elapsed = wxDateTime::Now().Subtract(pStatus->started);
	int elapsedSeconds = elapsed.GetSeconds().GetLo(); // Assume GetHi is always 0

	wxString text;
	if (elapsedSeconds)
	{
		wxFileOffset rate = (pStatus->currentOffset - pStatus->startOffset) / elapsedSeconds;

        if (rate > (1000*1000))
			text.Printf(_("%s bytes (%d.%d MB/s)"), wxLongLong(pStatus->currentOffset).ToString().c_str(), (int)(rate / 1000 / 1000), (int)((rate / 1000 / 100) % 10));
		else if (rate > 1000)
			text.Printf(_("%s bytes (%d.%d KB/s)"), wxLongLong(pStatus->currentOffset).ToString().c_str(), (int)(rate / 1000), (int)((rate / 100) % 10));
		else
			text.Printf(_("%s bytes (%d B/s)"), wxLongLong(pStatus->currentOffset).ToString().c_str(), (int)rate);

		if (pStatus->totalSize > 0 && rate > 0)
		{
			int left = ((pStatus->totalSize - pStatus->startOffset) / rate) - elapsedSeconds;
			if (left < 0)
				left = 0;
			wxTimeSpan timeLeft(0, 0, left);
			text += timeLeft.Format(_(", %H:%M:%S left"));
		}
		else
		{
			text += _(", --:--:-- left");
		}
	}
	else
		text.Format(_("%s bytes"), wxLongLong(pStatus->currentOffset).ToString().c_str());

	XRCCTRL(*this, "ID_DOWNLOADPROGRESSTEXT", wxStaticText)->SetLabel(text);
	m_pages[2]->GetSizer()->Layout();

	if (pStatus->totalSize > 0)
	{	
		int percent = (pStatus->currentOffset * 100) / pStatus->totalSize;
		if (percent > 100)
			percent = 100;
		XRCCTRL(*this, "ID_DOWNLOADPROGRESS", wxGauge)->SetValue(percent);
	}

	if (!m_statusTimer.IsRunning())
		m_statusTimer.Start(100);
}

void CUpdateWizard::OnFinish(wxWizardEvent& event)
{
#ifdef __WXMSW__
	if (m_currentPage == 2)
	{
		wxExecute(m_localFile);
		CMainFrame* pFrame = (CMainFrame*)m_parent;
		pFrame->Close();
	}
#endif
}

void CUpdateWizard::InitAutoUpdateCheck()
{
	COptions* pOptions = COptions::Get();
	wxASSERT(pOptions->GetOptionVal(OPTION_UPDATECHECK));

	if (CBuildInfo::GetVersion() == _T("custom build"))
		return;

	// Check every hour if allowed to check for updates

	m_autoCheckTimer.Start(1000 * 3600);
	if (!CanAutoCheckForUpdateNow())
	{
		DisplayUpdateAvailability(false);
		return;
	}
	else
	{
		m_autoUpdateCheckRunning = true;
		const wxDateTime& now = wxDateTime::Now();
		pOptions->SetOption(OPTION_UPDATECHECK_LASTDATE, now.Format(_T("%Y-%m-%d %H:%M:%S")));
		StartUpdateCheck();
	}
}

bool CUpdateWizard::CanAutoCheckForUpdateNow()
{
	const wxString &lastCheckStr = COptions::Get()->GetOption(OPTION_UPDATECHECK_LASTDATE);
	if (lastCheckStr == _T(""))
		return true;

	wxDateTime lastCheck;	
	lastCheck.ParseDateTime(lastCheckStr);
	if (!lastCheck.IsValid())
		return true;

	wxTimeSpan span = wxDateTime::Now() - lastCheck;

	if (span.GetSeconds() < 0)
		// Last check in future
		return true;
	
	const int days = COptions::Get()->GetOptionVal(OPTION_UPDATECHECK_INTERVAL) * 3600 * 24;
	if (span.GetSeconds() >= days)
		return true;

	return false;
}

void CUpdateWizard::StartUpdateCheck()
{
	m_inTransfer = false;
	int res = m_pEngine->Command(CConnectCommand(CServer(HTTP, DEFAULT, m_urlServer, 80)));
	if (res == FZ_REPLY_OK)
	{
		m_inTransfer = true;
		if (m_loaded)
		{
			XRCCTRL(*this, "ID_CHECKINGTEXTPROGRESS", wxStaticText)->SetLabel(_("Connecting to server"));
			wxGauge* pProgress = XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge);
			pProgress->SetValue(pProgress->GetValue() + 1);
		}
		CFileTransferCommand::t_transferSettings transferSettings;

		CServerPath path;
		wxString file = m_urlFile;
		path.SetPath(file, true);

		CFileTransferCommand cmd(_T(""), path, file, true, transferSettings);
		res = m_pEngine->Command(cmd);
	}
	wxASSERT(res != FZ_REPLY_OK);
	if (res != FZ_REPLY_WOULDBLOCK)
		FailedTransfer();
}

void CUpdateWizard::DisplayUpdateAvailability(bool showDialog, bool forceMenu /*=false*/)
{
	if (m_updateShown && !forceMenu)
		return;

	COptions* pOptions = COptions::Get();

	if (CBuildInfo::GetVersion() == _T("custom build"))
		return;

	const wxString& newVersion = pOptions->GetOption(OPTION_UPDATECHECK_NEWVERSION);
	if (newVersion == _T(""))
		return;

	if (CBuildInfo::ConvertToVersionNumber(newVersion) <= CBuildInfo::ConvertToVersionNumber(CBuildInfo::GetVersion()))
	{
		pOptions->SetOption(OPTION_UPDATECHECK_NEWVERSION, _T(""));
		pOptions->SetOption(OPTION_UPDATECHECK_URL, _T(""));
		return;
	}

	m_updateShown = true;

	CMainFrame* pFrame = (CMainFrame*)m_parent;
		
	wxMenu* pMenu = new wxMenu();
	const wxString& name = wxString::Format(_("&Version %s"), pOptions->GetOption(OPTION_UPDATECHECK_NEWVERSION).c_str());
	pMenu->Append(XRCID("ID_CHECKFORUPDATES"), name);
	pFrame->GetMenuBar()->Append(pMenu, _("&New version available!"));

	if (showDialog)
	{
		CUpdateWizard dlg(m_parent);
		if (dlg.Load())
			dlg.Run();
	}
}

void CUpdateWizard::PrepareUpdateAvailablePage(const wxString &newVersion, wxString newUrl)
{
	XRCCTRL(*this, "ID_VERSION", wxStaticText)->SetLabel(newVersion);

	if (newUrl == _T(""))
	{
		if (CBuildInfo::GetBuildType() == _T("official") || CBuildInfo::GetBuildType() == _T("nightly"))
			XRCCTRL(*this, "ID_UPDATEDESC", wxStaticText)->SetLabel(_("Please visit http://filezilla-project.org to download the most recent version."));
		else
			XRCCTRL(*this, "ID_UPDATEDESC", wxStaticText)->SetLabel(_("Please check the package manager of your system for an updated package or visit http://filezilla-project.org to download the source code of FileZilla."));
		m_pages[1]->SetNext(0);
	}
	else
	{
		if (!newUrl.Left(7).CmpNoCase(_T("http://")))
			newUrl = newUrl.Mid(7);
		int pos = newUrl.Find('/');
		if (pos == -1)
		{
			XRCCTRL(*this, "ID_UPDATEDESC", wxStaticText)->SetLabel(_("Please visit http://filezilla-project.org to download the most recent version."));
			XRCCTRL(*this, "ID_UPDATEDESC2", wxStaticText)->Hide();
			m_pages[1]->SetNext(0);
		}
		else
		{
			XRCCTRL(*this, "ID_UPDATEDESC", wxStaticText)->SetLabel(_("Click on next to download the new version."));
			XRCCTRL(*this, "ID_UPDATEDESC2", wxStaticText)->SetLabel(_("Alternatively, visit http://filezilla-project.org to download the most recent version."));
			XRCCTRL(*this, "ID_UPDATEDESC2", wxStaticText)->Show();

			m_urlServer = newUrl.Left(pos);
			m_urlFile = newUrl.Mid(pos);
		}
	}
	RewrapPage(1);
}

void CUpdateWizard::RewrapPage(int page)
{
	m_pages[page]->GetSizer()->Layout();
	m_pages[page]->GetSizer()->Fit(m_pages[page]);
	wxSize size(0, 0);
	for (int i = 0; i < 5; i++)
		if (i != page)
			size.IncTo(m_pages[i]->GetSize());
	wxGetApp().GetWrapEngine()->WrapRecursive(m_pages[page], m_pages[page]->GetSizer(), size.x);
	wxSize newSize = m_pages[page]->GetSizer()->GetMinSize();
	newSize.IncTo(size);
	m_pages[page]->GetSizer()->SetMinSize(newSize);
	m_pages[page]->GetSizer()->Layout();
	m_pages[page]->GetSizer()->Fit(m_pages[page]);
}

#endif //FZ_MANUALUPDATECHECK
