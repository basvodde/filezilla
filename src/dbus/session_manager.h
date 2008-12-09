#ifndef __SESSIONMANAGER_H__
#define __SESSIONMANAGER_H__

// This class communicates with the gnome session manager through D-Bus.
// Enables the program to gracefully react to an ending session.

class CSessionManager
{
public:
	static bool Init();
	static bool Uninit();
protected:
	CSessionManager();
	virtual ~CSessionManager();
};

#endif //__SESSIONMANAGER_H__
