#include "FileZilla.h"
#include "ipcmutex.h"

CInterProcessMutex::CInterProcessMutex(const wxString& name, bool initialLock /*=true*/)
{
	m_locked = false;
#ifdef __WXMSW__
	hMutex = ::CreateMutex(0, false, wxString::Format(_T("FileZilla 3 %s Mutex")));
	if (hMutex && initialLock)
		Lock();
#else
	#pragma message(__LOCWRN__("No interprocess synchronization for reading/writing Queue.xml available"))
#endif
}

CInterProcessMutex::~CInterProcessMutex()
{
#ifdef __WXMSW__
	if (hMutex)
	{
		if (m_locked)
			::ReleaseMutex(hMutex);
		::CloseHandle(hMutex);
	}
#else
#endif
}

void CInterProcessMutex::Lock()
{
	wxASSERT(!m_locked);
#ifdef __WXMSW__
	if (!hMutex)
	{
		m_locked = true;
		return;
	}

	::WaitForSingleObject(hMutex, INFINITE);
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
	m_locked = true;
	return true;
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
#endif
}
