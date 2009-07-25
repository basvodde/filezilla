#ifndef __POWER_MANAGEMENT_H__
#define __POWER_MANAGEMENT_H__

// This simple class prevents the system from entering idle sleep
// while FileZilla is busy

#include "state.h"

class CMainFrame;
class CPowerManagement : protected CStateEventHandler
{
public:
	static void Create(CMainFrame* pMainFrame);
	static void Destroy();

protected:
	CPowerManagement(CMainFrame* pMainFrame);
	virtual ~CPowerManagement() {}
	
	static CPowerManagement* m_pPowerManagement;

	virtual void OnStateChange(CState* pState, enum t_statechange_notifications notification, const wxString& data, const void* data2);

	void DoSetBusy();
	void DoSetIdle();

	bool m_busy;
	
	CMainFrame* m_pMainFrame;
};

#endif //__POWER_MANAGEMENT_H__
