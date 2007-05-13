#include "FileZilla.h"
#include "clearprivatedata.h"
#include "Mainfrm.h"
#include "recentserverlist.h"
#include "quickconnectbar.h"
#include "state.h"
#include "commandqueue.h"
#include "options.h"
#include "ipcmutex.h"
#include "filezillaapp.h"
#include "QueueView.h"

BEGIN_EVENT_TABLE(CClearPrivateDataDialog, wxDialogEx)
EVT_TIMER(1, CClearPrivateDataDialog::OnTimer)
END_EVENT_TABLE()

CClearPrivateDataDialog::CClearPrivateDataDialog(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
{
}

void CClearPrivateDataDialog::Show()
{
	if (!Load(m_pMainFrame, _T("ID_CLEARPRIVATEDATA")))
		return;

	if (ShowModal() != wxID_OK)
		return;

	wxCheckBox *pSitemanagerCheck = XRCCTRL(*this, "ID_CLEARSITEMANAGER", wxCheckBox);
	wxCheckBox *pQueueCheck = XRCCTRL(*this, "ID_CLEARQUEUE", wxCheckBox);
	if (pSitemanagerCheck->GetValue() && pQueueCheck->GetValue())
	{
		int res = wxMessageBox(_("Do you really want to delete all Site Manager entries and the transfer queue?"), _("Clear private data"), wxYES | wxNO | wxICON_QUESTION);
		if (res != wxYES)
			return;
	}
	else if (pQueueCheck->GetValue())
	{
		int res = wxMessageBox(_("Do you really want to delete the transfer queue?"), _("Clear private data"), wxYES | wxNO | wxICON_QUESTION);
		if (res != wxYES)
			return;
	}
	else if (pSitemanagerCheck->GetValue())
	{
		int res = wxMessageBox(_("Do you really want to delete all Site Manager entries?"), _("Clear private data"), wxYES | wxNO | wxICON_QUESTION);
		if (res != wxYES)
			return;
	}

	wxCheckBox *pCheck = XRCCTRL(*this, "ID_CLEARQUICKCONNECT", wxCheckBox);
	if (!pCheck)
		return;

	if (pCheck->GetValue())
	{
		CRecentServerList::Clear();
		m_pMainFrame->GetQuickconnectBar()->ClearFields();
	}

	pCheck = XRCCTRL(*this, "ID_CLEARRECONNECT", wxCheckBox);
	
	if (pCheck->GetValue())
	{
		CState* pState = m_pMainFrame->GetState();
		if (pState->IsConnected() || !pState->IsIdle())
		{
			int res = wxMessageBox(_("Reconnect information cannot be cleared while connected to a server.\nIf you continue, your connection will be disconnected."), _("Clear private data"), wxOK | wxCANCEL);
			if (res != wxOK)
				return;

			if (!pState->m_pCommandQueue->Cancel())
			{
				m_timer.SetOwner(this);
				m_timer.Start(250, true);
			}
			else
				pState->m_pCommandQueue->ProcessCommand(new CDisconnectCommand());
		}

		// Doesn't harm to do it now, but has to be repeated later just to be safe
		ClearReconnect();
	}

	if (pSitemanagerCheck->GetValue())
	{
		CInterProcessMutex sitemanagerMutex(MUTEX_SITEMANAGERGLOBAL, false);
		while (!sitemanagerMutex.TryLock())
		{
			int res = wxMessageBox(_("The Site Manager is opened in another instance of FileZilla 3.\nPlease close it or the data cannot be deleted."), _("Clear private data"), wxOK | wxCANCEL);
			if (res != wxYES)
				return;
		}
		CInterProcessMutex mutex(MUTEX_SITEMANAGER);
		RemoveXmlFile(_T("sitemanager"));
	}

	if (pQueueCheck->GetValue())
	{
		m_pMainFrame->GetQueue()->SetActive(false);
		m_pMainFrame->GetQueue()->RemoveAll();

		CInterProcessMutex mutex(MUTEX_QUEUE);
		RemoveXmlFile(_T("queue"));
	}
}

void CClearPrivateDataDialog::OnTimer(wxTimerEvent& event)
{
	CState* pState = m_pMainFrame->GetState();

	if (pState->IsConnected() || !pState->IsIdle())
	{
		if (!pState->m_pCommandQueue->Cancel())
			return;

		pState->m_pCommandQueue->ProcessCommand(new CDisconnectCommand());
	}

	if (pState->IsConnected() || !pState->IsIdle())
		return;

	m_timer.Stop();
	ClearReconnect();
	Delete();
}

void CClearPrivateDataDialog::Delete()
{
	if (m_timer.IsRunning())
		return;

	Destroy();
}

bool CClearPrivateDataDialog::ClearReconnect()
{
	COptions::Get()->SetLastServer(CServer());
	COptions::Get()->SetOption(OPTION_LASTSERVERPATH, _T(""));

	return true;
}

void CClearPrivateDataDialog::RemoveXmlFile(const wxString& name)
{
	{
		wxFileName fn(wxGetApp().GetSettingsDir(), name + _T(".xml"));
		if (fn.FileExists())
			wxRemoveFile(fn.GetFullPath());
	}
	{
		wxFileName fn(wxGetApp().GetSettingsDir(), name + _T("xml~"));
		if (fn.FileExists())
			wxRemoveFile(fn.GetFullPath());
	}
}
