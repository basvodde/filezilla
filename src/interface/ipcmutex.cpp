#include "FileZilla.h"
#include "ipcmutex.h"
#include "filezillaapp.h"

int CInterProcessMutex::m_fd = -1;
int CInterProcessMutex::m_instanceCount = 0;

CInterProcessMutex::CInterProcessMutex(unsigned int mutexType, bool initialLock /*=true*/)
{
	m_locked = false;
#ifdef __WXMSW_
	// Create mutex_
	hMutex = ::CreateMutex(0, false, wxString::Format(_T("FileZilla 3 Mutex Type %d"), mutexType));
#else
	m_type = mutexType;
	if (!m_instanceCount)
	{
		// Open file only if this is the first instance
		wxFileName fn(wxGetApp().GetSettingsDir(), _T("lockfile"));
		m_fd = open(fn.GetFullPath().mb_str(), O_CREAT | O_RDWR, 0644);
	}
	m_instanceCount++;
#endif
	if (initialLock)
		Lock();
}

CInterProcessMutex::~CInterProcessMutex()
{
	if (m_locked)
		Unlock();
#ifdef __WXMSW__
	if (hMutex)
		::CloseHandle(hMutex);
#else
	m_instanceCount--;
	// Close file only if this is the last instance. At least under
	// Linux, closing the lock file has the affect of removing all locks.
	if (!m_instanceCount && m_fd >= 0)
		close(m_fd);
#endif
}

void CInterProcessMutex::Lock()
{
	wxASSERT(!m_locked);
#ifdef __WXMSW__
	if (hMutex)
		::WaitForSingleObject(hMutex, INFINITE);
#else
	if (m_fd >= 0)
	{
		// Lock 1 byte region in the lockfile. m_type specifies the byte to lock.
		struct flock f = {0};
		f.l_type = F_WRLCK;
		f.l_whence = SEEK_SET;
		f.l_start = m_type;
		f.l_len = 1;
		f.l_pid = getpid();
		fcntl(m_fd, F_SETLKW, &f);
	}
#endif

	m_locked = true;
}

bool CInterProcessMutex::TryLock()
{
	wxASSERT(!m_locked);

#ifdef __WXMSW__
	if (!hMutex)
	{
		m_locked = false;
		return false;
	}

	int res = ::WaitForSingleObject(hMutex, 1);
	if (res == WAIT_OBJECT_0)
	{
		m_locked = true;
		return true;
	}
#else
	if (m_fd >= 0)
	{
		// Try to lock 1 byte region in the lockfile. m_type specifies the byte to lock.
		struct flock f = {0};
		f.l_type = F_WRLCK;
		f.l_whence = SEEK_SET;
		f.l_start = m_type;
		f.l_len = 1;
		f.l_pid = getpid();
		if (!fcntl(m_fd, F_SETLK, &f))
		{
			m_locked = true;
			return true;
		}
	}
#endif

	return false;
}

void CInterProcessMutex::Unlock()
{
	wxASSERT(m_locked);
	m_locked = false;

#ifdef __WXMSW__
	if (hMutex)
		::ReleaseMutex(hMutex);
#else
	if (m_fd >= 0)
	{
		// Unlock region specified by m_type.
		struct flock f = {0};
		f.l_type = F_UNLCK;
		f.l_whence = SEEK_SET;
		f.l_start = m_type;
		f.l_len = 1;
		f.l_pid = getpid();
		fcntl(m_fd, F_SETLKW, &f);
	}
#endif
}

