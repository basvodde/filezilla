#include "FileZilla.h"
#include "updatewizard.h"
#include "filezillaapp.h"
#include "Options.h"
#include "buildinfo.h"

#define MAXCHECKPROGRESS 9 // Maximum value of progress bar

BEGIN_EVENT_TABLE(CUpdateWizard, wxWizard)
EVT_CHECKBOX(wxID_ANY, CUpdateWizard::OnCheck)
EVT_WIZARD_PAGE_CHANGING(wxID_ANY, CUpdateWizard::OnPageChanging)
EVT_FZ_NOTIFICATION(wxID_ANY, CUpdateWizard::OnEngineEvent)
END_EVENT_TABLE()

CUpdateWizard::CUpdateWizard(wxWindow* pParent)
	: m_parent(pParent)
{
	m_pEngine = new CFileZillaEngine;

	m_pEngine->Init(this, COptions::Get());

	m_inTransfer = false;
	m_skipPageChanging = false;
	m_currentPage = 1;

#ifdef __WXMSW__
	const wxChar system[] = _T("Win32");
#else
	const wxChar system[] = _T("Other");
#endif
	wxString version(PACKAGE_VERSION, wxConvLocal);
	version.Replace(_T(" "), _T("%20"));
	m_urlServer = _T("update.filezilla-project.org");
	m_urlFile = wxString::Format(_T("updatecheck.php?platform=%s&version=%s"), system, version.c_str());
}

CUpdateWizard::~CUpdateWizard()
{
	delete m_pEngine;
}

bool CUpdateWizard::Load()
{
	if (!Create(m_parent, wxID_ANY, _("Check for updates")))
		return false;

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
	wxGetApp().GetWrapEngine()->WrapRecursive(windows, 1.7, "UpdateCheck");

	XRCCTRL(*this, "ID_CHECKINGTEXT", wxStaticText)->Hide();
	XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge)->Hide();
	XRCCTRL(*this, "ID_CHECKINGTEXTPROGRESS", wxStaticText)->Hide();

	XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge)->SetRange(MAXCHECKPROGRESS);

	m_pages[3]->SetPrev(0);
	m_pages[3]->SetNext(0);
	m_pages[4]->SetPrev(0);
	m_pages[4]->SetNext(0);


	return true;
}

bool CUpdateWizard::Run()
{
	return RunWizard(m_pages.front());
}

void CUpdateWizard::OnCheck(wxCommandEvent& event)
{
	if (event.GetId() == XRCID("ID_CHECKBETA") && event.IsChecked())
	{
		if (wxMessageBox(_("Do you really want to check for beta versions?\nUnless you want to test new features, keep using stable versions."), _("Update wizard"), wxICON_QUESTION | wxYES_NO) != wxYES)
		{
			XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->SetValue(0);
			return;
		}
	}
	else if (event.GetId() == XRCID("ID_CHECKNIGHTLY") && event.IsChecked())
	{
		if (wxMessageBox(_("Warning, use nightly builds at your own risk.\nNo support is given for nightly builds.\nNightly builds may not work as expected and might even damage your system.\n\nDo you really want to check for nightly builds?"), _("Update wizard"), wxICON_EXCLAMATION | wxYES_NO) != wxYES)
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
		pText->SetLabel(_("Revolving hostname"));


		event.Veto();

		int res = m_pEngine->Command(CConnectCommand(CServer(HTTP, DEFAULT, m_urlServer, 80)));
		if (res == FZ_REPLY_OK)
		{
			m_inTransfer = true;
			pText->SetLabel(_("Connecting to server"));
			pProgress->SetValue(pProgress->GetValue() + 1);
			CFileTransferCommand::t_transferSettings transferSettings;
			CFileTransferCommand cmd(_T(""), CServerPath(_T("/")), m_urlFile, true, transferSettings);
			res = m_pEngine->Command(cmd);
		}
		if (res == FZ_REPLY_OK)
			ShowPage(m_pages[1]);
		else if (res != FZ_REPLY_WOULDBLOCK)
			FailedCheck();
	}
	
	m_skipPageChanging = false;
}

void CUpdateWizard::FailedCheck()
{
	m_inTransfer = false;
	XRCCTRL(*this, "ID_FAILURE", wxStaticText)->SetLabel(_("Failed to check for newer version of FileZilla."));
	
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
				CLogmsgNotification* pLogMsg = reinterpret_cast<CLogmsgNotification *>(pNotification);
				if (pLogMsg->msgType == Status || pLogMsg->msgType == Command)
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
					FailedCheck();
					return;
				}
				if (!m_inTransfer)
				{
					m_inTransfer = true;
					wxGauge* pProgress = XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge);
					pProgress->SetValue(pProgress->GetValue() + 1);
					CFileTransferCommand::t_transferSettings transferSettings;
					CFileTransferCommand cmd(_T(""), CServerPath(_T("/")), m_urlFile, true, transferSettings);
					int res = m_pEngine->Command(cmd);
					if (res == FZ_REPLY_OK)
						ShowPage(m_pages[1]);
					else if (res != FZ_REPLY_WOULDBLOCK)
						FailedCheck();
				}
				else
				{
					ParseData();
				}
			}
			break;
		case nId_data:
			{
				CDataNotification* pOpMsg = reinterpret_cast<CDataNotification*>(pNotification);
				int len;
				char* data = pOpMsg->Detach(len);

				if (m_data.Len() + len > 4096)
				{
					delete [] data;
					m_pEngine->Command(CCancelCommand());
					FailedCheck();
					break;
				}
				for (int i = 0; i < len; i++)
				{
					if (!data[i] || (unsigned char)data[i] > 127)
					{
						delete [] data;
						m_pEngine->Command(CCancelCommand());
						FailedCheck();
						break;
					}
				}

				m_data += wxString(data, wxConvUTF8, len);
				delete [] data;
			}
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

			if (!XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->GetValue() &&
				!((ownVersionNumber & 0x0FFFFF) != 0 && newVersionNumber != 0))
				continue;

			if (newVersionNumber >= CBuildInfo::ConvertToVersionNumber(versionOrDate))
				continue;
		}
		else
		{
			// Final releases
			if (CBuildInfo::ConvertToVersionNumber(versionOrDate) < newVersionNumber && XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->GetValue())
				continue;
		}

		newVersion = versionOrDate;
		newVersionNumber = CBuildInfo::ConvertToVersionNumber(versionOrDate);
		newUrl = url;
	}

	if (newVersionNumber == 0)
	{
		m_skipPageChanging = true;
		ShowPage(m_pages[4]);
		m_currentPage = 4;
		m_skipPageChanging = false;
	}
	else
	{
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
