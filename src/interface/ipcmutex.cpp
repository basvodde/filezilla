#include "FileZilla.h"
#include "ipcmutex.h"
#include "filezillaapp.h"

int CInterProcessMutex::m_fd = -1;
int CInterProcessMutex::m_instanceCount = 0;

CInterProcessMutex::CInterProcessMutex(unsigned int mutexType, bool initialLock /*=true*/)
{
	m_locked = false;
#ifdef __WXMSW__
	hMutex = ::CreateMutex(0, false, wxString::Format(_T("FileZilla 3 Mutex Type %d"), mutexType));
#else
	m_type = mutexType;
	wxFileName fn(wxGetApp().GetSettingsDir(), _T("lockfile"));
	if (!m_instanceCount)
		m_fd = open(fn.GetFullPath().mb_str(), O_CREAT | O_RDWR, 0644);
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

