#ifndef __IPCMUTEX_H__
#define __IPCMUTEX_H__

/* 
 * Unfortunately wxWidgets does not provide interprocess mutexes, so I've to
 * use platform specific code here.
 * CInterProcessMutex represents an interprocess mutex. The mutex will be
 * created and locked in the constructor and released in the destructor.
 * Under Windows we use the normal Windows mutexes, under all other platforms
 * we use lockfiles using fcntl advisory locks.
 */

#define MUTEX_OPTIONS 1
#define MUTEX_SITEMANAGER 2
#define MUTEX_SITEMANAGERGLOBAL 3
#define MUTEX_QUEUE 4

class CInterProcessMutex
{
public:
	CInterProcessMutex(unsigned int mutexType, bool initialLock = true);
	~CInterProcessMutex();

	void Lock();
	bool TryLock(); // Tries to lock the mutex. Returns true if successful, else otherwise.
	void Unlock();

	bool IsLocked() const { return m_locked; }

private:

#ifdef __WXMSW__
	// Under windows use normal mutexes
	HANDLE hMutex;
#else
	// Use a lockfile under all other systems
	static int m_fd;
	unsigned int m_type;
	static int m_instanceCount;
#endif

	bool m_locked;
};

#endif //__IPCMUTEX_H__
