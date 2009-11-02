#ifndef __DESKTOP_NOTIFICATION_H__
#define __DESKTOP_NOTIFICATION_H__

/* In accordance to the Desktop Notifications Specification
   available at
   http://www.galago-project.org/specs/notification/0.9/index.html
*/

class CDesktopNotificationImpl;
class CDesktopNotification
{
public:
	CDesktopNotification();
	virtual ~CDesktopNotification();

	void Notify(const wxString& summary, const wxString& body, const wxString& category);
	void RequestBusy();
private:
	CDesktopNotificationImpl *impl;
};

#endif //__DESKTOP_NOTIFICATION_H__

