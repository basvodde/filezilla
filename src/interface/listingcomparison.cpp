#include "FileZilla.h"
#include "listingcomparison.h"
#include "filter.h"
#include "Options.h"
#include "Mainfrm.h"

CComparableListing::CComparableListing(wxWindow* pParent)
{
	m_pComparisonManager = 0;
	m_pParent = pParent;

	// Init backgrounds for directory comparison
	wxColour background = m_pParent->GetBackgroundColour();
	if (background.Red() + background.Green() + background.Blue() >= 384)
	{
		// Light background
		m_comparisonBackgrounds[0].SetBackgroundColour(wxColour(255, 128, 128));
		m_comparisonBackgrounds[1].SetBackgroundColour(wxColour(255, 255, 128));
		m_comparisonBackgrounds[2].SetBackgroundColour(wxColour(128, 255, 128));
	}
	else
	{
		// Light background
		m_comparisonBackgrounds[0].SetBackgroundColour(wxColour(192, 64, 64));
		m_comparisonBackgrounds[1].SetBackgroundColour(wxColour(192, 192, 64));
		m_comparisonBackgrounds[2].SetBackgroundColour(wxColour(64, 192, 64));
	}

	m_pOther = 0;
}

bool CComparableListing::IsComparing() const
{
	if (!m_pComparisonManager)
		return false;

	return m_pComparisonManager->IsComparing();
}

void CComparableListing::ExitComparisonMode()
{
	if (!m_pComparisonManager)
		return;

	m_pComparisonManager->ExitComparisonMode();
}

void CComparableListing::RefreshComparison()
{
	if (!m_pComparisonManager)
		return;

	if (!IsComparing())
		return;

	if (!CanStartComparison(0) || !GetOther() || !GetOther()->CanStartComparison(0))
	{
		ExitComparisonMode();
		return;
	}

	m_pComparisonManager->CompareListings();
}

bool CComparisonManager::CompareListings()
{
	if (!m_pLeft || !m_pRight)
		return false;

	CFilterDialog filters;
	if (!filters.HasSameLocalAndRemoteFilters())
	{
		wxMessageBox(_("Cannot compare directories, different filters for local and remote directories are enabled"), _("Directory comparison failed"), wxICON_EXCLAMATION);
		return false;
	}

	wxString error;
	if (!m_pLeft->CanStartComparison(&error))
	{
		wxMessageBox(error, _("Directory comparison failed"), wxICON_EXCLAMATION);
		return false;
	}
	if (!m_pRight->CanStartComparison(&error))
	{
		wxMessageBox(error, _("Directory comparison failed"), wxICON_EXCLAMATION);
		return false;
	}

	const int mode = COptions::Get()->GetOptionVal(OPTION_COMPARISONMODE);
	const int threshold = COptions::Get()->GetOptionVal(OPTION_COMPARISON_THRESHOLD);

	m_pLeft->m_pComparisonManager = this;
	m_pRight->m_pComparisonManager = this;

	m_isComparing = true;

	wxToolBar* pToolBar = m_pMainFrame->GetToolBar();
	if (pToolBar)
		pToolBar->ToggleTool(XRCID("ID_TOOLBAR_COMPARISON"), false);	

	wxMenuBar* pMenuBar = m_pMainFrame->GetMenuBar();
	if (pMenuBar)
		pMenuBar->Check(XRCID("ID_TOOLBAR_COMPARISON"), true);

	m_pLeft->StartComparison();
	m_pRight->StartComparison();

	wxString localFile, remoteFile;
	bool localDir = false;
	bool remoteDir = false;
	wxLongLong localSize, remoteSize;
	wxDateTime localDate, remoteDate;
	bool localHasTime = false;
	bool remoteHasTime = false;

	const int dirSortMode = COptions::Get()->GetOptionVal(OPTION_FILELIST_DIRSORT);

	bool gotLocal = m_pLeft->GetNextFile(localFile, localDir, localSize, localDate, localHasTime);
	bool gotRemote = m_pRight->GetNextFile(remoteFile, remoteDir, remoteSize, remoteDate, remoteHasTime);
	while (gotLocal && gotRemote)
	{
		int cmp = CompareFiles(dirSortMode, localFile, remoteFile, localDir, remoteDir);
		if (!cmp)
		{
			if (!mode)
			{
				const CComparableListing::t_fileEntryFlags flag = (localDir || localSize == remoteSize) ? CComparableListing::normal : CComparableListing::different;
				m_pLeft->CompareAddFile(flag);
				m_pRight->CompareAddFile(flag);
			}
			else
			{
				if (!localDate.IsValid() || !remoteDate.IsValid())
				{
					const CComparableListing::t_fileEntryFlags flag = CComparableListing::normal;
					m_pLeft->CompareAddFile(flag);
					m_pRight->CompareAddFile(flag);
				}
				else
				{
					CComparableListing::t_fileEntryFlags localFlag, remoteFlag;

					localDate.SetSecond(0);
					localDate.SetMillisecond(0);
					remoteDate.SetSecond(0);
					remoteDate.SetMillisecond(0);
					if (!localHasTime || !remoteHasTime)
					{
						localDate.ResetTime();
						remoteDate.ResetTime();
					}

					wxTimeSpan span = remoteDate - localDate;
					int minutes = span.GetMinutes();
					if ((minutes >= 0 && minutes <= threshold) ||
						(minutes < 0 && threshold + minutes >= 0))
					{
						localFlag = remoteFlag = CComparableListing::normal;
					}
					else if (minutes > 0)
					{
						localFlag = CComparableListing::normal;
						remoteFlag = CComparableListing::newer;
					}
					else 
					{
						localFlag = CComparableListing::newer;
						remoteFlag = CComparableListing::normal;
					}
					m_pLeft->CompareAddFile(localFlag);
					m_pRight->CompareAddFile(remoteFlag);
				}
			}
			gotLocal = m_pLeft->GetNextFile(localFile, localDir, localSize, localDate, localHasTime);
			gotRemote = m_pRight->GetNextFile(remoteFile, remoteDir, remoteSize, remoteDate, remoteHasTime);
			continue;
		}

		if (cmp == -1)
		{
			m_pLeft->CompareAddFile(CComparableListing::lonely);
			m_pRight->CompareAddFile(CComparableListing::fill);
			gotLocal = m_pLeft->GetNextFile(localFile, localDir, localSize, localDate, localHasTime);
		}
		else
		{
			m_pLeft->CompareAddFile(CComparableListing::fill);
			m_pRight->CompareAddFile(CComparableListing::lonely);
			gotRemote = m_pRight->GetNextFile(remoteFile, remoteDir, remoteSize, remoteDate, remoteHasTime);
		}
	}
	while (gotLocal)
	{
		m_pLeft->CompareAddFile(CComparableListing::lonely);
		m_pRight->CompareAddFile(CComparableListing::fill);
		gotLocal = m_pLeft->GetNextFile(localFile, localDir, localSize, localDate, localHasTime);
	}
	while (gotRemote)
	{
		m_pLeft->CompareAddFile(CComparableListing::fill);
		m_pRight->CompareAddFile(CComparableListing::lonely);
		gotRemote = m_pRight->GetNextFile(remoteFile, remoteDir, remoteSize, remoteDate, remoteHasTime);
	}

	m_pRight->FinishComparison();
	m_pLeft->FinishComparison();

	return false;
}

int CComparisonManager::CompareFiles(const int dirSortMode, const wxString& local, const wxString& remote, bool localDir, bool remoteDir)
{
	switch (dirSortMode)
	{
	default:
		if (localDir)
		{
			if (!remoteDir)
				return -1;
		}
		else if (remoteDir)
			return 1;
		break;
	case 2:
		// Inline
		break;
	}

#ifdef __WXMSW__
	return local.CmpNoCase(remote);
#else
	return local.Cmp(remote);
#endif

	return 0;
}

CComparisonManager::CComparisonManager(CMainFrame* pMainFrame, CComparableListing* pLeft, CComparableListing* pRight)
	: m_pMainFrame(pMainFrame), m_pLeft(pLeft), m_pRight(pRight)
{
	m_isComparing = false;
	m_pLeft->SetOther(m_pRight);
	m_pRight->SetOther(m_pLeft);
}

void CComparisonManager::ExitComparisonMode()
{
	if (!IsComparing())
		return;

	m_isComparing = false;
	if (m_pLeft)
		m_pLeft->OnExitComparisonMode();
	if (m_pRight)
		m_pRight->OnExitComparisonMode();

	wxToolBar* pToolBar = m_pMainFrame->GetToolBar();
	if (pToolBar)
		pToolBar->ToggleTool(XRCID("ID_TOOLBAR_COMPARISON"), false);	

	wxMenuBar* pMenuBar = m_pMainFrame->GetMenuBar();
	if (pMenuBar)
		pMenuBar->Check(XRCID("ID_TOOLBAR_COMPARISON"), false);
}
