#include "FileZilla.h"
#include "asyncrequestqueue.h"
#include "fileexistsdlg.h"
#include "Mainfrm.h"

CAsyncRequestQueue::CAsyncRequestQueue(CMainFrame *pMainFrame)
{
	m_pMainFrame = pMainFrame;
}

CAsyncRequestQueue::~CAsyncRequestQueue()
{
}

void CAsyncRequestQueue::AddRequest(CFileZillaEngine *pEngine, CAsyncRequestNotification *pNotification)
{
	t_queueEntry entry;
	entry.pEngine = pEngine;
	entry.pNotification = pNotification;

	if (!m_requestList.empty())
	{
		// Remove older requests coming from the same engine, but never the first
		// entry in the list as that one displays a dialog at this moment.
		for (std::list<t_queueEntry>::iterator iter = m_requestList.begin()++; iter != m_requestList.end(); iter++)
		{
			if (iter->pEngine == pEngine)
			{
				m_requestList.erase(iter);
				break;
			}
		}
	}
	m_requestList.push_back(entry);

	if (m_requestList.size() == 1)
		ProcessNextRequest();
}

void CAsyncRequestQueue::ProcessNextRequest()
{
	while (!m_requestList.empty())
	{
		t_queueEntry &entry = m_requestList.front();

		if (!entry.pEngine->IsPendingAsyncRequestReply(entry.pNotification))
		{
			delete entry.pNotification;
			m_requestList.pop_front();
			continue;
		}
		
		if (entry.pNotification->GetRequestID() == reqId_fileexists)
		{
			CFileExistsNotification *pNotification = reinterpret_cast<CFileExistsNotification *>(entry.pNotification);
			CFileExistsDlg dlg(pNotification);
			dlg.Create(m_pMainFrame);
			int res = dlg.ShowModal();
			
			if (res == wxID_OK)
			{
				switch (dlg.GetAction())
				{
					case 0:
						pNotification->overwriteAction = CFileExistsNotification::overwrite;
						break;
					case 1:
						pNotification->overwriteAction = CFileExistsNotification::overwriteNewer;
						break;
					case 2:
						pNotification->overwriteAction = CFileExistsNotification::resume;
						break;
					case 3:
						{
							wxString msg;
							wxString defaultName;
							if (pNotification->download)
							{
								msg.Printf(_("The file %s already exists.\nPlease enter a new name:"), pNotification->localFile.c_str());
								wxFileName fn = pNotification->localFile;
								defaultName = fn.GetFullName();
							}
							else
							{
								wxString fullName = pNotification->remotePath.GetPath() + pNotification->remoteFile;
								msg.Printf(_("The file %s already exists.\nPlease enter a new name:"), fullName.c_str());
								defaultName = pNotification->remoteFile;
							}
							wxTextEntryDialog dlg(m_pMainFrame, msg, _("Rename file"), defaultName);
							
							// Repeat until user cancels or entery a new name
							while (1)
							{
								int res = dlg.ShowModal();
								if (res == wxID_OK)
								{
									if (dlg.GetValue() == _T(""))
										continue; // Disallow empty names
									if (dlg.GetValue() == defaultName)
									{
										wxMessageDialog dlg2(m_pMainFrame, _("You did not enter a new name for the file. Overwrite the file instead?"), _("Filename unchanged"), 
															wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION | wxCANCEL);
										int res = dlg2.ShowModal();
										
										if (res == wxID_CANCEL)
											pNotification->overwriteAction = CFileExistsNotification::skip;
										else if (res == wxID_NO)
											continue;
										else
											pNotification->overwriteAction = CFileExistsNotification::skip;
									}
									else
									{
										pNotification->overwriteAction = CFileExistsNotification::rename;
										pNotification->newName = dlg.GetValue();
									}
								}
								else
									pNotification->overwriteAction = CFileExistsNotification::skip;
								break;
							}
						}
						break;
					case 4:
						pNotification->overwriteAction = CFileExistsNotification::skip;
						break;
					default:
 						pNotification->overwriteAction = CFileExistsNotification::overwrite;
				}
			}
			else
				pNotification->overwriteAction = CFileExistsNotification::skip;

			entry.pEngine->SetAsyncRequestReply(entry.pNotification);
			delete pNotification;
		}
		else if (entry.pNotification->GetRequestID() == reqId_interactiveLogin)
		{
			CInteractiveLoginNotification* pNotification = reinterpret_cast<CInteractiveLoginNotification*>(entry.pNotification);

			if (m_pMainFrame->GetPassword(pNotification->server, _T(""), pNotification->challenge))
				pNotification->passwordSet = true;

			entry.pEngine->SetAsyncRequestReply(pNotification);
			delete pNotification;
		}
		else if (entry.pNotification->GetRequestID() == reqId_hostkey || entry.pNotification->GetRequestID() == reqId_hostkeyChanged)
		{
			CHostKeyNotification *pNotification = reinterpret_cast<CHostKeyNotification *>(entry.pNotification);

			wxDialogEx* pDlg = new wxDialogEx;
			if (pNotification->GetRequestID() == reqId_hostkey)
				pDlg->Load(m_pMainFrame, _T("ID_HOSTKEY"));
			else
				pDlg->Load(m_pMainFrame, _T("ID_HOSTKEYCHANGED"));

			pDlg->WrapText(pDlg, XRCID("ID_DESC"), 400);

			pDlg->SetLabel(XRCID("ID_HOST"), wxString::Format(_T("%s:%d"), pNotification->GetHost().c_str(), pNotification->GetPort()));
			pDlg->SetLabel(XRCID("ID_FINGERPRINT"), pNotification->GetFingerprint());

			pDlg->GetSizer()->Fit(pDlg);
			pDlg->GetSizer()->SetSizeHints(pDlg);

			int res = pDlg->ShowModal();

			if (res == wxID_OK)
			{
				pNotification->m_trust = true;
				pNotification->m_alwaysTrust = XRCCTRL(*pDlg, "ID_ALWAYS", wxCheckBox)->GetValue();
			}
			
			entry.pEngine->SetAsyncRequestReply(pNotification);
			delete pNotification;
		}
		

		m_requestList.pop_front();
	}
}
