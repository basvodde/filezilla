#ifndef __POWER_MANAGEMENT_H__
#define __POWER_MANAGEMENT_H__

// This simple class prevents the system from entering idle sleep
// while FileZilla is busy

#include "state.h"
#ifdef __WXMAC__
	// >= 10.5 Required for Power Management
	#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
		#include <IOKit/pwr_mgt/IOPMLib.h>
	#endif
#endif

class CMainFrame;
#ifdef WITH_LIBDBUS
class CPowerManagementInhibitor;
#endif

class CPowerManagement : protected CStateEventHandler
{
public:
	static void Create(CMainFrame* pMainFrame);
	static void Destroy();

	static bool IsSupported();
protected:
	CPowerManagement(CMainFrame* pMainFrame);
	virtual ~CPowerManagement();
	
	static CPowerManagement* m_pPowerManagement;

	virtual void OnStateChange(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2);

	void DoSetBusy();
	void DoSetIdle();

	bool m_busy;
	
	CMainFrame* m_pMainFrame;

#ifdef WITH_LIBDBUS
	CPowerManagementInhibitor *m_inhibitor;
#elif defined(__WXMAC__)
	#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
		// >= 10.5 Required for Power Management
		IOPMAssertionID m_assertionID;
	#endif
#endif
};

#endif //__POWER_MANAGEMENT_H__
