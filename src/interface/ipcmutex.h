#ifndef __IPCMUTEX_H__
#define __IPCMUTEX_H__

// Unfortunately wxWidgets does not provide interprocess mutexes, so I've to use 
// platform specific code here.
// CInterProcessMutex represents an interprocess mutex. The mutex will be created and
// locked in the constructor and released in the destructor.

class CInterProcessMutex
{
public:
	CInterProcessMutex(const wxString& name, bool initialLock = true);
	~CInterProcessMutex();

	void Lock();
	bool TryLock();
	void Unlock();

	bool IsLocked() const { return m_locked; }

private:

#ifdef __WXMSW__
	HANDLE hMutex;
#endif

	bool m_locked;
};

#endif //__IPCMUTEX_H__
