#include <wx/wx.h>
#include "desktop_notification.h"
#include "wxdbusconnection.h"
#include "wxdbusmessage.h"
#include <list>
#include <memory>

class CDesktopNotificationImpl : private wxEvtHandler
{
public:
	CDesktopNotificationImpl();
	virtual ~CDesktopNotificationImpl();

	void Notify(const wxString& summary, const wxString& body, const wxString& category);

protected:
	wxDBusConnection* m_pConnection;

	void EmitNotifications();

	bool m_debug;

	enum _state
	{
		error,
		idle,
		busy
	} m_state;

	struct _notification
	{
		wxString summary;
		wxString body;
		wxString category;
	};
	std::list<struct _notification> m_notifications;

	DECLARE_EVENT_TABLE();
	void OnSignal(wxDBusConnectionEvent& event);
	void OnAsyncReply(wxDBusConnectionEvent& event);
};

BEGIN_EVENT_TABLE(CDesktopNotificationImpl, wxEvtHandler)
EVT_DBUS_SIGNAL(wxID_ANY, CDesktopNotificationImpl::OnSignal)
EVT_DBUS_ASYNC_RESPONSE(wxID_ANY, CDesktopNotificationImpl::OnAsyncReply)
END_EVENT_TABLE()

CDesktopNotification::CDesktopNotification()
{
	impl = new CDesktopNotificationImpl;
}

CDesktopNotification::~CDesktopNotification()
{
	delete impl;
}

void CDesktopNotification::Notify(const wxString& summary, const wxString& body, const wxString& category)
{
	impl->Notify(summary, body, category);
}

CDesktopNotificationImpl::CDesktopNotificationImpl()
{
#ifdef __WXDEBUG__
	m_debug = true;
#else
	m_debug = false;
	wxString v;
	if (wxGetEnv(_T("FZDEBUG"), &v) && v == _T("1"))
		m_debug = true;
#endif
	m_pConnection = new wxDBusConnection(wxID_ANY, this, false);
	if (!m_pConnection->IsConnected())
	{
		if (m_debug)
			printf("wxD-Bus: Could not connect to session bus\n");
		m_state = error;
	}
	else
		m_state = idle;
}

CDesktopNotificationImpl::~CDesktopNotificationImpl()
{
	delete m_pConnection;
}

void CDesktopNotificationImpl::Notify(const wxString& summary, const wxString& body, const wxString& category)
{
	if (m_state == error)
		return;

	struct _notification notification;
	notification.summary = summary;
	notification.body = body;
	notification.category = category;

	m_notifications.push_back(notification);

	EmitNotifications();
}

void CDesktopNotificationImpl::EmitNotifications()
{
	if (m_state != idle)
		return;

	if (m_notifications.empty())
		return;

	m_state = busy;

	struct _notification notification = m_notifications.front();
	m_notifications.pop_front();

	wxDBusMethodCall *call = new wxDBusMethodCall(
		"org.freedesktop.Notifications",
		"/org/freedesktop/Notifications",
		"org.freedesktop.Notifications",
		"Notify");

	call->AddString("FileZilla");
	call->AddUnsignedInt(0);
	call->AddString("filezilla");
	call->AddString(notification.summary.mb_str());
	call->AddString(notification.body.mb_str());
	call->AddArrayOfString(0, 0);

	if (notification.category != _T(""))
	{
		const wxWX2MBbuf category = notification.category.mb_str();
		const char *hints[2];
		hints[0] = "category";
		hints[1] = (const char*)category;

		call->AddDict(hints, 2);
	}
	else
		call->AddDict(0, 0);

	call->AddInt(-1);

	if (!call->CallAsync(m_pConnection, 1000))
	{
		m_state = error;
		if (m_debug)
			printf("wxD-Bus: CPowerManagementInhibitor: Request failed\n");
	}

	delete call;
}

void CDesktopNotificationImpl::OnSignal(wxDBusConnectionEvent& event)
{
	std::auto_ptr<wxDBusMessage> msg(wxDBusMessage::ExtractFromEvent(&event));
}

void CDesktopNotificationImpl::OnAsyncReply(wxDBusConnectionEvent& event)
{
	std::auto_ptr<wxDBusMessage> msg(wxDBusMessage::ExtractFromEvent(&event));

	if (m_state == error)
		return;

	if (msg->GetType() == DBUS_MESSAGE_TYPE_ERROR)
	{
		if (m_debug)
			printf("wxD-Bus: Reply: Error: %s\n", msg->GetString());

		m_state = error;
		return;
	}

	m_state = idle;
	EmitNotifications();
}
