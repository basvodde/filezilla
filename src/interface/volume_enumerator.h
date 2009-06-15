#ifndef __VOLUME_ENUMERATOR_H__
#define __VOLUME_ENUMERATOR_H__

// Class to enumerate volume labels of volumes assigned
// a drive letter under MSW.
// Also gets the full UNC path for drive-mapped network
// shares.

// Windows has this very exotic concept of drive letters (nowadays called
// volumes), even if the drive isn't mounted (in the sense of no media
// inserted).
// This can result in a long seek time if trying to enumerate the volume
// labels, especially with legacy floppy drives (why are people still using
// them?). Worse, even if no floppy drive is installed the BIOS can report
// one to exist and Windows dutifully displays A:

// Since the local directory tree including the drives is populated at
// startup, use a background thread to obtain the labels.
#ifdef __WXMSW__

DECLARE_EVENT_TYPE(fzEVT_VOLUMEENUMERATED, -1)
DECLARE_EVENT_TYPE(fzEVT_VOLUMESENUMERATED, -1)

class CVolumeDescriptionEnumeratorThread : protected wxThreadEx
{
public:
	CVolumeDescriptionEnumeratorThread(wxEvtHandler* pEvtHandler);
	virtual ~CVolumeDescriptionEnumeratorThread();

	bool Failed() const { return m_failure; }

	struct t_VolumeInfo
	{
		wxString volume;
		wxString volumeName;
	};

	std::list<t_VolumeInfo> GetVolumes();

protected:
	bool GetDrives();
	bool GetDrive(const wxChar* pDrive, const int len);
	virtual ExitCode Entry();

	wxEvtHandler* m_pEvtHandler;

	bool m_failure;
	bool m_stop;
	bool m_running;

	struct t_VolumeInfoInternal
	{
		wxChar* pVolume;
		wxChar* pVolumeName;
	};

	std::list<t_VolumeInfoInternal> m_volumeInfo;

	wxCriticalSection m_crit_section;
};

#endif //__WXMSW__

#endif //__VOLUME_ENUMERATOR_H__
