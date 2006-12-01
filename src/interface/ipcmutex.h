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

enum t_ipcMutexType
{
	// Important: Never ever change a value.
	// If adding a new mutex type, give it the value of MUTEX_LASTFREE and 
	// increase MUTEX_LASTFREE by one.
	// Otherwise this will cause interesting effects between different
	// versions of FileZilla
	MUTEX_OPTIONS = 1,
	MUTEX_SITEMANAGER = 2,
	MUTEX_SITEMANAGERGLOBAL = 3,
	MUTEX_QUEUE = 4,
	MUTEX_FILTERS = 5,
	MUTEX_LAYOUT = 6,
	MUTEX_MOSTRECENTSERVERS = 7,
	MUTEX_TRUSTEDCERTS = 8,
	
	MUTEX_LASTFREE = 9
};

class CInterProcessMutex
{
public:
	CInterProcessMutex(enum t_ipcMutexType mutexType, bool initialLock = true);
	~CInterProcessMutex();

	void Lock();
	bool TryLock(); // Tries to lock the mutex. Returns true if successful, else otherwise.
	void Unlock();

	bool IsLocked() const { return m_locked; }

	enum t_ipcMutexType GetType() const { return m_type; }

private:

#ifdef __WXMSW__
	// Under windows use normal mutexes
	HANDLE hMutex;
#else
	// Use a lockfile under all other systems
	static int m_fd;
	static int m_instanceCount;
#endif
	enum t_ipcMutexType m_type;

	bool m_locked;
};

class CReentrantInterProcessMutexLocker
{
public:
	CReentrantInterProcessMutexLocker(enum t_ipcMutexType mutexType);
	~CReentrantInterProcessMutexLocker();

protected:
	struct t_data
	{
		CInterProcessMutex* pMutex;
		unsigned int lockCount;
	};
	static std::list<t_data> m_mutexes;

	enum t_ipcMutexType m_type;
};

#endif //__IPCMUTEX_H__
