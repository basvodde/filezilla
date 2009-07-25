#ifndef __POWER_MANAGEMENT_INHIBITOR
#define __POWER_MANAGEMENT_INHIBITOR

// Inhibits system idle sleep through org.freedesktop.PowerManagement.Inhibit interface
// See http://people.freedesktop.org/~hughsient/temp/dbus-interface.html

class CPowerManagementInhibitorImpl;
class CPowerManagementInhibitor
{
public:
	CPowerManagementInhibitor();
	virtual ~CPowerManagementInhibitor();

	void RequestIdle();
	void RequestBusy();
private:
	CPowerManagementInhibitorImpl *impl;
};

#endif //__POWER_MANAGEMENT_INHIBITOR
