#include "FileZilla.h"
#include "listingcomparison.h"
#include "filter.h"
#include "Options.h"

CComparableListing::CComparableListing(wxWindow* pParent)
{
	m_pParent = pParent;

	// Init backgrounds for directory comparison
	wxColour background = m_pParent->GetBackgroundColour();
	if (background.Red() + background.Green() + background.Blue() >= 384)
	{
		// Light background
		m_comparisonBackgrounds[0].SetBackgroundColour(wxColour(255, 128, 128));
		m_comparisonBackgrounds[1].SetBackgroundColour(wxColour(255, 255, 128));
	}
	else
	{
		// Light background
		m_comparisonBackgrounds[0].SetBackgroundColour(wxColour(192, 64, 64));
		m_comparisonBackgrounds[1].SetBackgroundColour(wxColour(192, 192, 64));
	}
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

	m_pLeft->StartComparison();
	m_pRight->StartComparison();

	wxString localFile, remoteFile;
	bool localDir = false;
	bool remoteDir = false;
	wxLongLong localSize, remoteSize;

	const int dirSortMode = COptions::Get()->GetOptionVal(OPTION_FILELIST_DIRSORT);

	bool gotLocal = m_pLeft->GetNextFile(localFile, localDir, localSize);
	bool gotRemote = m_pRight->GetNextFile(remoteFile, remoteDir, remoteSize);
	while (gotLocal && gotRemote)
	{
		int cmp = CompareFiles(dirSortMode, localFile, remoteFile, localDir, remoteDir);
		if (!cmp)
		{
			CComparableListing::t_fileEntryFlags flag = (localDir || localSize == remoteSize) ? CComparableListing::normal : CComparableListing::different;
			m_pLeft->CompareAddFile(flag);
			m_pRight->CompareAddFile(flag);
			gotLocal = m_pLeft->GetNextFile(localFile, localDir, localSize);
			gotRemote = m_pRight->GetNextFile(remoteFile, remoteDir, remoteSize);
			continue;
		}

		if (cmp == -1)
		{
			m_pLeft->CompareAddFile(CComparableListing::lonely);
			m_pRight->CompareAddFile(CComparableListing::fill);
			gotLocal = m_pLeft->GetNextFile(localFile, localDir, localSize);
		}
		else
		{
			m_pLeft->CompareAddFile(CComparableListing::fill);
			m_pRight->CompareAddFile(CComparableListing::lonely);
			gotRemote = m_pRight->GetNextFile(remoteFile, remoteDir, remoteSize);
		}
	}
	while (gotLocal)
	{
		m_pLeft->CompareAddFile(CComparableListing::lonely);
		m_pRight->CompareAddFile(CComparableListing::fill);
		gotLocal = m_pLeft->GetNextFile(localFile, localDir, localSize);
	}
	while (gotRemote)
	{
		m_pLeft->CompareAddFile(CComparableListing::fill);
		m_pRight->CompareAddFile(CComparableListing::lonely);
		gotRemote = m_pRight->GetNextFile(remoteFile, remoteDir, remoteSize);
	}

	m_pLeft->FinishComparison();
	m_pRight->FinishComparison();

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
	case 1:
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

CComparisonManager::CComparisonManager(CComparableListing* pLeft, CComparableListing* pRight)
	: m_pLeft(pLeft), m_pRight(pRight)
{
}
