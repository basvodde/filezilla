#include "FileZilla.h"

#if FZ_MANUALUPDATECHECK

#include "updatewizard.h"
#include "filezillaapp.h"
#include "Options.h"
#include "buildinfo.h"
#include <wx/stdpaths.h>
#include "Mainfrm.h"
#ifdef __WXMSW__
#include <wx/dynlib.h> // Used by GetDownloadDir
#endif //__WXMSW__

// This is ugly but does the job
#define SHA512_STANDALONE
typedef unsigned int uint32;
#include "../putty/int64.h"
#include "../putty/sshsh512.c"

#if 0
// Used inside of wx to create the next/back buttons on the wizard.
// Listed here so that they are included in the translations
_("&Next >");
_("< &Back");
_("&Finish");
#endif

#define MAXCHECKPROGRESS 11 // Maximum value of progress bar

BEGIN_EVENT_TABLE(CUpdateWizard, wxWizard)
EVT_CHECKBOX(wxID_ANY, CUpdateWizard::OnCheck)
EVT_WIZARD_PAGE_CHANGING(wxID_ANY, CUpdateWizard::OnPageChanging)
EVT_WIZARD_PAGE_CHANGED(wxID_ANY, CUpdateWizard::OnPageChanged)
EVT_FZ_NOTIFICATION(wxID_ANY, CUpdateWizard::OnEngineEvent)
EVT_TIMER(wxID_ANY, CUpdateWizard::OnTimer)
EVT_WIZARD_FINISHED(wxID_ANY, CUpdateWizard::OnFinish)
EVT_WIZARD_CANCEL(wxID_ANY, CUpdateWizard::OnCancel)
END_EVENT_TABLE()

static wxChar s_update_cert[] = _T("-----BEGIN CERTIFICATE-----\n\
MIIFsTCCA5ugAwIBAgIESnXLbzALBgkqhkiG9w0BAQ0wSTELMAkGA1UEBhMCREUx\n\
GjAYBgNVBAoTEUZpbGVaaWxsYSBQcm9qZWN0MR4wHAYDVQQDExVmaWxlemlsbGEt\n\
cHJvamVjdC5vcmcwHhcNMDkwODAyMTcyMjU2WhcNMzEwNjI4MTcyMjU4WjBJMQsw\n\
CQYDVQQGEwJERTEaMBgGA1UEChMRRmlsZVppbGxhIFByb2plY3QxHjAcBgNVBAMT\n\
FWZpbGV6aWxsYS1wcm9qZWN0Lm9yZzCCAh8wCwYJKoZIhvcNAQEBA4ICDgAwggIJ\n\
AoICAJqWXy7YzVP5pOk8VB9bd/ROC9SVbAxJiFHh0I0/JmyW+jSfzFCYWr1DKGVv\n\
Oui+qiUsaSgjWTh/UusnVu4Q4Lb00k7INRF6MFcGFkGNmOZPk4Qt0uuWMtsxiFek\n\
9QMPWSYs+bxk+M0u0rNOdAblsIzeV16yhfUQDtrJxPWbRpuLgp9/4/oNbixet7YM\n\
pvwlns2o1KXcsNcBcXraux5QmnD4oJVYbTY2qxdMVyreA7dxd40c55F6FvA+L36L\n\
Nv54VwRFSqY12KBG4I9Up+c9OQ9HMN0zm0FhYtYeKWzdMIRk06EKAxO7MUIcip3q\n\
7v9eROPnKM8Zh4dzkWnCleirW8EKFEm+4+A8pDqirMooiQqkkMesaJDV361UCoVo\n\
fRhqfK+Prx0BaJK/5ZHN4tmgU5Tmq+z2m7aIKwOImj6VF3somVvmh0G/othnU2MH\n\
GB7qFrIUMZc5VhrAwmmSA2Z/w4+0ToiR+IrdGmDKz3cVany3EZAzWRJUARaId9FH\n\
v/ymA1xcFAKmfxsjGNlNpXd7b8UElS8+ccKL9m207k++IIjc0jUPgrM70rU3cv5M\n\
Kevp971eHLhpWa9vrjbz/urDzBg3Dm8XEN09qwmABfIEnhm6f7oz2bYXjz73ImYj\n\
rZsogz+Jsx3NWhHFUD42iA4ZnxHIEgchD/TAihpbdrEhgmdvAgMBAAGjgacwgaQw\n\
EgYDVR0TAQH/BAgwBgEB/wIBAjAmBgNVHREEHzAdgRthZG1pbkBmaWxlemlsbGEt\n\
cHJvamVjdC5vcmcwDwYDVR0PAQH/BAUDAwcGADAdBgNVHQ4EFgQUd4w2verFjXAn\n\
CrNLor39nFtemNswNgYDVR0fBC8wLTAroCmgJ4YlaHR0cHM6Ly9jcmwuZmlsZXpp\n\
bGxhLXByb2plY3Qub3JnL2NybDALBgkqhkiG9w0BAQ0DggIBAF3fmV/Bs4amV78d\n\
uhe5PkW7yTO6iCfKJVDB22kXPvL0rzZn4SkIZNoac8Xl5vOoRd6k+06i3aJ78w+W\n\
9Z0HK1jUdjW7taYo4bU58nAp3Li+JwjE/lUBNqSKSescPjdZW0KzIIZls91W30yt\n\
tGq85oWAuyVprHPlr2uWLg1q4eUdF6ZAz4cZ0+9divoMuk1HiWxi1Y/1fqPRzUFf\n\
UGK0K36iPPz2ktzT7qJYXRfC5QDoX7tCuoDcO5nccVjDypRKxy45O5Ucm/fywiQW\n\
NQfz/yQAmarQSCfDjNcHD1rdJ0lx9VWP6xi+Z8PGSlR9eDuMaqPVAE1DLHwMMTTZ\n\
93PbfP2nvgbElgEki28LUalyVuzvrKcu/rL1LnCJA4jStgE/xjDofpYwgtG4ZSnE\n\
KgNy48eStvNZbGhwn2YvrxyKmw58WSQG9ArOCHoLcWnpedSZuTrPTLfgNUx7DNbo\n\
qJU36tgxiO0XLRRSetl7jkSIO6U1okVH0/tvstrXEWp4XwdlmoZf92VVBrkg3San\n\
fA5hBaI2gpQwtpyOJzwLzsd43n4b1YcPiyzhifJGcqRCBZA1uArNsH5iG6z/qHXp\n\
KjuMxZu8aM8W2gp8Yg8QZfh5St/nut6hnXb5A8Qr+Ixp97t34t264TBRQD6MuZc3\n\
PqQuF7sJR6POArUVYkRD/2LIWsB7\n\
-----END CERTIFICATE-----\n\
");


class CUpdateWizardOptions : public COptionsBase
{
public:
	virtual int GetOptionVal(unsigned int nID)
	{
		return COptions::Get()->GetOptionVal(nID);
	}

	virtual wxString GetOption(unsigned int nID)
	{
		if (nID == OPTION_INTERNAL_ROOTCERT)
			return s_update_cert;

		return COptions::Get()->GetOption(nID);
	}

	virtual bool SetOption(unsigned int nID, int value)
	{
		return COptions::Get()->SetOption(nID, value);
	}

	virtual bool SetOption(unsigned int nID, wxString value)
	{
		return COptions::Get()->SetOption(nID, value);
	}
};

CUpdateWizard::CUpdateWizard(wxWindow* pParent)
	: m_parent(pParent)
{
	m_pEngine = new CFileZillaEngine;

	m_update_options = new CUpdateWizardOptions();

	m_pEngine->Init(this, m_update_options);

	m_inTransfer = false;
	m_skipPageChanging = false;
	m_currentPage = 0;

	wxString host = CBuildInfo::GetHostname();
	if (host == _T(""))
		host = _T("unknown");

	wxString version(PACKAGE_VERSION, wxConvLocal);
	version.Replace(_T(" "), _T("%20"));
	m_urlProtocol = HTTPS;
	m_urlServer = _T("update.filezilla-project.org");
	m_urlFile = wxString::Format(_T("/updatecheck.php?platform=%s&version=%s"), host.c_str(), version.c_str());
#if defined(__WXMSW__) || defined(__WXMAC__)
	// Makes not much sense to submit OS version on Linux, *BSD and the likes, too many flavours.
	wxString osVersion = wxString::Format(_T("&osversion=%d.%d"), wxPlatformInfo::Get().GetOSMajorVersion(), wxPlatformInfo::Get().GetOSMinorVersion());
	m_urlFile += osVersion;
#endif

	m_statusTimer.SetOwner(this);
	m_autoCheckTimer.SetOwner(this);

	m_autoUpdateCheckRunning = false;

	m_loaded = false;
	m_updateShown = false;
	m_start_check = false;
}

CUpdateWizard::~CUpdateWizard()
{
	delete m_pEngine;
	delete m_update_options;
}

bool CUpdateWizard::Load()
{
	if (!Create(m_parent, wxID_ANY, _("Check for updates"), wxNullBitmap, wxPoint(0, 0)))
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

	if (COptions::Get()->GetOptionVal(OPTION_UPDATECHECK_CHECKBETA) != 0)
		XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->SetValue(1);
	else
		XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->SetValue(0);

	m_loaded = true;

	CenterOnParent();

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
		return RunWizard(m_pages.front());
	}

	// Force another check
	PrepareUpdateCheckPage();
	m_start_check = true;
	m_currentPage = 0;

	return RunWizard(m_pages[0]);
}

void CUpdateWizard::OnCheck(wxCommandEvent& event)
{
	if (event.GetId() == XRCID("ID_CHECKBETA"))
	{
		if (event.IsChecked())
		{
			if (wxMessageBox(_("Do you really want to check for beta versions?\nUnless you want to test new features, keep using stable versions."), _("Update wizard"), wxICON_QUESTION | wxYES_NO, this) != wxYES)
			{
				XRCCTRL(*this, "ID_CHECKBETA", wxCheckBox)->SetValue(0);
				return;
			}
			COptions::Get()->SetOption(OPTION_UPDATECHECK_CHECKBETA, 1);
		}
		else
			COptions::Get()->SetOption(OPTION_UPDATECHECK_CHECKBETA, 0);

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

#ifdef __WXMSW__
// See comment a few lines below
GUID VISTASHIT_FOLDERID_Downloads = { 0x374de290, 0x123f, 0x4565, 0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b };
extern "C" typedef HRESULT (WINAPI *tSHGetKnownFolderPath)(const GUID& rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);
#endif

wxString CUpdateWizard::GetDownloadDir()
{
#ifdef __WXMSW__
	// Old Vista has a profile directory for downloaded files,
	// need to get it using SHGetKnownFolderPath which we need to
	// load dynamically to preserve forward compatibility with the 
	// upgrade to Windows XP.
	wxDynamicLibrary lib(_T("shell32.dll"));
	if (lib.IsLoaded() && lib.HasSymbol(_T("SHGetKnownFolderPath")))
	{
		tSHGetKnownFolderPath pSHGetKnownFolderPath = (tSHGetKnownFolderPath)lib.GetSymbol(_T("SHGetKnownFolderPath"));

		PWSTR path;
		HRESULT result = pSHGetKnownFolderPath(VISTASHIT_FOLDERID_Downloads, 0, 0, &path);
		if (result == S_OK)
		{
			wxString dir = path;
			CoTaskMemFree(path);
			return dir;
		}
	}
#endif
	return wxStandardPaths::Get().GetDocumentsDir();
}

void CUpdateWizard::OnPageChanging(wxWizardEvent& event)
{
	if (m_skipPageChanging)
		return;
	m_skipPageChanging = true;

	if (event.GetPage() == m_pages[0])
	{
		event.Veto();

		PrepareUpdateCheckPage();
		StartUpdateCheck();
	}
	if (event.GetPage() == m_pages[1] && m_pages[1]->GetNext())
	{
		wxString filename = m_urlFile;
		int pos = filename.Find('/', true);
		if (pos != -1)
			filename = filename.Mid(pos + 1);

		const wxString defaultDir = GetDownloadDir();

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

		{
			wxLogNull log;
			wxRemoveFile(dialog.GetPath());
		}
		
		if (wxFileName::FileExists(dialog.GetPath()))
		{
			wxMessageBox(_("Error, local file exists but cannot be removed"));
			event.Veto();
			m_skipPageChanging = false;
			return;
		}

		const wxString file = dialog.GetPath() + _T(".tmp");
		m_localFile = file;

		int i = 1;
		while (wxFileName::FileExists(m_localFile))
		{
			i++;
			m_localFile = file + wxString::Format(_T("%d"), i);
		}
	}
	
	m_skipPageChanging = false;
}

void CUpdateWizard::OnPageChanged(wxWizardEvent& event)
{
	if (event.GetPage() == m_pages[0])
	{
		if (m_start_check)
		{
			m_start_check = false;
			StartUpdateCheck();
		}
		return;
	}

	if (event.GetPage() != m_pages[2])
		return;

	wxButton* pNext = wxDynamicCast(FindWindow(wxID_FORWARD), wxButton);
	pNext->Disable();

	m_currentPage = 2;

	wxStaticText *pText = XRCCTRL(*this, "ID_DOWNLOADTEXT", wxStaticText);
	wxString text = wxString::Format(_("Downloading %s"), (CServer::GetPrefixFromProtocol(m_urlProtocol) + _T("://") + m_urlServer + m_urlFile).c_str());
	text.Replace(_T("&"), _T("&&"));
	pText->SetLabel(text);

	m_inTransfer = false;

	int res = m_pEngine->Command(CConnectCommand(CServer(m_urlProtocol, DEFAULT, m_urlServer, (m_urlProtocol == HTTPS) ? 443 : 80)));
	if (res == FZ_REPLY_OK)
	{
		XRCCTRL(*this, "ID_DOWNLOADPROGRESSTEXT", wxStaticText)->SetLabel(_("Connecting to server"));
		res = SendTransferCommand();

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

	if (m_localFile != _T(""))
	{
		wxLogNull log;
		wxRemoveFile(m_localFile);
	}

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

	XRCCTRL(*this, "ID_LOG", wxTextCtrl)->ChangeValue(m_update_log);
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
						wxString text = pLogMsg->msg;
						text.Replace(_T("&"), _T("&&"));
						WrapText(pText, text, m_pages[0]->GetClientSize().x);
						pText->SetLabel(text);

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
						wxStaticText *pText = XRCCTRL(*this, "ID_DOWNLOADPROGRESSTEXT", wxStaticText);
						
						wxString text = pLogMsg->msg;
						text.Replace(_T("&"), _T("&&"));
						WrapText(pText, text, m_pages[2]->GetClientSize().x);
						pText->SetLabel(text);
						m_pages[2]->GetSizer()->Layout();
					}
				}

				wxString label;
				switch (pLogMsg->msgType)
				{
				case Error:
					label = _("Error:");
					break;
				case Status:
					label = _("Status:");
					break;
				case Command:
					label = _("Command:");
					break;
				case Response:
					label = _("Response:");
					break;
				default:
					break;
				}
				if (label != _T(""))
					m_update_log += label + _T(" ") + pLogMsg->msg + _T("\n");
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
					if (m_loaded && !m_currentPage)
					{
						wxGauge* pProgress = XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge);
						pProgress->SetValue(pProgress->GetValue() + 1);
					}

					int res = SendTransferCommand();
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
					if (!VerifyChecksum())
						break;

					int pos = m_localFile.Find('.', true);
					wxASSERT(pos > 0);
					wxRenameFile(m_localFile, m_localFile.Left(pos));
					m_localFile = m_localFile.Left(pos);

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
				if (!m_inTransfer)
					break;

				wxASSERT(!m_currentPage);
				CDataNotification* pOpMsg = reinterpret_cast<CDataNotification*>(pNotification);
				int len;
				char* data = pOpMsg->Detach(len);

				if (m_data.Len() + len > 131072)
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
						data = 0;
						m_pEngine->Command(CCancelCommand());
						FailedTransfer();
						break;
					}
				}

				if (data)
				{
					m_data += wxString(data, wxConvUTF8, len);
					delete [] data;
				}
				break;
			}
		case nId_asyncrequest:
			{
				CAsyncRequestNotification* pData = reinterpret_cast<CAsyncRequestNotification *>(pNotification);
				if (pData->GetRequestID() == reqId_fileexists)
				{
					reinterpret_cast<CFileExistsNotification *>(pData)->overwriteAction = CFileExistsNotification::overwrite;
				}
				else if (pData->GetRequestID() == reqId_certificate)
				{
					CCertificateNotification* pCertNotification = (CCertificateNotification*)pData;
					pCertNotification->m_trusted = true;
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
	const wxLongLong ownVersionNumber = CBuildInfo::ConvertToVersionNumber(CBuildInfo::GetVersion());

	wxString newVersion;
	wxLongLong newVersionNumber = -1;
	wxString newUrl;
	wxString newChecksum;

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

		if (line == _T(""))
		{
			// After empty line, changelog follows
			m_news = m_data;
			m_news.Trim(true);
			m_news.Trim(false);
			break;
		}
		
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
		wxString url = line;
		if (url == _T("none"))
			url = _T("");

		pos = url.Find(' ');
		if (pos < 1)
			newChecksum.clear();
		else
		{
			newChecksum = url.Mid(pos + 1);
			url = url.Left(pos);
		}

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
			wxLongLong v = CBuildInfo::ConvertToVersionNumber(versionOrDate);
			if (v <= ownVersionNumber)
				continue;
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

		DisplayUpdateAvailability(true);

		return;
	}
	else
	{
		// Since the auto check and the manual check, a newer version might have been published
		COptions* pOptions = COptions::Get();
		if (!pOptions->GetOption(OPTION_UPDATECHECK_NEWVERSION).empty())
			pOptions->SetOption(OPTION_UPDATECHECK_NEWVERSION, newVersion);
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
		PrepareUpdateAvailablePage(newVersion, newUrl, newChecksum);

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
	if (event.GetId() == m_statusTimer.GetId())
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
		wxExecute(_T("\"") + m_localFile +  _T("\" /update"));
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
	
	int days;
	if (CBuildInfo::GetBuildType() == _T("official") && CBuildInfo::IsUnstable())
		days = 3600 * 24;
	else
		days = COptions::Get()->GetOptionVal(OPTION_UPDATECHECK_INTERVAL) * 3600 * 24;
	if (span.GetSeconds() >= days)
		return true;

	return false;
}

void CUpdateWizard::StartUpdateCheck()
{
	m_inTransfer = false;

	if (COptions::Get()->GetOptionVal(OPTION_UPDATECHECK_CHECKBETA) != 0)
		m_urlFile += _T("&beta=1");

	int res = m_pEngine->Command(CConnectCommand(CServer(m_urlProtocol, DEFAULT, m_urlServer, (m_urlProtocol == HTTPS) ? 443 : 80)));
	if (res == FZ_REPLY_OK)
	{
		if (m_loaded)
		{
			XRCCTRL(*this, "ID_CHECKINGTEXTPROGRESS", wxStaticText)->SetLabel(_("Connecting to server"));
			wxGauge* pProgress = XRCCTRL(*this, "ID_CHECKINGPROGRESS", wxGauge);
			pProgress->SetValue(pProgress->GetValue() + 1);
		}
		res = SendTransferCommand();
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

	wxLongLong v = CBuildInfo::ConvertToVersionNumber(newVersion);
	if (v <= CBuildInfo::ConvertToVersionNumber(CBuildInfo::GetVersion()))
	{
		pOptions->SetOption(OPTION_UPDATECHECK_NEWVERSION, _T(""));
		return;
	}

#ifdef __WXMSW__
	// All open menus need to be closed or app will become unresponsive.
	::EndMenu();
#endif

	m_updateShown = true;

	CMainFrame* pFrame = (CMainFrame*)m_parent;
		
	wxMenu* pMenu = new wxMenu();
	const wxString& name = wxString::Format(_("&Version %s"), pOptions->GetOption(OPTION_UPDATECHECK_NEWVERSION).c_str());
	pMenu->Append(XRCID("ID_CHECKFORUPDATES"), name);
	wxMenuBar* pMenuBar = pFrame->GetMenuBar();
	if (pMenuBar)
		pMenuBar->Append(pMenu, _("&New version available!"));

	if (showDialog)
	{
		CUpdateWizard dlg(m_parent);
		if (dlg.Load())
			dlg.Run();
	}
}

void CUpdateWizard::PrepareUpdateAvailablePage(const wxString &newVersion, wxString newUrl, const wxString& newChecksum)
{
	XRCCTRL(*this, "ID_VERSION", wxStaticText)->SetLabel(newVersion);

	if (newUrl == _T(""))
	{
		if (CBuildInfo::GetBuildType() == _T("official") || CBuildInfo::GetBuildType() == _T("nightly"))
			XRCCTRL(*this, "ID_UPDATEDESC", wxStaticText)->SetLabel(_("Please visit http://filezilla-project.org to download the most recent version."));
		else
			XRCCTRL(*this, "ID_UPDATEDESC", wxStaticText)->SetLabel(_("Please check the package manager of your system for an updated package or visit http://filezilla-project.org to download the source code of FileZilla."));
		XRCCTRL(*this, "ID_UPDATEDESC2", wxStaticText)->SetLabel(_T(""));

		XRCCTRL(*this, "ID_NEWS_DESC", wxStaticText)->Hide();
		XRCCTRL(*this, "ID_NEWS", wxTextCtrl)->Hide();
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

			m_urlProtocol = HTTP;
			m_urlServer = newUrl.Left(pos);
			m_urlFile = newUrl.Mid(pos);
			m_update_checksum = newChecksum;
		}
		if (m_news == _T(""))
		{
			XRCCTRL(*this, "ID_NEWS_DESC", wxStaticText)->Hide();
			XRCCTRL(*this, "ID_NEWS", wxTextCtrl)->Hide();
		}
		else
			XRCCTRL(*this, "ID_NEWS", wxTextCtrl)->ChangeValue(m_news);
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

int CUpdateWizard::SendTransferCommand()
{
	m_inTransfer = true;

	CFileTransferCommand::t_transferSettings transferSettings;

	CServerPath path;
	wxString file = m_urlFile;
	path.SetPath(file, true);

	CFileTransferCommand cmd(m_localFile, path, file, true, transferSettings);
	return m_pEngine->Command(cmd);
}

void CUpdateWizard::PrepareUpdateCheckPage()
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
}

bool CUpdateWizard::VerifyChecksum()
{
	if (m_localFile == _T("") || m_update_checksum == _T(""))
		return true;

	if (m_update_checksum.Left(7).CmpNoCase(_T("sha512 ")))
	{
		// Unknown hash algorithm? Fail
		FailedChecksum();
		return false;
	}
	m_update_checksum = m_update_checksum.Mid(7);
	SHA512_State state;
	SHA512_Init(&state);

	wxFile f;
	{
		wxLogNull null;
		if (!f.Open(m_localFile))
		{
			FailedChecksum();
			return false;
		}
	}
	char buffer[65536];
	size_t read;
	while ((read = f.Read(buffer, sizeof(buffer))) > 0)
	{
		SHA512_Bytes(&state, buffer, read);
	}
	if (read < 0)
	{
		FailedChecksum();
		return false;
	}
	f.Close();

	unsigned char raw_digest[64];
	SHA512_Final(&state, raw_digest);

	wxString digest;

	for (unsigned int i = 0; i < sizeof(raw_digest); i++)
	{
		unsigned char l = raw_digest[i] >> 4;
		unsigned char r = raw_digest[i] & 0x0F;

		if (l > 9)
			digest += 'a' + l - 10;
		else
			digest += '0' + l;

		if (r > 9)
			digest += 'a' + r - 10;
		else
			digest += '0' + r;
	}

	if (m_update_checksum.CmpNoCase(digest))
	{
		FailedChecksum();
		return false;
	}

	return true;
}

void CUpdateWizard::FailedChecksum()
{
	m_inTransfer = false;

	if (m_localFile == _T(""))
		return;
	else
	{
		wxLogNull log;
		wxRemoveFile(m_localFile);
	}

	wxString label = _("Checksum mismatch of downloaded file.");
	XRCCTRL(*this, "ID_FAILURE", wxStaticText)->SetLabel(label);

	XRCCTRL(*this, "ID_MISMATCH1", wxStaticText)->Show();
	XRCCTRL(*this, "ID_MISMATCH2", wxStaticText)->Show();
	XRCCTRL(*this, "ID_MISMATCH3", wxStaticText)->Show();
	XRCCTRL(*this, "ID_MISMATCH4", wxStaticText)->Show();
	
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

	XRCCTRL(*this, "ID_LOG", wxTextCtrl)->ChangeValue(m_update_log);

	RewrapPage(3);
}

void CUpdateWizard::OnCancel(wxWizardEvent& event)
{
	delete m_pEngine;
	m_pEngine = 0;
	if (m_localFile != _T(""))
		wxRemoveFile(m_localFile);
}

#endif //FZ_MANUALUPDATECHECK
