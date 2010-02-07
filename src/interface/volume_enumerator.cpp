#include "filezilla.h"
#include "volume_enumerator.h"

#ifdef __WXMSW__

#include <wx/msw/registry.h>

DEFINE_EVENT_TYPE(fzEVT_VOLUMEENUMERATED)
DEFINE_EVENT_TYPE(fzEVT_VOLUMESENUMERATED)

CVolumeDescriptionEnumeratorThread::CVolumeDescriptionEnumeratorThread(wxEvtHandler* pEvtHandler)
	: wxThreadEx(wxTHREAD_JOINABLE),
	  m_pEvtHandler(pEvtHandler)
{
	m_failure = false;
	m_stop = false;
	m_running = true;

	if (Create() != wxTHREAD_NO_ERROR)
	{
		m_running = false;
		m_failure = true;
	}
	Run();
}

CVolumeDescriptionEnumeratorThread::~CVolumeDescriptionEnumeratorThread()
{
	m_stop = true;
	Wait();

	for (std::list<t_VolumeInfoInternal>::const_iterator iter = m_volumeInfo.begin(); iter != m_volumeInfo.end(); iter++)
	{
		delete [] iter->pVolume;
		delete [] iter->pVolumeName;
	}
	m_volumeInfo.clear();
}

wxThread::ExitCode CVolumeDescriptionEnumeratorThread::Entry()
{
	if (!GetDrives())
		m_failure = true;

	m_running = false;

	wxCommandEvent evt(fzEVT_VOLUMESENUMERATED);
	m_pEvtHandler->AddPendingEvent(evt);

	return 0;
}

bool CVolumeDescriptionEnumeratorThread::GetDrive(const wxChar* pDrive, const int len)
{
	wxChar* pVolume = new wxChar[len + 1];
	wxStrcpy(pVolume, pDrive);
	if (pVolume[len - 1] == '\\')
		pVolume[len - 1] = 0;
	if (!*pVolume)
	{
		delete [] pVolume;
		return false;
	}

	// Check if it is a network share
	wxChar *share_name = new wxChar[512];
	DWORD dwSize = 511;
	if (!WNetGetConnection(pVolume, share_name, &dwSize))
	{
		m_crit_section.Enter();
		t_VolumeInfoInternal volumeInfo;
		volumeInfo.pVolume = pVolume;
		volumeInfo.pVolumeName = share_name;
		m_volumeInfo.push_back(volumeInfo);
		m_crit_section.Leave();
		pDrive += len + 1;
		return true;
	}
	else
		delete [] share_name;

	// Get the label of the drive
	wxChar* pVolumeName = new wxChar[501];
	int oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
	BOOL res = GetVolumeInformation(pDrive, pVolumeName, 500, 0, 0, 0, 0, 0);
	SetErrorMode(oldErrorMode);
	if (res && pVolumeName[0])
	{
		m_crit_section.Enter();
		t_VolumeInfoInternal volumeInfo;
		volumeInfo.pVolume = pVolume;
		volumeInfo.pVolumeName = pVolumeName;
		m_volumeInfo.push_back(volumeInfo);
		m_crit_section.Leave();
		return true;
	}

	delete [] pVolumeName;
	delete [] pVolume;

	return false;
}

bool CVolumeDescriptionEnumeratorThread::GetDrives()
{
	long drivesToHide = 0;
	// Adhere to the NODRIVES group policy
	wxRegKey key(_T("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"));
	if (key.Exists())
	{
		if (!key.HasValue(_T("NoDrives")) || !key.QueryValue(_T("NoDrives"), &drivesToHide))
			drivesToHide = 0;
	}

	int len = GetLogicalDriveStrings(0, 0);
	if (!len)
		return false;

	wxChar* drives = new wxChar[len + 1];

	if (!GetLogicalDriveStrings(len, drives))
	{
		delete [] drives;
		return false;
	}

	const wxChar* drive_a = 0;

	const wxChar* pDrive = drives;
	while (*pDrive)
	{
		if (m_stop)
		{
			delete [] drives;
			return false;
		}

		// Check if drive should be hidden by default
		if (pDrive[0] != 0 && pDrive[1] == ':')
		{
			int bit = 0;
			char letter = pDrive[0];
			if (letter >= 'A' && letter <= 'Z')
				bit = 1 << (letter - 'A');
			if (letter >= 'a' && letter <= 'z')
				bit = 1 << (letter - 'a');

			if (drivesToHide & bit)
			{
				pDrive += wxStrlen(pDrive) + 1;
				continue;
			}
		}

		const int len = wxStrlen(pDrive);

		if ((pDrive[0] == 'a' || pDrive[0] == 'A') && !drive_a)
		{
			// Defer processing of A:, most commonly the slowest of all drives.
			drive_a = pDrive;
			pDrive += len + 1;
			continue;
		}
		if (GetDrive(pDrive, len))
		{
			wxCommandEvent evt(fzEVT_VOLUMEENUMERATED);
			m_pEvtHandler->AddPendingEvent(evt);
		}

		pDrive += len + 1;
	}

	if (drive_a)
	{
		const int len = wxStrlen(drive_a);
		if (GetDrive(drive_a, len))
		{
			wxCommandEvent evt(fzEVT_VOLUMEENUMERATED);
			m_pEvtHandler->AddPendingEvent(evt);
		}
	}

	delete [] drives;

	return true;
}

std::list<CVolumeDescriptionEnumeratorThread::t_VolumeInfo> CVolumeDescriptionEnumeratorThread::GetVolumes()
{
	std::list<t_VolumeInfo> volumeInfo;

	m_crit_section.Enter();
	
	for (std::list<t_VolumeInfoInternal>::const_iterator iter = m_volumeInfo.begin(); iter != m_volumeInfo.end(); iter++)
	{
		t_VolumeInfo info;
		info.volume = iter->pVolume;
		delete [] iter->pVolume;
		info.volumeName = iter->pVolumeName;
		delete [] iter->pVolumeName;
		volumeInfo.push_back(info);
	}
	m_volumeInfo.clear();

	m_crit_section.Leave();

	return volumeInfo;
}

#endif //__WXMSW__
