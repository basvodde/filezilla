#ifndef __POWER_MANAGEMENT_INHIBITOR
#define __POWER_MANAGEMENT_INHIBITOR

// Inhibits system idle sleep through either org.freedesktop.PowerManagement.Inhibit interface,
// see http://people.freedesktop.org/~hughsient/temp/dbus-interface.html
// or alternatively through org.gnome.SessionManager interface,
// see http://www.gnome.org/~mccann/gnome-session/docs/gnome-session.html
//
// The former seems to be deprecated, so fall back to GSM if
// org.freedesktop.PowerManagement.Inhibit does not work.

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
